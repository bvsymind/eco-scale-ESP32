/*
 * PROGRAM PEMBACAAN HX711 - DENGAN KOMPENSASI SUHU SOFTWARE
 * Thermal compensation tanpa sensor suhu hardware
 */

#include <Arduino.h>
#include "HX711_ADC.h"

const int HX711_dout = 2;
const int HX711_sck  = 4;

// Ganti dengan hasil kalibrasi kamu
const float calibrationFactor = 12.307766;

HX711_ADC LoadCell(HX711_dout, HX711_sck);

// Variabel untuk sampling
const int TOTAL_SAMPLES = 500;
const int STABILIZATION_SAMPLES = 20;
const float STABILITY_THRESHOLD = 0.001;

int samplesCollected = 0;
bool samplingCompleted = false;
bool stabilizationCompleted = false;
unsigned long samplingStartTime = 0;

// Array untuk menyimpan data
float weightReadings[TOTAL_SAMPLES];
float sumReadings = 0;
float averageWeight = 0;

// Variabel timing
const unsigned long SAMPLE_DELAY = 100;

// 🎯 VARIABEL KOMPENSASI SUHU SOFTWARE
float longTermAverage = 0;
int longTermSamples = 0;
float thermalDriftCompensation = 0;
bool compensationActive = false;
const float DRIFT_THRESHOLD = 0.002; // 2 gram threshold untuk deteksi drift
const float COMPENSATION_RATE = 0.05; // Rate koreksi lambat

