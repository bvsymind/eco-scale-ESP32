#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <ezButton.h>
#include <HX711_ADC.h>
#include <EEPROM.h>
#include <esp_task_wdt.h>

// Include Modular Files
#include "credentials.h"
#include "Config.h"
#include "Types.h"
#include "DisplayHandler.h"
#include "NetworkHandler.h"

// ==================== GLOBAL OBJECTS ====================
// Inisialisasi LCD dan BigNumbers
LiquidCrystal_I2C lcd(0x27, 20, 4); 

// Inisialisasi Load Cell & Network Client
HX711_ADC loadCell(Config::HX711_DOUT, Config::HX711_SCK);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Inisialisasi Tombol
ezButton buttons[] = {
  ezButton(Config::PIN_TOMBOL_1),
  ezButton(Config::PIN_TOMBOL_2),
  ezButton(Config::PIN_TOMBOL_3),
  ezButton(Config::PIN_TOMBOL_4)
};

// Global State
SystemState state;

// Timer struct (Local untuk Main Loop)
struct Timers {
  unsigned long lastWeightRead = 0;
  unsigned long lastLCDUpdate = 0;
  unsigned long lastWifiCheck = 0;
  unsigned long lastMqttRetry = 0;
  unsigned long lastStatusDisplay = 0;
  unsigned long statusMsgTimestamp = 0;
} timers;

// ==================== LOCAL FUNCTION DECLARATIONS ====================
void processButtons();
void handleSendData();
float readSmoothedWeight();
void playTone(uint16_t freq, uint16_t duration);

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== EcoScale Modular Firmware ===");
  
  // 1. Init WDT
  esp_task_wdt_init(60, true);
  esp_task_wdt_add(NULL);
  
  // 2. Init Hardware
  pinMode(Config::PIN_BUZZER, OUTPUT);
  
  // Panggil fungsi dari DisplayHandler
  initializeLCD(lcd);
  
  // Init LoadCell
  loadCell.begin();
  EEPROM.begin(512);
  loadCell.start(2000, true);
  if (loadCell.getTareTimeoutFlag()) {
    lcd.clear();
    lcd.print("HX711 Error!");
    while (true) { esp_task_wdt_reset(); delay(100); }
  }
  loadCell.setCalFactor(Config::CALIBRATION_VALUE);
  loadCell.setSamplesInUse(Config::LOAD_CELL_SAMPLES);

  // 3. Init Network (Panggil dari NetworkHandler)
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  
  // Pass address lcd untuk menampilkan status koneksi
  if (!connectWiFi(&lcd)) {
    state.offlineMode = true;
    showStatusMessage(lcd, "WiFi Gagal!");
    playTone(500, 1000);
  } else {
    connectMQTT(mqttClient);
    showStatusMessage(lcd, "System Ready");
  }
  
  delay(2000);
  
  // Tampilkan layar utama
  restoreDefaultDisplay(lcd, state);
  updateWeightDisplay(0.0f);
  
  timers.lastWeightRead = millis();
  timers.lastLCDUpdate = millis();
}

// ==================== MAIN LOOP ====================
void loop() {
  esp_task_wdt_reset();
  
  // 1. Update Buttons
  for (auto& btn : buttons) btn.loop();
  
  // 2. Network Maintenance (Panggil dari NetworkHandler)
  manageWiFiConnection(state, timers.lastWifiCheck);
  
  if (!state.offlineMode) {
    if (!mqttClient.connected() && millis() - timers.lastMqttRetry > Config::MQTT_RETRY_INTERVAL) {
      connectMQTT(mqttClient);
      timers.lastMqttRetry = millis();
    }
    mqttClient.loop();
  }
  
  // 3. State Machine Logic
  switch (state.appState) {
    case AppState::IDLE: {
      unsigned long now = millis();
      
      // Update Load Cell
      if (loadCell.update()) {
        state.newDataReady = true;
      }
      
      // Read Weight
      if (state.newDataReady && now - timers.lastWeightRead >= Config::WEIGHT_READ_INTERVAL) {
        state.currentWeight = readSmoothedWeight();
        timers.lastWeightRead = now;
        state.newDataReady = false;
      }
      
      // Update Display Weight
      if (now - timers.lastLCDUpdate >= Config::LCD_UPDATE_INTERVAL) {
        if (abs(state.currentWeight - state.lastDisplayedWeight) > Config::MIN_WEIGHT_THRESHOLD ||
            state.lastDisplayedWeight < 0) {
          updateWeightDisplay(state.currentWeight);
          state.lastDisplayedWeight = state.currentWeight;
        }
        timers.lastLCDUpdate = now;
      }
      
      // Update Status Icons (WiFi/MQTT) di baris bawah
      if (now - timers.lastStatusDisplay >= Config::STATUS_DISPLAY_INTERVAL) {
        updateStatusIndicators(lcd, state.offlineMode, mqttClient.connected());
        timers.lastStatusDisplay = now;
      }

      processButtons();
      handleSendData();
      break;
    }
    
    case AppState::SELECTING_SUBTYPE:
      processButtons();
      break;
      
    case AppState::SENDING_DATA:
      // Menunggu proses kirim (blocking sebentar di handleSendData)
      break;
      
    case AppState::SHOWING_STATUS:
      // Kembali ke tampilan awal setelah durasi tertentu
      if (millis() - timers.statusMsgTimestamp > Config::STATUS_MSG_DURATION) {
        restoreDefaultDisplay(lcd, state);
        state.appState = AppState::IDLE;
      }
      break;
  }
}

