#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==================== CONFIGURATION ====================
namespace Config {
  // Timing Constants
  constexpr unsigned long WEIGHT_READ_INTERVAL = 100;
  constexpr unsigned long LCD_UPDATE_INTERVAL = 150;
  constexpr unsigned long WIFI_CHECK_INTERVAL = 15000;
  constexpr unsigned long MQTT_RETRY_INTERVAL = 5000;
  constexpr unsigned long PING_CHECK_INTERVAL = 10000;
  constexpr unsigned long STATUS_MSG_DURATION = 2000;
  constexpr unsigned long STATUS_DISPLAY_INTERVAL = 1000;
  
  // Hardware Configuration
  constexpr float CALIBRATION_VALUE = 12.487849f;
  constexpr float MIN_WEIGHT_THRESHOLD = 0.01f;
  constexpr float NOISE_GATE_THRESHOLD = 0.01f;
  constexpr int LOAD_CELL_SAMPLES = 16;
  
  // Pin Configuration
  constexpr int PIN_TOMBOL_1 = 27;
  constexpr int PIN_TOMBOL_2 = 26;
  constexpr int PIN_TOMBOL_3 = 25;
  constexpr int PIN_TOMBOL_4 = 33;
  constexpr int PIN_BUZZER = 5;
  constexpr int HX711_DOUT = 2;
  constexpr int HX711_SCK = 4;
  
  // Network Configuration
  constexpr int WIFI_RETRY_COUNT = 20;
  constexpr int HTTP_TIMEOUT = 15000;
}

#endif