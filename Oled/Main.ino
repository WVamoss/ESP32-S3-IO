#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// minimal pins (from mapping you gave)
#define BUTTON1_PIN 13
#define BUTTON2_PIN 12

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
#define SDA_PIN 8
#define SCL_PIN 7

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// RTOS objects
TaskHandle_t taskButton = NULL;
TaskHandle_t taskOLED = NULL;
SemaphoreHandle_t oledMutex = NULL;
SemaphoreHandle_t oledExitSem = NULL; // signal OLED exit

// shared state
volatile uint8_t lastButtonPressed = 0; // 0=none,1=btn1,2=btn2
volatile bool requestSwapOLED = false;
volatile bool oledKillReq = false;      // orchestrator -> OLED: please exit
int oledPinnedCore = 1; // initial core for OLED task

// ---------------- Button Task (producer) ----------------
void TaskButton(void *pvParams) {
  const TickType_t pollDelay = 20 / portTICK_PERIOD_MS;
  bool last1 = digitalRead(BUTTON1_PIN);
  bool last2 = digitalRead(BUTTON2_PIN);
  TickType_t t1 = 0, t2 = 0;

  while (1) {
    bool cur1 = digitalRead(BUTTON1_PIN); // LOW = pressed
    bool cur2 = digitalRead(BUTTON2_PIN);

    // debounce simple:
    if (cur1 != last1) t1 = xTaskGetTickCount();
    else if (cur1 == LOW && (xTaskGetTickCount() - t1) * portTICK_PERIOD_MS >= 50) {
      lastButtonPressed = 1;
    }

    if (cur2 != last2) t2 = xTaskGetTickCount();
    else if (cur2 == LOW && (xTaskGetTickCount() - t2) * portTICK_PERIOD_MS >= 50) {
      lastButtonPressed = 2;
    }

    // detect BOTH pressed >1s -> request swap (long press)
    if (cur1 == LOW && cur2 == LOW) {
      TickType_t start = xTaskGetTickCount();
      while (digitalRead(BUTTON1_PIN) == LOW && digitalRead(BUTTON2_PIN) == LOW) {
        if ((xTaskGetTickCount() - start) * portTICK_PERIOD_MS >= 1000) {
          requestSwapOLED = true;
          vTaskDelay(300 / portTICK_PERIOD_MS); // debounce guard
          break;
        }
        vTaskDelay(30 / portTICK_PERIOD_MS);
      }
    }

    last1 = cur1;
    last2 = cur2;
    vTaskDelay(pollDelay);
  }
}

// ---------------- OLED Task (single renderer) ----------------
void TaskOLED(void *pvParams) {
  const TickType_t renderDelay = 100 / portTICK_PERIOD_MS; // ~10Hz

  while (1) {
    // check kill request early so we don't block long on mutex
    if (oledKillReq) break;

    // lock I2C/display
    if (xSemaphoreTake(oledMutex, portMAX_DELAY)) {
      // immediately re-check kill flag after acquiring
      if (oledKillReq) {
        // release mutex and exit cleanly
        xSemaphoreGive(oledMutex);
        break;
      }

      display.clearDisplay();

      display.setTextSize(1);
      display.setCursor(0, 0);
      display.printf("OLED pinned core: %d", xPortGetCoreID());

      display.setTextSize(2);
      display.setCursor(0, 18);
      switch (lastButtonPressed) {
        case 1: display.println("Button 1"); break;
        case 2: display.println("Button 2"); break;
        default: display.println("None"); break;
      }

      display.setTextSize(1);
      display.setCursor(0, 50);
      display.println("Hold both >1s to swap core");

      display.display(); // single commit

      xSemaphoreGive(oledMutex);
    }

    vTaskDelay(renderDelay);
  }

  // signal orchestrator that we're exiting (non-blocking give)
  if (oledExitSem) xSemaphoreGive(oledExitSem);

  // self-delete (clean)
  taskOLED = NULL; // clear handle (best-effort)
  vTaskDelete(NULL);
}

// ---------------- Setup & Orchestrator ----------------
void setup() {
  Serial.begin(115200);
  Serial.println("Stable OLED multi-core demo (text, low-refresh) - SAFE SWAP");

  // input pins
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

  // init I2C once here
  Wire.begin(SDA_PIN, SCL_PIN);
  delay(150);

  // init display
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED not found!");
    while (1) { vTaskDelay(1000 / portTICK_PERIOD_MS); }
  }

  // initial ready screen (single-shot)
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 20);
  display.print("READY");
  display.display();
  delay(400);

  // mutex for I2C/display
  oledMutex = xSemaphoreCreateMutex();
  if (!oledMutex) {
    Serial.println("oledMutex creation failed");
    while (1) { vTaskDelay(1000 / portTICK_PERIOD_MS); }
  }

  // create OLED exit semaphore (binary)
  oledExitSem = xSemaphoreCreateBinary();
  if (!oledExitSem) {
    Serial.println("oledExitSem creation failed");
    while (1) { vTaskDelay(1000 / portTICK_PERIOD_MS); }
  }

  // create button task pinned to CORE 0 (recommended)
  xTaskCreatePinnedToCore(TaskButton, "Button", 3072, NULL, 2, &taskButton, 0);

  // create oled task pinned to default core (oledPinnedCore)
  BaseType_t res = xTaskCreatePinnedToCore(TaskOLED, "OLED", 4096, NULL, 1, &taskOLED, oledPinnedCore);
  if (res == pdPASS) Serial.printf("OLED task created on core %d\n", oledPinnedCore);
  else Serial.println("Failed to create OLED task!");
}

void loop() {
  // orchestrator performs safe swap
  if (requestSwapOLED) {
    requestSwapOLED = false;
    Serial.println("[ORCH] swap requested -> performing safe swap...");

    // 1) request OLED to exit
    oledKillReq = true;

    // 2) wait for OLED to signal exit (timeout = 500 ms)
    const TickType_t waitTicks = pdMS_TO_TICKS(500);
    if (xSemaphoreTake(oledExitSem, waitTicks) == pdTRUE) {
      Serial.println("[ORCH] OLED exited cleanly (signaled).");
    } else {
      // fallback: OLED didn't signal in time. Try a safe forced delete as last resort.
      Serial.println("[ORCH] WARNING: OLED did not exit in time -> forcing delete (last-resort).");
      if (taskOLED != NULL) {
        vTaskDelete(taskOLED);
        taskOLED = NULL;
        vTaskDelay(20 / portTICK_PERIOD_MS);
      }
    }

    // reset kill flag (for next lifecycle)
    oledKillReq = false;

    // flip core and recreate OLED on other core
    oledPinnedCore = (oledPinnedCore == 0) ? 1 : 0;
    BaseType_t r = xTaskCreatePinnedToCore(TaskOLED, "OLED", 4096, NULL, 1, &taskOLED, oledPinnedCore);
    if (r == pdPASS) Serial.printf("[ORCH] OLED recreated on core %d\n", oledPinnedCore);
    else Serial.println("[ORCH] failed to recreate OLED!");
  }

  // small idle delay so loop() isn't hogging CPU
  vTaskDelay(50 / portTICK_PERIOD_MS);
}