// ==================== LOCAL HELPER FUNCTIONS ====================

void processButtons() {
  // Tombol 1: Organik / Anorganik (Umum)
  if (buttons[0].isPressed()) {
    if (state.appState == AppState::IDLE) {
      state.waste.setType("Organik");
      playTone(2500, 100);
      restoreDefaultDisplay(lcd, state);
    } else if (state.appState == AppState::SELECTING_SUBTYPE) {
      state.waste.setType("Anorganik", "Umum");
      playTone(2500, 100);
      restoreDefaultDisplay(lcd, state);
      state.appState = AppState::IDLE;
    }
  } 
  // Tombol 2: Menu Subtype / Botol
  else if (buttons[1].isPressed()) {
    if (state.appState == AppState::IDLE) {
      state.appState = AppState::SELECTING_SUBTYPE;
      showSubtypeSelection(lcd);
    } else if (state.appState == AppState::SELECTING_SUBTYPE) {
      state.waste.setType("Anorganik", "Botol");
      playTone(2500, 100);
      restoreDefaultDisplay(lcd, state);
      state.appState = AppState::IDLE;
    }
  } 
  // Tombol 3: Residu / Kertas
  else if (buttons[2].isPressed()) {
    if (state.appState == AppState::IDLE) {
      state.waste.setType("Residu");
      playTone(2500, 100);
      restoreDefaultDisplay(lcd, state);
    } else if (state.appState == AppState::SELECTING_SUBTYPE) {
      state.waste.setType("Anorganik", "Kertas");
      playTone(2500, 100);
      restoreDefaultDisplay(lcd, state);
      state.appState = AppState::IDLE;
    }
  }
}

void handleSendData() {
  // Tombol 4: Kirim Data
  if (state.appState != AppState::IDLE || !buttons[3].isPressed()) return;
  
  playTone(2000, 100);
  
  if (state.offlineMode) {
    showStatusMessage(lcd, "Gagal: Offline!");
    timers.statusMsgTimestamp = millis(); // Set timer agar pesan hilang otomatis
    return;
  }
  
  if (strcmp(state.waste.jenis, "--") == 0) {
    showStatusMessage(lcd, "Error: Pilih Jenis!");
    timers.statusMsgTimestamp = millis();
    return;
  }
  
  state.appState = AppState::SENDING_DATA;
  lcd.setCursor(0, 0);
  lcd.print("Status: Mengirim... ");
  
  // Panggil fungsi dari NetworkHandler
  bool laravelOk = sendToLaravel(state);
  sendToMQTT(mqttClient, state);
  
  if (laravelOk) {
    showStatusMessage(lcd, "Status: Sukses!");
    state.waste.reset(); // Reset jenis sampah jika sukses
  } else {
    showStatusMessage(lcd, "Status: Gagal!");
  }
  
  timers.statusMsgTimestamp = millis(); // Mulai hitung mundur untuk menghilangkan pesan
  state.appState = AppState::SHOWING_STATUS;
}

float readSmoothedWeight() {
  float rawWeight = loadCell.getData();
  float weightKg = rawWeight / 1000.0f;
  
  // Noise gate
  if (abs(weightKg) < Config::NOISE_GATE_THRESHOLD) {
    weightKg = 0.0f;
  }
  
  return weightKg;
}

void playTone(uint16_t freq, uint16_t duration) {
  tone(Config::PIN_BUZZER, freq, duration);
}