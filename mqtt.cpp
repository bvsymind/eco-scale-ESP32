#include <Arduino.h>
#include <WiFi.h>              // Ganti ke <ESP8266WiFi.h> jika pakai ESP8266
#include <PubSubClient.h>
#include "HX711_ADC.h"
#include "credentials.h"

// WiFi credentials
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// MQTT broker
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_topic = "undip/ecoscale/loadcell/berat";

// HX711 setup
HX711_ADC LoadCell(2, 4);  // Pin DT, SCK
float calFactor = 12.351620;

// WiFi and MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

// Reconnect MQTT
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Menghubungkan ke MQTT...");
    if (client.connect("LoadCellClient")) {
      Serial.println("terhubung!");
    } else {
      Serial.print("gagal, rc=");
      Serial.print(client.state());
      Serial.println(" coba lagi dalam 5 detik");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Koneksi WiFi
  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan ke WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi terhubung");

  // MQTT setup
  client.setServer(mqtt_server, mqtt_port);

  // Load Cell
  LoadCell.begin();
  LoadCell.start(2000, true);
  LoadCell.setCalFactor(calFactor);
  delay(1000);
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  LoadCell.update();
  float berat = LoadCell.getData();

  char payload[16];
  dtostrf(berat, 1, 4, payload);

  client.publish(mqtt_topic, payload);
  Serial.print("Data terkirim: ");
  Serial.println(payload);

  delay(130);
}
