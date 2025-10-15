/*
 * SUPER SIMPLE HX711 READER - GRAM
 */

#include <Arduino.h>
#include "HX711_ADC.h"

HX711_ADC LoadCell(2, 4);
float calFactor = 12.351620;

void setup() {
  Serial.begin(115200);
  LoadCell.begin();
  LoadCell.start(2000, true);
  LoadCell.setCalFactor(calFactor);
  delay(1000);
}

void loop() {
  LoadCell.update();
  Serial.println(LoadCell.getData(), 4); // Gram, 4 decimal
  delay(100);
}