#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Encoder Pins
#define CLK 16
#define DT  17

// Task Handles
TaskHandle_t TaskEncoderCore0;
TaskHandle_t TaskEncoderCore1;

volatile int encoderValue = 0;
int lastCLKState;

void IRAM_ATTR readEncoder() {
  int currentCLK = digitalRead(CLK);
  int currentDT  = digitalRead(DT);

  if (currentCLK != lastCLKState) { 
    if (currentDT != currentCLK) encoderValue++;
    else encoderValue--;
  }
  lastCLKState = currentCLK;
}

// Task di Core 0
void Task_ReadEncoder_Core0(void *pvParameters) {
  for(;;){
    Serial.printf("[CORE 0] Encoder Value: %d\n", encoderValue);
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

// Task di Core 1
void Task_ReadEncoder_Core1(void *pvParameters) {
  for(;;){
    Serial.printf("[CORE 1] Encoder Value: %d\n", encoderValue);
    vTaskDelay(pdMS_TO_TICKS(700));
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(CLK, INPUT_PULLUP);
  pinMode(DT, INPUT_PULLUP);

  lastCLKState = digitalRead(CLK);

  attachInterrupt(digitalPinToInterrupt(CLK), readEncoder, CHANGE);

  xTaskCreatePinnedToCore(Task_ReadEncoder_Core0, "EncoderCore0", 2048, NULL, 1, &TaskEncoderCore0, 0);
  xTaskCreatePinnedToCore(Task_ReadEncoder_Core1, "EncoderCore1", 2048, NULL, 1, &TaskEncoderCore1, 1);

  Serial.println("Dual-Core Encoder Read Started");
}

void loop() {}
