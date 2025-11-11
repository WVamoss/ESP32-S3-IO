#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include <AccelStepper.h>

// ================= OLED =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
#define SDA_PIN 14
#define SCL_PIN 13
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ================= SERVO =================
#define SERVO_PIN 17
Servo myServo;
int servoPos = 0;
bool servoDir = true;

// ================= STEPPER =================
#define IN1 37
#define IN2 38
#define IN3 39
#define IN4 40
AccelStepper stepper(AccelStepper::FULL4WIRE, IN1, IN3, IN2, IN4);
bool stepDir = true;

// ================= POTENSIOMETER =================
#define POT_PIN 16
int lastPotValue = -1;

// ================= ROTARY ENCODER =================
#define CLK_PIN 4
#define DT_PIN 5
#define SW_PIN 6
int encoderPos = 0;
int lastCLK = HIGH;

// ================= BUTTON, LED, BUZZER =================
#define BUTTON_PIN 21
#define LED_PIN 2
#define BUZZER_PIN 1

// ================= TASK HANDLE =================
TaskHandle_t oledTaskHandle;
TaskHandle_t servoTaskHandle;
TaskHandle_t potTaskHandle;
TaskHandle_t encoderTaskHandle;
TaskHandle_t buttonTaskHandle;
TaskHandle_t ledTaskHandle;
TaskHandle_t buzzerTaskHandle;
TaskHandle_t stepperTaskHandle;

// ================= TASK 1: OLED =================
void OledTask(void *pvParameters) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 20);
  display.println("Task berjalan");
  display.display();
  // Task selesai setelah menampilkan pesan pertama kali
  vTaskDelete(NULL);
}

// ================= TASK 2: SERVO =================
void ServoTask(void *pvParameters) {
  Serial.println("Servo: Task berjalan");
  for (;;) {
    myServo.write(servoPos);
    if (servoDir) servoPos += 5;
    else servoPos -= 5;

    if (servoPos >= 180) servoDir = false;
    if (servoPos <= 0) servoDir = true;

    vTaskDelay(pdMS_TO_TICKS(150)); // Update posisi setiap 150ms
  }
}

// ================= TASK 3: POTENTIOMETER =================
void PotTask(void *pvParameters) {
  for (;;) {
    int potValue = analogRead(POT_PIN);
    int potPercentage = map(potValue, 0, 4095, 0, 100);  // Map ke % (0-100)

    // Hanya cetak jika ada perubahan signifikan pada nilai potensiometer
    if (potPercentage != lastPotValue) {
      Serial.print("Task Potensiometer value: ");
      Serial.print(potPercentage);
      Serial.println("%");
      lastPotValue = potPercentage;
    }

    vTaskDelay(pdMS_TO_TICKS(500)); // Update tiap 500ms
  }
}

// ================= TASK 4: ROTARY ENCODER =================
void EncoderTask(void *pvParameters) {
  for (;;) {
    int clkState = digitalRead(CLK_PIN);
    if (clkState != lastCLK && clkState == LOW) {
      if (digitalRead(DT_PIN) != clkState)
        encoderPos++;
      else
        encoderPos--;
      Serial.print("Task Encoder value: ");
      Serial.println(encoderPos);
    }
    lastCLK = clkState;

    // Reset posisi encoder jika tombol encoder ditekan
    if (digitalRead(SW_PIN) == LOW) {
      encoderPos = 0;
      Serial.println("Task Encoder: Direset ke 0");
      vTaskDelay(pdMS_TO_TICKS(300));  // Debounce
    }

    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// ================= TASK 5: BUTTON =================
void ButtonTask(void *pvParameters) {
  for (;;) {
    if (digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("Task Button: Diklik");
      vTaskDelay(pdMS_TO_TICKS(300)); // Debounce
    }
    vTaskDelay(pdMS_TO_TICKS(50)); // Polling interval
  }
}

// ================= TASK 6: LED =================
void LedTask(void *pvParameters) {
  for (;;) {
    digitalWrite(LED_PIN, HIGH);   // LED menyala
    Serial.println("Task LED: Nyala");
    vTaskDelay(pdMS_TO_TICKS(5000)); // Nyala selama 5 detik
    digitalWrite(LED_PIN, LOW);    // LED mati
    Serial.println("Task LED: Mati");
    vTaskDelay(pdMS_TO_TICKS(5000)); // Mati selama 5 detik
  }
}

// ================= TASK 7: BUZZER =================
void BuzzerTask(void *pvParameters) {
  for (;;) {
    digitalWrite(BUZZER_PIN, HIGH);   // Buzzer menyala
    Serial.println("Task Buzzer: Nyala");
    vTaskDelay(pdMS_TO_TICKS(5000));  // Buzzer menyala selama 5 detik
    digitalWrite(BUZZER_PIN, LOW);    // Buzzer mati
    Serial.println("Task Buzzer: Mati");
    vTaskDelay(pdMS_TO_TICKS(5000));  // Buzzer mati selama 5 detik
  }
}

// ================= TASK 8: STEPPER =================
void StepperTask(void *pvParameters) {
  Serial.println("Stepper: Task berjalan");
  stepper.setMaxSpeed(500);
  stepper.setAcceleration(200);
  for (;;) {
    if (stepDir)
      stepper.moveTo(200);
    else
      stepper.moveTo(0);

    stepper.run();
    if (stepper.distanceToGo() == 0) stepDir = !stepDir;

    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("Gagal inisialisasi OLED!");
    for (;;);
  }
  display.clearDisplay();
  display.display();

  // I/O
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(CLK_PIN, INPUT);
  pinMode(DT_PIN, INPUT);
  pinMode(SW_PIN, INPUT_PULLUP);

  // Servo
  myServo.attach(SERVO_PIN);
  // Stepper
  stepper.setSpeed(200);

  // ================= BUAT SEMUA TASK =================
  xTaskCreatePinnedToCore(OledTask, "OLED", 4096, NULL, 1, &oledTaskHandle, 0);
  xTaskCreatePinnedToCore(ServoTask, "Servo", 2048, NULL, 2, &servoTaskHandle, 1);
  xTaskCreatePinnedToCore(PotTask, "Pot", 2048, NULL, 1, &potTaskHandle, 0);
  xTaskCreatePinnedToCore(EncoderTask, "Encoder", 2048, NULL, 1, &encoderTaskHandle, 1);
  xTaskCreatePinnedToCore(ButtonTask, "Button", 2048, NULL, 1, &buttonTaskHandle, 0);
  xTaskCreatePinnedToCore(LedTask, "LED", 2048, NULL, 1, &ledTaskHandle, 0);
  xTaskCreatePinnedToCore(BuzzerTask, "Buzzer", 2048, NULL, 1, &buzzerTaskHandle, 0);
  xTaskCreatePinnedToCore(StepperTask, "Stepper", 4096, NULL, 2, &stepperTaskHandle, 1);

  Serial.println("=== FreeRTOS 8 Task Started ===");
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}

