/*
 * PROGRAM PEMBACAAN HX711 - 500 SAMPEL DENGAN STABILISASI AWAL
 * Tunggu sampai stabil, lalu baca 500 sampel dan hitung statistik
 */

#include <Arduino.h>
#include "HX711_ADC.h"

const int HX711_dout = 2;
const int HX711_sck  = 4;

// Ganti dengan hasil kalibrasi kamu
const float calibrationFactor = 12.307766; // contoh nilai, ubah sesuai hasilmu

HX711_ADC LoadCell(HX711_dout, HX711_sck);

// Variabel untuk sampling
const int TOTAL_SAMPLES = 500;
const int STABILIZATION_SAMPLES = 20;
const float STABILITY_THRESHOLD = 0.001; // 1 gram threshold untuk stabilitas

int samplesCollected = 0;
bool samplingCompleted = false;
bool stabilizationCompleted = false;
unsigned long samplingStartTime = 0;

// Array untuk menyimpan data
float weightReadings[TOTAL_SAMPLES];
float sumReadings = 0;
float averageWeight = 0;

// Variabel timing
const unsigned long SAMPLE_DELAY = 100;   // delay 100ms antara sampel

// 🎯 DEKLARASI FUNGSI (untuk PlatformIO)
void calculateStatistics();
float findMinWeight();
float findMaxWeight();
void displayProgress();
bool waitForStabilization();

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("==============================================");
  Serial.println("   PEMBACAAN HX711 - DENGAN STABILISASI");
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
  Serial.println();
  Serial.println("🟢 MENUNGGU STABILISASI SINYAL...");
  Serial.println("➡️  Letakkan beban dan tunggu sampai stabil");
  Serial.println("----------------------------------------------");
  
  samplingStartTime = millis();
}

void loop() {
  if (!samplingCompleted) {
    // 🟢 FASE 1: Tunggu sampai sinyal stabil
    if (!stabilizationCompleted) {
      stabilizationCompleted = waitForStabilization();
      if (stabilizationCompleted) {
        Serial.println("✅ SINYAL SUDAH STABIL! Mulai merekam 500 sampel...");
        Serial.println("----------------------------------------------");
        samplingStartTime = millis(); // Reset waktu mulai
      }
    } 
    // 🟢 FASE 2: Rekam 500 sampel setelah stabil
    else {
      LoadCell.update();
      
      // Simpan data
      if (samplesCollected < TOTAL_SAMPLES) {
        float weight = LoadCell.getData() / 1000.0; // Convert to kg
        weightReadings[samplesCollected] = weight;
        sumReadings += weight;
        samplesCollected++;
        
        // 🔵 Tampilkan progress untuk SETIAP sampel
        displayProgress();
        
        // 🕒 Delay 100ms antara sampel
        delay(SAMPLE_DELAY);
      }
      
      // ✅ Cek apakah sudah selesai
      if (samplesCollected >= TOTAL_SAMPLES) {
        samplingCompleted = true;
        calculateStatistics();
      }
    }
  }
  // else: Program selesai, tidak melakukan apa-apa
}

// 🎯 FUNGSI UNTUK MENUNGGU STABILISASI
bool waitForStabilization() {
  static float lastReadings[STABILIZATION_SAMPLES];
  static int readingIndex = 0;
  static unsigned long lastStabPrint = 0;
  
  LoadCell.update();
  float currentWeight = LoadCell.getData() / 1000.0;
  
  // Simpan reading terbaru
  lastReadings[readingIndex] = currentWeight;
  readingIndex = (readingIndex + 1) % STABILIZATION_SAMPLES;
  
  // Tampilkan progress stabilisasi setiap 500ms
  unsigned long currentMillis = millis();
  if (currentMillis - lastStabPrint >= 500) {
    lastStabPrint = currentMillis;
    Serial.print("Stabilisasi - Pembacaan terkini: ");
    Serial.print(currentWeight, 4);
    Serial.println(" kg");
    
    // Beri warning jika berat terlalu kecil
    if (currentWeight < 0.01) {
      Serial.println("⚠️  Berat sangat kecil! Pastikan beban sudah diletakkan.");
    }
  }
  
  // Cek stabilitas jika sudah mengumpulkan cukup samples
  if (readingIndex == 0) {
    // Hitung rata-rata dan deviasi
    float sum = 0;
    for (int i = 0; i < STABILIZATION_SAMPLES; i++) {
      sum += lastReadings[i];
    }
    float average = sum / STABILIZATION_SAMPLES;
    
    float maxDev = 0;
    for (int i = 0; i < STABILIZATION_SAMPLES; i++) {
      float dev = abs(lastReadings[i] - average);
      if (dev > maxDev) maxDev = dev;
    }
    
    // Jika deviasi maksimum di bawah threshold, dianggap stabil
    if (maxDev <= STABILITY_THRESHOLD && average > 0.01) {
      Serial.println();
      Serial.print("✅ STABIL! Deviasi maksimum: ");
      Serial.print(maxDev * 1000, 2);
      Serial.println(" gram");
      return true;
    } else if (maxDev <= STABILITY_THRESHOLD) {
      Serial.println("❌ Sinyal stabil tapi berat terlalu kecil - tunggu beban...");
    }
  }
  
  delay(SAMPLE_DELAY);
  return false;
}

