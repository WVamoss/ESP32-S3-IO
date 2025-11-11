# 01 — Dual Core LED Blinking (ESP32-S3)
## Description / Deskripsi 

Percobaan ini menunjukkan proses LED blinking secara paralel pada dua core berbeda di ESP32-S3 menggunakan FreeRTOS Task.

- LED1 dikendalikan oleh task yang berjalan pada **Core 0**  
- LED2 dikendalikan oleh task yang berjalan pada **Core 1**  
- Kedua task berjalan independen dengan interval kedip berbeda  
- Tidak menggunakan loop() (seluruh logic berjalan pada task)

Tujuan utama: membuktikan bahwa kedua core dapat menjalankan task berbeda secara simultan tanpa blocking, serta memberikan bukti visual bahwa eksekusi benar-benar paralel di kedua core.


## Hardware Mapping

| Device | Pin     | Mode                 |
|--------|---------|----------------------|
| LED1   | GPIO 2  | Output (Task Core 0) |
| LED2   | GPIO 15 | Output (Task Core 1) |


## Test Procedure / Langkah Percobaan
|No	|Step	Expected Result|
|---|--------------------|
|1	|Upload program	Serial Monitor tampil log Core 0 & Core 1|
|2	|Observe LED1	LED1 berkedip lebih cepat (~300ms)|
|3	|Observe LED2	LED2 berkedip lebih lambat (~500ms)|
|4	|Observe Serial Output	Ada tulisan “Core 0” dan “Core 1” → membuktikan task berjalan pada core berbeda|

## Image Evidence
### 1. Core 0 berjalan 

<img width="711" height="591" alt="Screenshot 2025-11-10 145803" src="https://github.com/user-attachments/assets/ee035afc-bf1b-4ce3-9b87-3a9967abbab9" />


### 2. Core 1 berjalan 

<img width="711" height="591" alt="Screenshot 2025-11-10 145803" src="https://github.com/user-attachments/assets/ee035afc-bf1b-4ce3-9b87-3a9967abbab9" />


gambar menunjukkan log berbeda tiap core secara jelas

LED blink pattern konsisten dan tidak jitter


## Video Evidence

Google Drive:
https://drive.google.com/file/d/1HoRDTLnAkrKF1bCPkEXxot8ThCFlIGAO/view?usp=sharing
