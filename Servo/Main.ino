#include <ESP32Servo.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define SERVO_PIN 9

#define BTN_RIGHT 13   // Button to move the servo to the right (Clockwise) → Core 0
#define BTN_LEFT  12   // Button to move the servo to the left (Counter-Clockwise) → Core 1

Servo myServo;

int angle = 90; // Initial angle at the center

SemaphoreHandle_t servoMutex; // Mutex for accessing the servo

TaskHandle_t TaskRightCore0;
TaskHandle_t TaskLeftCore1;

// ============= TASK TO MOVE SERVO TO THE RIGHT (CORE 0) ===============
void Task_ServoRight(void *pvParameters){
  while(1){
    if(digitalRead(BTN_RIGHT) == LOW){  // Button pressed
      if(angle < 180){
        xSemaphoreTake(servoMutex, portMAX_DELAY);
        angle++;
        myServo.write(angle);
        xSemaphoreGive(servoMutex);
      }
      Serial.printf("RIGHT -> Angle: %d | Core: %d\n", angle, xPortGetCoreID());
      vTaskDelay(20 / portTICK_PERIOD_MS);
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// ============= TASK TO MOVE SERVO TO THE LEFT (CORE 1) ===============
void Task_ServoLeft(void *pvParameters){
  while(1){
    if(digitalRead(BTN_LEFT) == LOW){   // Button pressed
      if(angle > 0){
        xSemaphoreTake(servoMutex, portMAX_DELAY);
        angle--;
        myServo.write(angle);
        xSemaphoreGive(servoMutex);
      }
      Serial.printf("LEFT -> Angle: %d | Core: %d\n", angle, xPortGetCoreID());
      vTaskDelay(20 / portTICK_PERIOD_MS);
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void setup(){
  Serial.begin(115200);
  delay(1000);

  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);

  myServo.attach(SERVO_PIN);

  servoMutex = xSemaphoreCreateMutex();

  // Run right control on Core 0
  xTaskCreatePinnedToCore(Task_ServoRight, "ServoRight", 4096, NULL, 2, &TaskRightCore0, 0);

  // Run left control on Core 1
  xTaskCreatePinnedToCore(Task_ServoLeft, "ServoLeft", 4096, NULL, 2, &TaskLeftCore1, 1);
}

void loop(){
  // Empty loop, tasks are running on separate cores
}