// 🎯 FUNGSI UNTUK MENAMPILKAN PROGRESS (SETIAP SAMPEL)
void displayProgress() {
  // Tampilkan setiap sampel tanpa lompat-lompat
  Serial.print("Data ");
  Serial.print(samplesCollected);
  Serial.print("/");
  Serial.print(TOTAL_SAMPLES);
  Serial.print(": ");
  Serial.print(weightReadings[samplesCollected-1], 4);
  Serial.println(" kg");
}

// 🎯 FUNGSI UNTUK MENGHITUNG STATISTIK
void calculateStatistics() {
  // Hitung rata-rata
  averageWeight = sumReadings / TOTAL_SAMPLES;
  
  // Hitung deviasi
  float sumDeviations = 0;
  float maxDeviation = 0;
  float minDeviation = 999999;
  
  for (int i = 0; i < TOTAL_SAMPLES; i++) {
    float deviation = abs(weightReadings[i] - averageWeight);
    sumDeviations += deviation;
    
    if (deviation > maxDeviation) {
      maxDeviation = deviation;
    }
    if (deviation < minDeviation) {
      minDeviation = deviation;
    }
  }
  
  float averageDeviation = sumDeviations / TOTAL_SAMPLES;
  
  // Hitung waktu total
  unsigned long totalTime = millis() - samplingStartTime;
  float samplesPerSecond = (float)TOTAL_SAMPLES / (totalTime / 1000.0);
  
  // 🔬 TAMPILKAN HASIL STATISTIK
  Serial.println();
  Serial.println("==============================================");
  Serial.println("           HASIL STATISTIK 500 SAMPEL");
  Serial.println("==============================================");
  
  Serial.print("⏱️  Waktu pengambilan: ");
  Serial.print(totalTime / 1000.0, 2);
  Serial.println(" detik");
  
  Serial.print("📊 Kecepatan sampling: ");
  Serial.print(samplesPerSecond, 1);
  Serial.println(" sampel/detik");
  
  Serial.println();
  Serial.println("📈 STATISTIK BERAT:");
  Serial.println("----------------------------------------------");
  Serial.print("Rata-rata berat: ");
  Serial.print(averageWeight, 4);
  Serial.println(" kg");
  
  Serial.print("Berat minimum: ");
  float minWeight = findMinWeight();
  Serial.print(minWeight, 4);
  Serial.println(" kg");
  
  Serial.print("Berat maksimum: ");
  float maxWeight = findMaxWeight();
  Serial.print(maxWeight, 4);
  Serial.println(" kg");
  
  Serial.print("Range: ");
  Serial.print(maxWeight - minWeight, 4);
  Serial.println(" kg");
  
  Serial.println();
  Serial.println("📊 STATISTIK DEVIASI:");
  Serial.println("----------------------------------------------");
  Serial.print("Deviasi rata-rata: ±");
  Serial.print(averageDeviation, 4);
  Serial.println(" kg");
  
  Serial.print("Deviasi maksimum: ±");
  Serial.print(maxDeviation, 4);
  Serial.println(" kg");
  
  Serial.print("Deviasi minimum: ±");
  Serial.print(minDeviation, 4);
  Serial.println(" kg");
  
  // Konversi ke gram untuk perspektif yang lebih baik
  Serial.println();
  Serial.println("💡 DALAM GRAM:");
  Serial.println("----------------------------------------------");
  Serial.print("Deviasi rata-rata: ±");
  Serial.print(averageDeviation * 1000, 2);
  Serial.println(" g");
  
  Serial.print("Deviasi maksimum: ±");
  Serial.print(maxDeviation * 1000, 2);
  Serial.println(" g");
  
  // Evaluasi kualitas data
  Serial.println();
  Serial.println("🔍 EVALUASI STABILITAS:");
  Serial.println("----------------------------------------------");
  
  float stabilityRatio = (maxDeviation / averageWeight) * 100.0;
  
  if (stabilityRatio < 0.1) {
    Serial.println("✅ SANGAT STABIL - Kualitas data excellent");
  } else if (stabilityRatio < 0.5) {
    Serial.println("✅ STABIL - Kualitas data baik");
  } else if (stabilityRatio < 1.0) {
    Serial.println("⚠️  CUKUP STABIL - Kualitas data acceptable");
  } else {
    Serial.println("❌ KURANG STABIL - Periksa setup hardware");
  }
  
  Serial.print("Stability index: ");
  Serial.print(stabilityRatio, 3);
  Serial.println("%");
  
  Serial.println();
  Serial.println("==============================================");
  Serial.println("         PENGAMBILAN DATA SELESAI");
  Serial.println("==============================================");
}

// 🎯 FUNGSI UNTUK MENCARI BERAT MINIMUM
float findMinWeight() {
  float min = weightReadings[0];
  for (int i = 1; i < TOTAL_SAMPLES; i++) {
    if (weightReadings[i] < min) {
      min = weightReadings[i];
    }
  }
  return min;
}

// 🎯 FUNGSI UNTUK MENCARI BERAT MAKSIMUM
float findMaxWeight() {
  float max = weightReadings[0];
  for (int i = 1; i < TOTAL_SAMPLES; i++) {
    if (weightReadings[i] > max) {
      max = weightReadings[i];
    }
  }
  return max;
}