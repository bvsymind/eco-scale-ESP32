#ifndef NETWORK_HANDLER_H
#define NETWORK_HANDLER_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <ESP32Ping.h>
#include <LiquidCrystal_I2C.h>
#include <esp_task_wdt.h>

#include "Config.h"
#include "Types.h"
#include "credentials.h" // Pastikan file ini berisi WIFI_SSID, WIFI_PASSWORD, API_KEY

// ==================== KONSTANTA SERVER ====================
extern const char* SERVER_URL;
extern const char* MQTT_SERVER;
extern const int MQTT_PORT;
extern const char* MQTT_TOPIC;
extern const char* PING_HOST;

// ==================== FUNGSI NETWORK ====================

// Menghubungkan ke WiFi. 
// Parameter 'lcd' opsional (bisa nullptr jika tidak ingin update layar)
bool connectWiFi(LiquidCrystal_I2C* lcd = nullptr);

// Menghubungkan ke Broker MQTT
void connectMQTT(PubSubClient& client);

// Mengecek apakah internet benar-benar hidup (ping google)
bool checkNetworkHealth();

// Mengelola koneksi WiFi (reconnect jika putus)
// Memerlukan akses ke SystemState untuk update flag offlineMode
void manageWiFiConnection(SystemState& state, unsigned long& lastWifiCheckTime);

// Mengirim data ke Laravel
bool sendToLaravel(const SystemState& state);

// Mengirim data ke MQTT
bool sendToMQTT(PubSubClient& client, const SystemState& state);

#endif