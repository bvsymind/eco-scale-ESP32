#include "DisplayHandler.h"
#include <WiFi.h>

// [PERUBAHAN]: Include library HANYA DI SINI
#define USE_SERIAL_2004_LCD 
#include "LCDBigNumbers.hpp"

// ==================== GLOBAL PRIVATE VARIABLE ====================
// Kita gunakan pointer agar main.cpp tidak perlu tahu objek ini ada
LCDBigNumbers* internalBigNumbers = nullptr;

// ... (Definisi Bitmap Ikon TETAP SAMA seperti sebelumnya) ...
enum IconIndex : uint8_t { ICON_SIGNAL_1 = 0, ICON_SIGNAL_2 = 1, ICON_SIGNAL_3 = 2, ICON_SIGNAL_4 = 3, ICON_NO_INTERNET = 4 };
const byte WIFI_SIGNAL_1[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18};
const byte WIFI_SIGNAL_2[] = {0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x1B, 0x1B};
const byte WIFI_SIGNAL_3[] = {0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18};
const byte WIFI_SIGNAL_4[] = {0x03, 0x03, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B};
const byte NO_INTERNET_ICON[] = {0x14, 0x08, 0x14, 0x00, 0x00, 0x00, 0x18, 0x18};

// ==================== IMPLEMENTASI FUNGSI ====================

void initializeLCD(LiquidCrystal_I2C& lcd) {
  lcd.init();
  lcd.backlight();
  lcd.clear();
  
  // Create Custom Characters
  lcd.createChar(static_cast<uint8_t>(ICON_SIGNAL_1), (uint8_t*)WIFI_SIGNAL_1);
  lcd.createChar(static_cast<uint8_t>(ICON_SIGNAL_2), (uint8_t*)WIFI_SIGNAL_2);
  lcd.createChar(static_cast<uint8_t>(ICON_SIGNAL_3), (uint8_t*)WIFI_SIGNAL_3);
  lcd.createChar(static_cast<uint8_t>(ICON_SIGNAL_4), (uint8_t*)WIFI_SIGNAL_4);
  lcd.createChar(static_cast<uint8_t>(ICON_NO_INTERNET), (uint8_t*)NO_INTERNET_ICON);
  
  // [PERUBAHAN]: Inisialisasi BigNumbers di sini menggunakan pointer
  if (internalBigNumbers == nullptr) {
    internalBigNumbers = new LCDBigNumbers(&lcd, BIG_NUMBERS_FONT_2_COLUMN_3_ROWS_VARIANT_2);
    internalBigNumbers->begin();
  }
}

void updateWeightDisplay(float weight) {
  if (internalBigNumbers == nullptr) return; // Safety check

  char buffer[10];
  snprintf(buffer, sizeof(buffer), "%6.2f", weight);
  
  internalBigNumbers->setBigNumberCursor(1, 1);
  internalBigNumbers->print(buffer);
}

void restoreDefaultDisplay(LiquidCrystal_I2C& lcd, const SystemState& state) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Jenis: " + state.waste.getDisplayName());
  lcd.setCursor(17, 1);
  lcd.print("kg");
}

void showSubtypeSelection(LiquidCrystal_I2C& lcd) {
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Pilih Sub-jenis:");
  lcd.setCursor(0, 1);
  lcd.print(" 1.Umum     2.Botol");
  lcd.setCursor(0, 2);
  lcd.print(" 3.Kertas");
}

void updateStatusIndicators(LiquidCrystal_I2C& lcd, bool isOffline, bool mqttConnected) {
  if (WiFi.status() == WL_CONNECTED && !isOffline) {
    char rssi[5];
    snprintf(rssi, sizeof(rssi), "%3ld", WiFi.RSSI());
    lcd.setCursor(17, 3);
    lcd.print(rssi);
  } else {
    lcd.setCursor(17, 3);
    lcd.print("OFF");
  }
  
  lcd.setCursor(0, 3);
  if (isOffline) {
    lcd.write(static_cast<uint8_t>(ICON_NO_INTERNET));
  } else if (mqttConnected) {
    lcd.print(" ");
  } else {
    static bool blink = false;
    lcd.print(blink ? "-" : " ");
    blink = !blink;
  }
}

void showStatusMessage(LiquidCrystal_I2C& lcd, const char* msg) {
  lcd.setCursor(0, 0);
  lcd.print(msg);
  for (int i = strlen(msg); i < 20; i++) lcd.print(" ");
}