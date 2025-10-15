/*
 * KODE KALIBRASI HX711 - DENGAN LCD 20x4 I2C
 * Tunggu 10 detik untuk penempatan beban,
 * lalu lakukan pembacaan dummy selama 5 detik agar sinyal stabil,
 * kemudian ambil 500 data dan hitung rata-rata.
 * Hasil akhir ditampilkan di LCD.
 */

#include <Arduino.h>
#include "HX711_ADC.h"
#include <LiquidCrystal_I2C.h>

// Inisialisasi LCD 20x4 I2C
LiquidCrystal_I2C lcd(0x27, 20, 4); // Ganti alamat I2C jika perlu (0x27 atau 0x3F)

const int HX711_dout = 2;
const int HX711_sck = 4;

HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int numSamples = 500;
const float knownWeight = 1000.0; // gram

void setup() {
  Serial.begin(115200);
  
  // Inisialisasi LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  
  // Tampilan awal di LCD
  lcd.setCursor(0, 0);
  lcd.print("  KALIBRASI HX711  ");
  lcd.setCursor(0, 1);
  lcd.print(" Letakkan beban 1kg ");
  lcd.setCursor(0, 2);
  lcd.print("   dalam 10 detik  ");
  lcd.setCursor(0, 3);
  lcd.print("====================");

  Serial.println();
  Serial.println("==============================================");
  Serial.println("     Program Kalibrasi HX711 (500 sampel)");
  Serial.println("==============================================");

  LoadCell.begin();
  Serial.println("Memulai timbangan...");
  LoadCell.start(2000, true);

  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout! Cek koneksi HX711.");
    lcd.clear();
    lcd.print("  ERROR: Timeout   ");
    lcd.setCursor(0, 1);
    lcd.print(" Cek koneksi HX711 ");
    while (1);
  }

  LoadCell.setCalFactor(1.0);
  Serial.println("Faktor kalibrasi diatur ke 1.0 untuk pembacaan mentah.");

  Serial.println("\n--> Letakkan beban acuan (misal 1000g).");
  Serial.println("--> Pengambilan data akan dimulai dalam 10 detik...");
  
  // Countdown di LCD
  for (int i = 10; i > 0; i--) {
    lcd.setCursor(8, 3);
    lcd.print("    ");
    lcd.setCursor(8, 3);
    lcd.print(i);
    lcd.print("s ");
    delay(1000);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Stabilisasi sinyal...");
  lcd.setCursor(0, 1);
  lcd.print("5 detik");

  Serial.println("\nMenstabilkan pembacaan selama 5 detik...");
  unsigned long t0 = millis();
  while (millis() - t0 < 5000) {
    LoadCell.update();
    delay(10);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Merekam 500 data...");
  lcd.setCursor(0, 1);
  lcd.print("Progress: ");

  Serial.println("Mulai merekam data...");
  Serial.println("----------------------------------------------");
}

void loop() {
  static bool done = false;

  if (!done) {
    float sum = 0;
    int count = 0;

    while (count < numSamples) {
      if (LoadCell.update()) {
        float reading = LoadCell.getData();
        sum += reading;
        count++;

        // Update progress di LCD setiap 50 data
        if (count % 50 == 0) {
          lcd.setCursor(10, 1);
          lcd.print("    ");
          lcd.setCursor(10, 1);
          lcd.print(count);
          lcd.print("/500");
        }

        Serial.print("Reading (Cal Factor = 1.0): ");
        Serial.println(reading, 0);
        delay(50);
      }
    }

    float avg = sum / numSamples;
    float calValue = avg / knownWeight;

    // Tampilan hasil di Serial
    Serial.println("----------------------------------------------");
    Serial.print("Rata-rata nilai mentah (500 sampel): ");
    Serial.println(avg, 3);
    Serial.print("Nilai kalibrasi (dibagi ");
    Serial.print(knownWeight, 0);
    Serial.print(" g): ");
    Serial.println(calValue, 6);
    Serial.println("----------------------------------------------");
    Serial.println("Selesai.");

    // Tampilan hasil di LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" KALIBRASI SELESAI ");
    lcd.setCursor(0, 1);
    lcd.print("Avg: ");
    lcd.print(avg, 0);
    lcd.setCursor(0, 2);
    lcd.print("Cal Factor: ");
    lcd.setCursor(0, 3);
    lcd.print(calValue, 6);

    done = true;
  }
}