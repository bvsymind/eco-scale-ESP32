#include "NetworkHandler.h"

// ==================== DEFINISI KONSTANTA ====================
const char* SERVER_URL = "https://ecoscale.undip.us/api/receive-sampah";
const char* MQTT_SERVER = "broker.hivemq.com";
const int MQTT_PORT = 1883;
const char* MQTT_TOPIC = "undip/scale/new";
const char* PING_HOST = "8.8.8.8";

// ==================== IMPLEMENTASI FUNGSI ====================

bool connectWiFi(LiquidCrystal_I2C* lcd) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  if (lcd != nullptr) {
    lcd->clear();
    lcd->setCursor(0, 1);
    lcd->print("Connecting WiFi...");
  }
  
  for (int i = 0; i < Config::WIFI_RETRY_COUNT; i++) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi Connected");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
      return true;
    }
    
    if (lcd != nullptr) {
      lcd->setCursor(i % 20, 2);
      lcd->print(".");
    }
    
    delay(500);
    esp_task_wdt_reset(); // Mencegah Watchdog Timer reset otomatis
  }
  
  return false;
}

void connectMQTT(PubSubClient& client) {
  if (client.connected()) return;
  
  Serial.print("Connecting MQTT...");
  // Buat Client ID Random agar tidak tabrakan sesi
  String clientId = "ESP32Scale-" + String(random(0xFFFF), HEX);
  
  if (client.connect(clientId.c_str())) {
    Serial.println(" Success!");
  } else {
    Serial.print(" Failed, rc=");
    Serial.println(client.state());
  }
}

bool checkNetworkHealth() {
  return Ping.ping(PING_HOST, 1);
}

void manageWiFiConnection(SystemState& state, unsigned long& lastWifiCheckTime) {
  if (millis() - lastWifiCheckTime < Config::WIFI_CHECK_INTERVAL) return;
  
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    state.offlineMode = true;
    state.isOnline = false;
  } else if (state.offlineMode) {
    // Jika sebelumnya offline, cek apakah sekarang sudah ada internet
    if (checkNetworkHealth()) {
      state.offlineMode = false;
      Serial.println("Reconnected! Ready to send.");
    }
  }
  
  lastWifiCheckTime = millis();
}

bool sendToLaravel(const SystemState& state) {
  if (WiFi.status() != WL_CONNECTED) return false;
  
  WiFiClientSecure clientSecure;
  clientSecure.setInsecure(); // Bypass SSL Certificate check (development only)
  
  HTTPClient http;
  Serial.println("\n--- LARAVEL POST ---");
  
  if (!http.begin(clientSecure, SERVER_URL)) {
    Serial.println("HTTP init failed!");
    return false;
  }
  
  http.setTimeout(Config::HTTP_TIMEOUT);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("Connection", "close");
  
  // Buat buffer char yang cukup besar (misal 256 karakter)
    char postData[256];

    // Format data ke dalam buffer
    // %.2f artinya float dengan 2 angka di belakang koma
    // %s artinya string (char array)
    snprintf(postData, sizeof(postData), 
            "api_key=%s&berat=%.2f&fakultas=%s&jenis=%s", 
            API_KEY, 
            state.currentWeight, 
            state.fakultas, 
            state.waste.getDisplayName().c_str()); // .c_str() mengubah String jadi char*

    Serial.print("Data: ");
    Serial.println(postData);

    int httpCode = http.POST(postData);
  bool success = false;
  
  if (httpCode > 0) {
    Serial.printf("HTTP Code: %d\n", httpCode);
    String response = http.getString();
    // Anggap sukses jika 200/201 atau ada kata "berhasil" di response body
    success = (httpCode == 200 || httpCode == 201 || response.indexOf("berhasil") >= 0);
    Serial.println(success ? "Database OK" : "Response unexpected");
  } else {
    Serial.printf("HTTP Error: %d - %s\n", httpCode, http.errorToString(httpCode).c_str());
  }
  
  http.end();
  clientSecure.stop();
  return success;
}

bool sendToMQTT(PubSubClient& client, const SystemState& state) {
  if (!client.connected()) {
    connectMQTT(client);
    if (!client.connected()) return false;
  }
  
    char payload[200];

    snprintf(payload, sizeof(payload), 
            "{\"weight\":%.2f,\"fakultas\":\"%s\",\"jenis\":\"%s\"}", 
            state.currentWeight, 
            state.fakultas, 
            state.waste.getDisplayName().c_str());

    Serial.print("📡 MQTT Publish: ");
    Serial.println(payload);

    bool success = client.publish(MQTT_TOPIC, payload);
    Serial.println(success ? "MQTT Sent" : "MQTT Failed");
    return success;
}