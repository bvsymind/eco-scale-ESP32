/*
 * PROGRAM PEMBACAAN REAL-TIME HX711
 * Baca sensor secepat mungkin, tampilkan ke Serial Monitor setiap 500 ms.
 * Menampilkan berat dalam kilogram (4 angka di belakang koma).
 */

#include <Arduino.h>
#include "HX711_ADC.h"

const int HX711_dout = 2;
const int HX711_sck  = 4;

// Ganti dengan hasil kalibrasi kamu
const float calibrationFactor = 12.244260; // contoh nilai, ubah sesuai hasilmu

HX711_ADC LoadCell(HX711_dout, HX711_sck);

unsigned long lastPrint = 0;
const unsigned long printInterval = 500; // tampilkan setiap 500 ms

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("==============================================");
  Serial.println("   Pembacaan Real-Time HX711 (update cepat)");
  Serial.println("==============================================");

  LoadCell.begin();
  Serial.println("Menyiapkan load cell...");

  unsigned long stabilizingtime = 2000;
  boolean _tare = true;
  LoadCell.start(stabilizingtime, _tare);

  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("❌ Timeout! Periksa koneksi HX711.");
    while (1);
  }

  LoadCell.setCalFactor(calibrationFactor);
  Serial.print("Faktor kalibrasi diterapkan: ");
  Serial.println(calibrationFactor, 6);
  Serial.println("----------------------------------------------");
}

void loop() {
  // 🟢 Baca sensor SECEPAT MUNGKIN
  LoadCell.update();

  // 🔵 Cetak ke serial setiap 500 ms
  unsigned long currentMillis = millis();
  if (currentMillis - lastPrint >= printInterval) {
    lastPrint = currentMillis;

    // Ambil nilai berat dalam gram, ubah ke kilogram
    float weight = LoadCell.getData() / 1000.0;
    Serial.print("Berat: ");
    Serial.print(weight, 4);
    Serial.println(" kg");
  }
}
