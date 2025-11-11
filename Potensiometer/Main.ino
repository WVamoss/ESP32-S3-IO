#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define POT_PIN 4   // pin potensiometer (ADC)
#define LED_PIN 1    // pin LED output

TaskHandle_t TaskCore0;
TaskHandle_t TaskCore1;

volatile int potValue = 0;        // nilai global dari potensiometer
volatile int ledBrightness = 0;   // nilai PWM untuk LED

// ===== CORE 0: Baca potensiometer, ubah LED, tampilkan log singkat =====
void TaskPotentiometer_Core0(void *pvParameters) {
  pinMode(POT_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  while (1) {
    // Baca potensiometer
    potValue = analogRead(POT_PIN);        // 0 - 4095
    ledBrightness = map(potValue, 0, 4095, 0, 255);

    // Atur LED sesuai potensiometer
    analogWrite(LED_PIN, ledBrightness);

    // Log singkat dari Core 0
    Serial.printf("[CORE 0] PotValue: %d | LED Brightness: %d | Running on Core %d\n",
                  potValue, ledBrightness, xPortGetCoreID());

    vTaskDelay(pdMS_TO_TICKS(200));  // Delay 200ms antar pembacaan
  }
}

// ===== CORE 1: Menampilkan data monitoring lengkap =====
void TaskMonitor_Core1(void *pvParameters) {
  while (1) {
    float voltage = (potValue / 4095.0) * 3.3;

    Serial.printf("[CORE 1] Raw: %d | Voltage: %.2f V | LED PWM: %d | Running on Core %d\n",
                  potValue, voltage, ledBrightness, xPortGetCoreID());

    vTaskDelay(pdMS_TO_TICKS(500));  // Tampilkan tiap 0.5 detik
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Buat task untuk Core 0 (baca pot dan kontrol LED)
  xTaskCreatePinnedToCore(
    TaskPotentiometer_Core0,
    "TaskCore0",
    4096,
    NULL,
    1,
    &TaskCore0,
    0 // Core 0
  );

  // Buat task untuk Core 1 (monitor)
  xTaskCreatePinnedToCore(
    TaskMonitor_Core1,
    "TaskCore1",
    4096,
    NULL,
    1,
    &TaskCore1,
    1 // Core 1
  );
}

void loop() {
  // Kosong — semua berjalan di task RTOS
}
