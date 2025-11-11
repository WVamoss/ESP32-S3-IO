#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// Pin Stepper
#define STEP_PIN 5
#define DIR_PIN 14

// Push Buttons
#define BTN_RIGHT 13  // putar kanan
#define BTN_LEFT 12   // putar kiri

// Task Handles
TaskHandle_t TaskStepperRight;
TaskHandle_t TaskStepperLeft;

// Mutex untuk memastikan kedua core tidak rebutan DIR
SemaphoreHandle_t xDirMutex;

void stepperStep() {
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(10000);   // atur kecepatan di sini
  digitalWrite(STEP_PIN, LOW);
  delayMicroseconds(10000);
}

// Task Core 0 → Putar ke Kanan
void TaskRightCore0(void * pvParameters){
  for(;;){
    if (digitalRead(BTN_RIGHT) == LOW) {

      if (xSemaphoreTake(xDirMutex, portMAX_DELAY)) {
        digitalWrite(DIR_PIN, HIGH);  // arah kanan
        stepperStep();
        xSemaphoreGive(xDirMutex);
      }

      Serial.printf("Stepper Kanan - Core %d\n", xPortGetCoreID());
    }
    vTaskDelay(1);
  }
}

// Task Core 1 → Putar ke Kiri
void TaskLeftCore1(void * pvParameters){
  for(;;){
    if (digitalRead(BTN_LEFT) == LOW) {

      if (xSemaphoreTake(xDirMutex, portMAX_DELAY)) {
        digitalWrite(DIR_PIN, LOW);  // arah kiri
        stepperStep();
        xSemaphoreGive(xDirMutex);
      }

      Serial.printf("Stepper Kiri - Core %d\n", xPortGetCoreID());
    }
    vTaskDelay(1);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);

  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);

  xDirMutex = xSemaphoreCreateMutex(); // init mutex

  xTaskCreatePinnedToCore(TaskRightCore0, "StepperKanan", 2048, NULL, 1, &TaskStepperRight, 0);
  xTaskCreatePinnedToCore(TaskLeftCore1, "StepperKiri", 2048, NULL, 1, &TaskStepperLeft, 1);

  Serial.println("Stepper Control Ready (RTOS Enabled)");
}

void loop() {
  // kosong, semua berjalan di FreeRTOS task
}