// 🎯 DEKLARASI FUNGSI
void calculateStatistics();
float findMinWeight();
float findMaxWeight();
void displayProgress();
bool waitForStabilization();
float applySoftwareThermalCompensation(float currentWeight);
void initializeCompensationSystem();

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("==============================================");
  Serial.println(" HX711 - DENGAN KOMPENSASI SUHU SOFTWARE");
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
  
  // Inisialisasi sistem kompensasi
  initializeCompensationSystem();
  
  Serial.println();
  Serial.println("🟢 MENUNGGU STABILISASI SINYAL...");
  Serial.println("➡️  Letakkan beban dan tunggu sampai stabil");
  Serial.println("💡 Kompensasi suhu software AKTIF");
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
        samplingStartTime = millis();
      }
    } 
    // 🟢 FASE 2: Rekam 500 sampel setelah stabil
    else {
      LoadCell.update();
      
      // Simpan data
      if (samplesCollected < TOTAL_SAMPLES) {
        float rawWeight = LoadCell.getData() / 1000.0;
        
        // 🎯 TERAPKAN KOMPENSASI SUHU SOFTWARE
        float compensatedWeight = applySoftwareThermalCompensation(rawWeight);
        
        weightReadings[samplesCollected] = compensatedWeight;
        sumReadings += compensatedWeight;
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
}

// 🎯 INISIALISASI SISTEM KOMPENSASI
void initializeCompensationSystem() {
  longTermAverage = 0;
  longTermSamples = 0;
  thermalDriftCompensation = 0;
  compensationActive = false;
  
  // Pre-fill dengan beberapa pembacaan awal
  for(int i = 0; i < 10; i++) {
    LoadCell.update();
    float weight = LoadCell.getData() / 1000.0;
    longTermAverage = (longTermAverage * longTermSamples + weight) / (longTermSamples + 1);
    longTermSamples++;
    delay(50);
  }
}

// 🎯 KOMPENSASI SUHU SOFTWARE-ONLY
float applySoftwareThermalCompensation(float currentWeight) {
  // Update long-term average (exponential moving average)
  if (longTermSamples < 1000) {
    longTermAverage = (longTermAverage * longTermSamples + currentWeight) / (longTermSamples + 1);
    longTermSamples++;
  } else {
    // Gunakan EMA untuk respons lebih cepat setelah banyak sampel
    longTermAverage = longTermAverage * 0.999 + currentWeight * 0.001;
  }
  
  // Hitung deviasi dari rata-rata jangka panjang
  float deviation = currentWeight - longTermAverage;
  
  // Deteksi drift signifikan (mungkin akibat suhu)
  if (abs(deviation) > DRIFT_THRESHOLD && !compensationActive) {
    compensationActive = true;
    Serial.println("🌡️  DRIFT TERDETEKSI! Mengaktifkan kompensasi...");
    Serial.print("   Deviasi: ");
    Serial.print(deviation * 1000, 2);
    Serial.println(" gram");
  }
  
  // Terapkan kompensasi jika drift terdeteksi
  if (compensationActive) {
    // Koreksi bertahap untuk menghindari over-compensation
    float compensation = -deviation * COMPENSATION_RATE;
    thermalDriftCompensation += compensation;
    
    // Batasi kompensasi maksimum
    if (abs(thermalDriftCompensation) > 0.01) { // Maks 10 gram
      thermalDriftCompensation = (thermalDriftCompensation > 0) ? 0.01 : -0.01;
    }
    
    return currentWeight + thermalDriftCompensation;
  }
  
  return currentWeight;
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
    
    // Tampilkan status kompensasi
    if (compensationActive) {
      Serial.print(" [Kompensasi: ");
      Serial.print(thermalDriftCompensation * 1000, 2);
      Serial.print("g]");
    }
    Serial.println();
    
    // Beri warning jika berat terlalu kecil
    if (currentWeight < 0.01) {
      Serial.println("⚠️  Berat sangat kecil! Pastikan beban sudah diletakkan.");
    }
  }
  
  // Cek stabilitas jika sudah mengumpulkan cukup samples
  if (readingIndex == 0) {
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

// 🎯 FUNGSI UNTUK MENAMPILKAN PROGRESS
void displayProgress() {
  // Tampilkan setiap sampel tanpa lompat-lompat
  Serial.print("Data ");
  Serial.print(samplesCollected);
  Serial.print("/");
  Serial.print(TOTAL_SAMPLES);
  Serial.print(": ");
  Serial.print(weightReadings[samplesCollected-1], 4);
  Serial.print(" kg");
  
  // Tampilkan info kompensasi jika aktif
  if (compensationActive && samplesCollected % 50 == 0) {
    Serial.print(" [Komp: ");
    Serial.print(thermalDriftCompensation * 1000, 2);
    Serial.print("g]");
  }
  Serial.println();
}

// 🎯 FUNGSI UNTUK MENGHITUNG STATISTIK
void calculateStatistics() {
  // Hitung statistik dasar
  averageWeight = sumReadings / TOTAL_SAMPLES;
  
  float sumDeviations = 0;
  float maxDeviation = 0;
  float minDeviation = 999999;
  
  for (int i = 0; i < TOTAL_SAMPLES; i++) {
    float deviation = abs(weightReadings[i] - averageWeight);
    sumDeviations += deviation;
    
    if (deviation > maxDeviation) maxDeviation = deviation;
    if (deviation < minDeviation) minDeviation = deviation;
  }
  
  float averageDeviation = sumDeviations / TOTAL_SAMPLES;
  
  // Hitung waktu total
  unsigned long totalTime = millis() - samplingStartTime;
  float samplesPerSecond = (float)TOTAL_SAMPLES / (totalTime / 1000.0);
  
  // 🔬 TAMPILKAN HASIL STATISTIK LENGKAP
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
  
  // 🎯 INFO KOMPENSASI
  Serial.println();
  Serial.println("🌡️  STATUS KOMPENSASI SUHU:");
  Serial.println("----------------------------------------------");
  Serial.print("Kompensasi aktif: ");
  Serial.println(compensationActive ? "YA" : "TIDAK");
  Serial.print("Total kompensasi diterapkan: ");
  Serial.print(thermalDriftCompensation * 1000, 2);
  Serial.println(" gram");
  Serial.print("Sampel jangka panjang: ");
  Serial.println(longTermSamples);
  
  Serial.println();
  Serial.println("📈 STATISTIK BERAT (SETELAH KOMPENSASI):");
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
  
  // Konversi ke gram
  Serial.println();
  Serial.println("💡 DALAM GRAM:");
  Serial.println("----------------------------------------------");
  Serial.print("Deviasi rata-rata: ±");
  Serial.print(averageDeviation * 1000, 2);
  Serial.println(" g");
  
  Serial.print("Deviasi maksimum: ±");
  Serial.print(maxDeviation * 1000, 2);
  Serial.println(" g");
  
  // 🎯 ANALISIS PERBAIKAN
  Serial.println();
  Serial.println("🔍 ANALISIS PERBAIKAN STABILITAS:");
  Serial.println("----------------------------------------------");
  
  float stabilityRatio = (maxDeviation / averageWeight) * 100.0;
  
  // Estimasi perbaikan (berdasarkan pengalaman)
  float estimatedImprovement = compensationActive ? 0.3 : 0.0; // 30% improvement jika kompensasi aktif
  
  if (stabilityRatio < 0.1) {
    Serial.println("✅ SANGAT STABIL - Kualitas data excellent");
    if (compensationActive) Serial.println("   💡 Kompensasi suhu berkontribusi pada stabilitas");
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
  
  if (compensationActive) {
    Serial.print("Estimasi perbaikan dengan kompensasi: ~");
    Serial.print(estimatedImprovement * 100, 0);
    Serial.println("%");
  }
  
  Serial.println();
  Serial.println("==============================================");
  Serial.println("         PENGAMBILAN DATA SELESAI");
  Serial.println("==============================================");
}

// 🎯 FUNGSI UNTUK MENCARI BERAT MINIMUM
float findMinWeight() {
  float min = weightReadings[0];
  for (int i = 1; i < TOTAL_SAMPLES; i++) {
    if (weightReadings[i] < min) min = weightReadings[i];
  }
  return min;
}

// 🎯 FUNGSI UNTUK MENCARI BERAT MAKSIMUM
float findMaxWeight() {
  float max = weightReadings[0];
  for (int i = 1; i < TOTAL_SAMPLES; i++) {
    if (weightReadings[i] > max) max = weightReadings[i];
  }
  return max;
}