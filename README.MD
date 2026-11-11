# 🌾 Smart Farming - Sistem Penyiraman Otomatis berbasis ESP32

Proyek Internet of Things (IoT) menggunakan mikrokontroler **ESP32** untuk mengotomatisasi penyiraman tanaman berdasarkan tingkat kelembapan tanah, suhu udara, serta dilengkapi dengan sistem keamanan tandon air dan alarm berbasis *Ultrasonic* serta *Buzzer*.

---

## 🚀 Fitur Utama
1. **Penyiraman Otomatis dengan Hysteresis:** Mencegah pompa menyala-mati secara terus-menerus di ambang batas kelembapan tertentu.
2. **Proteksi Tandon Air Kosong:** Pompa akan otomatis dimatikan dan *buzzer* akan berbunyi jika air di dalam tandon habis, mencegah kerusakan pada pompa akibat jalan tanpa air (*dry running*).
3. **Monitoring Lingkungan (DHT22):** Memantau suhu dan kelembapan udara sekitar secara *real-time*.
4. **Tampilan LCD I2C (16x2):** Menampilkan informasi status secara bergantian dalam 2 halaman (Status Tanah & Pompa, serta Suhu/Kelembapan & Tandon Air).
5. **Serial Monitor / Laporan Berkala:** Memberikan log status operasional secara berkala dan *event-driven* untuk proses *debugging*.

---

## 🛠️ Komponen yang Digunakan
* **Mikrokontroler:** ESP32 DevKit V1
* **Sensor Kelembapan Tanah:** Kapasitif/Resistif (terhubung ke pin analog `GPIO 34`)
* **Sensor Suhu & Kelembapan Udara:** DHT22 (`GPIO 15`)
* **Sensor Ultrasonik:** HC-SR04 untuk pengukuran level air tandon (`TRIG: GPIO 5`, `ECHO: GPIO 18`)
* **Aktuator / Output:**
  * Pompa Air / Relay (`GPIO 26`)
  * Buzzer Alarm (`GPIO 25`)
* **Display:** LCD 16x2 dengan Modul I2C (`Address: 0x27`)

---

## 📌 Logika Kerja Sistem
1. **Tanah Kering (`< 40%`)** ➔ Pompa **Menyala** (Tanaman disiram).
2. **Tanah Basah (`> 60%`)** ➔ Pompa **Mati**.
3. **Kondisi Normal (40% - 60%)** ➔ Mempertahankan status pompa sebelumnya (Mencegah *chattering* / *hysteresis*).
4. **Tandon Kosong (`Jarak > 15 cm`)** ➔ Pompa **Dimatikan Paksa** + **Buzzer Berbunyi**.

---

## 📦 Pustaka (Library) yang Dibutuhkan
Sebelum mengunggah kode ke ESP32, pastikan Anda telah menginstal *library* berikut melalui Library Manager di Arduino IDE:
* `DHT sensor library` (oleh Adafruit)
* `LiquidCrystal_I2C` (oleh Frank de Brabander)

---

## ⚙️ Cara Instalasi & Penggunaan
1. Hubungkan komponen ke ESP32 sesuai dengan definisi pin pada kode:
   * `SOIL_PIN` ➔ `GPIO 34`
   * `DHTPIN` ➔ `GPIO 15`
   * `TRIG` ➔ `GPIO 5` | `ECHO` ➔ `GPIO 18`
   * `PUMP_PIN` ➔ `GPIO 26`
   * `BUZZER` ➔ `GPIO 25`
   * `SDA` / `SCL` ➔ Pin I2C default ESP32 (`GPIO 21` / `GPIO 22`)
2. Buka proyek ini menggunakan **Arduino IDE** atau simulasikan melalui platform seperti **Wokwi**.
3. Pilih *Board* **ESP32 Dev Module**.
4. Unggah (*Upload*) program ke ESP32.
5. Buka **Serial Monitor** pada kecepatan `115200 baud` untuk melihat laporan status secara *real-time*.

---

## 💡 Catatan Tambahan
Jika pada simulasi Wokwi indikator pompa (LED) terbalik, Anda dapat mengubah makro berikut pada bagian atas kode:
```cpp
#define RELAY_ON  HIGH
#define RELAY_OFF LOW
// Tukar jika diperlukan:
// #define RELAY_ON  LOW
// #define RELAY_OFF HIGH