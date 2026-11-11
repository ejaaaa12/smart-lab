#include <Arduino.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define SOIL_PIN 34
#define DHTPIN   15
#define TRIG     5
#define ECHO     18
#define PUMP_PIN 26
#define BUZZER   25

// Kalau pompa (LED) terbalik di Wokwi, tukar HIGH <-> LOW di sini
#define RELAY_ON  HIGH
#define RELAY_OFF LOW

DHT dht(DHTPIN, DHT22);
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int SOIL_DRY_PERCENT = 40;   // di bawah ini = tanah kering -> pompa ON
const int SOIL_WET_PERCENT = 60;   // di atas ini  = tanah basah  -> pompa OFF
const int TANK_EMPTY_CM    = 15;   // jarak sensor ke air > 15 cm = tandon kosong
const int TANK_FULL_CM     = 3;    // jarak sensor ke air <= 3 cm = tandon penuh

bool pumpOn = false;
bool lastPump = false;
bool lastTankEmpty = false;
float t = 0, h = 0;
unsigned long lastDHT = 0;
unsigned long lastReport = 0;

// ---------- Fungsi bantu ----------
String soilStatus(int soil) {
  if (soil < SOIL_DRY_PERCENT) return "Kering";
  if (soil <= SOIL_WET_PERCENT) return "Cukup";
  return "Basah";
}

String tempStatus(float t) {
  if (t < 22) return "Dingin";
  if (t < 28) return "Nyaman";
  if (t < 32) return "Hangat";
  return "PANAS";
}

float readDistanceCm() {
  digitalWrite(TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long d = pulseIn(ECHO, HIGH, 30000);
  if (d == 0) return 999;
  return d * 0.0343 / 2;
}

int tankPercent(float dist) {
  float p = (TANK_EMPTY_CM - dist) * 100.0 / (TANK_EMPTY_CM - TANK_FULL_CM);
  return constrain((int)p, 0, 100);
}

void lcdLine(int row, String s) {
  while (s.length() < 16) s += ' ';
  lcd.setCursor(0, row);
  lcd.print(s.substring(0, 16));
}

void printReport(int soil, float dist, int tank, bool tankEmpty) {
  Serial.println();
  Serial.println("============== LAPORAN SMART FARMING ==============");
  Serial.printf("Kelembapan tanah : %d %%  -> %s (kering < %d%%, basah > %d%%)\n",
                soil, soilStatus(soil).c_str(), SOIL_DRY_PERCENT, SOIL_WET_PERCENT);
  Serial.printf("Suhu udara       : %.1f C  -> %s\n", t, tempStatus(t).c_str());
  Serial.printf("Kelembapan udara : %.0f %%\n", h);
  if (dist >= 999) {
    Serial.println("Jarak ke air     : tidak terbaca");
  } else {
    Serial.printf("Jarak ke air     : %.0f cm\n", dist);
  }
  Serial.printf("Isi tandon       : %d %%  -> %s\n", tank, tankEmpty ? "KOSONG!" : "Aman");
  Serial.printf("Pompa air        : %s\n", pumpOn ? "MENYALA (menyiram)" : "MATI");
  Serial.printf("Alarm            : %s\n", tankEmpty ? "BERBUNYI (tandon kosong)" : "Diam");
  Serial.println("===================================================");
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(PUMP_PIN, RELAY_OFF);

  dht.begin();
  lcd.init();
  lcd.backlight();
  lcdLine(0, "Smart Farming");

  Serial.println("--------------------------------------------------");
  Serial.println(" SMART FARMING - Penyiraman Otomatis (ESP32)");
  Serial.println("--------------------------------------------------");
  Serial.println("Cara kerja singkat:");
  Serial.println(" 1. Tanah KERING (< 40%) -> pompa menyala, tanaman disiram");
  Serial.println(" 2. Tanah BASAH (> 60%)  -> pompa mati");
  Serial.println(" 3. Antara 40-60%        -> pompa tetap di kondisi terakhir");
  Serial.println("    (supaya pompa tidak nyala-mati terus, disebut hysteresis)");
  Serial.println(" 4. Tandon KOSONG        -> pompa dipaksa mati + alarm bunyi");
  Serial.println("    (supaya pompa tidak rusak karena jalan tanpa air)");
  Serial.println("--------------------------------------------------");

  delay(1500);
  lcd.clear();

  float nt = dht.readTemperature();
  float nh = dht.readHumidity();
  if (!isnan(nt) && !isnan(nh)) { t = nt; h = nh; }
}

void loop() {
  // --- Baca sensor ---
  int raw = analogRead(SOIL_PIN);
  int soil = map(raw, 0, 4095, 0, 100);   // potensiometer: 0 = kering, 100 = basah

  if (millis() - lastDHT > 2000) {
    float nt = dht.readTemperature();
    float nh = dht.readHumidity();
    if (!isnan(nt) && !isnan(nh)) { t = nt; h = nh; }
    else Serial.println("[PERINGATAN] Sensor DHT22 gagal dibaca, cek kabel!");
    lastDHT = millis();
  }

  float dist = readDistanceCm();
  bool tankEmpty = dist > TANK_EMPTY_CM;
  int tank = tankPercent(dist);

  // --- Logika pompa (hysteresis + proteksi tandon kosong) ---
  if (tankEmpty) {
    pumpOn = false;
  } else if (soil < SOIL_DRY_PERCENT) {
    pumpOn = true;
  } else if (soil > SOIL_WET_PERCENT) {
    pumpOn = false;
  }

  digitalWrite(PUMP_PIN, pumpOn ? RELAY_ON : RELAY_OFF);
  digitalWrite(BUZZER, tankEmpty ? HIGH : LOW);

  // --- Event: dicetak sekali saat kondisi berubah ---
  if (pumpOn != lastPump) {
    if (pumpOn) {
      Serial.printf("[EVENT] Tanah KERING (%d%%) -> Pompa MENYALA, tanaman disiram\n", soil);
    } else if (tankEmpty) {
      Serial.println("[EVENT] Tandon kosong -> Pompa DIMATIKAN paksa agar tidak rusak");
    } else {
      Serial.printf("[EVENT] Tanah sudah cukup basah (%d%%) -> Pompa MATI\n", soil);
    }
    lastPump = pumpOn;
  }
  if (tankEmpty != lastTankEmpty) {
    if (tankEmpty) Serial.println("[ALARM!] Air di tandon HABIS -> buzzer berbunyi, isi ulang tandon!");
    else           Serial.println("[INFO] Tandon sudah terisi air lagi -> alarm berhenti");
    lastTankEmpty = tankEmpty;
  }

  // --- LCD: 2 halaman bergantian tiap 3 detik ---
  int page = (millis() / 3000) % 2;
  if (page == 0) {
    lcdLine(0, "Tanah:" + String(soil) + "% " + soilStatus(soil));
    lcdLine(1, String("Pompa: ") + (pumpOn ? "MENYALA" : "MATI"));
  } else {
    lcdLine(0, "Suhu:" + String(t, 1) + "C " + String(h, 0) + "%");
    lcdLine(1, tankEmpty ? String("TANDON KOSONG!") : "Tandon: " + String(tank) + "%");
  }

  // --- Laporan lengkap tiap 3 detik ---
  if (millis() - lastReport > 3000) {
    printReport(soil, dist, tank, tankEmpty);
    lastReport = millis();
  }

  delay(100);
}