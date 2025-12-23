#ifndef DISPLAY_HANDLER_H
#define DISPLAY_HANDLER_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "Config.h"
#include "Types.h"

// [PERUBAHAN]: Hapus include LCDBigNumbers.hpp dari sini!
// [PERUBAHAN]: Hapus parameter LCDBigNumbers& dari fungsi-fungsi di bawah

// ==================== FUNGSI DISPLAY ====================

// Inisialisasi LCD dan internal BigNumbers
void initializeLCD(LiquidCrystal_I2C& lcd);

// Menampilkan angka berat (BigNumbers dikelola secara internal di .cpp)
void updateWeightDisplay(float weight);

// Kembali ke tampilan default
void restoreDefaultDisplay(LiquidCrystal_I2C& lcd, const SystemState& state);

// Menu pilihan sub-jenis
void showSubtypeSelection(LiquidCrystal_I2C& lcd);

// Update indikator (WiFi, MQTT, dll)
void updateStatusIndicators(LiquidCrystal_I2C& lcd, bool isOffline, bool mqttConnected);

// Tampilkan pesan status
void showStatusMessage(LiquidCrystal_I2C& lcd, const char* msg);

#endif