#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ================= PIN DEFINISI =================
#define BUTTON 5
#define LED 6
#define BUZZER 2  // Buzzer on pin 2

// Variabel untuk melacak keadaan tombol (apakah sudah ditekan atau belum)
bool buttonState = false;

// ================= TASK CORE 0 =================
void Task_Core0(void *parameter) {
  for (;;) {
    if (!buttonState) {
      digitalWrite(LED, LOW); // LED mati
      noTone(BUZZER);          // Buzzer mati
      Serial.println("Core 0 aktif, LED mati, Buzzer OFF");
    }
    vTaskDelay(100 / portTICK_PERIOD_MS); // delay sederhana
  }
}

// ================= TASK CORE 1 =================
void Task_Core1(void *parameter) {
  for (;;) {
    if (buttonState) {
      digitalWrite(LED, HIGH); // LED menyala
      tone(BUZZER, 1000);      // Buzzer menyala dengan frekuensi 1000 Hz
      Serial.println("Core 1 aktif, LED menyala, Buzzer ON");
    }
    vTaskDelay(100 / portTICK_PERIOD_MS); // delay sederhana
  }
}

// ================= TASK BUTTON =================
void Task_Button(void *parameter) {
  bool lastButtonState = HIGH;
  for (;;) {
    bool currentButtonState = digitalRead(BUTTON); // Baca status tombol

    // Jika tombol ditekan (berubah dari HIGH ke LOW)
    if (lastButtonState == HIGH && currentButtonState == LOW) {
      buttonState = !buttonState; // Toggle antara Core 0 dan Core 1
      Serial.println("Tombol ditekan, status toggled");
    }

    lastButtonState = currentButtonState; // Simpan status tombol untuk perbandingan berikutnya
    vTaskDelay(50 / portTICK_PERIOD_MS); // debounce sederhana
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  pinMode(BUTTON, INPUT_PULLUP); // Tombol dengan pull-up internal
  pinMode(LED, OUTPUT);          // LED sebagai output
  pinMode(BUZZER, OUTPUT);       // Buzzer sebagai output
  
  digitalWrite(LED, LOW); // LED mati pada awalnya
  noTone(BUZZER);         // Buzzer mati pada awalnya

  // Jalankan task pada core yang sesuai
  xTaskCreatePinnedToCore(Task_Button, "Button_Task", 2048, NULL, 1, NULL, 1); // Core 1 untuk tombol
  xTaskCreatePinnedToCore(Task_Core0, "Core0_Task", 2048, NULL, 1, NULL, 0);   // Core 0 untuk LED OFF dan Buzzer OFF
  xTaskCreatePinnedToCore(Task_Core1, "Core1_Task", 2048, NULL, 1, NULL, 1);   // Core 1 untuk LED ON dan Buzzer ON
}

void loop() {
  // Tidak digunakan karena semua berjalan dalam task
}
