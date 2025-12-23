#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>

enum class AppState : uint8_t {
  IDLE,
  SELECTING_SUBTYPE,
  SENDING_DATA,
  SHOWING_STATUS
};

struct WasteData {
  char jenis[16] = "--";
  char subJenis[16] = "--";
  
  void reset() {
    strcpy(jenis, "--");
    strcpy(subJenis, "--");
  }
  
  void setType(const char* type, const char* subtype = "--") {
    strncpy(jenis, type, sizeof(jenis) - 1);
    jenis[sizeof(jenis) - 1] = '\0';
    strncpy(subJenis, subtype, sizeof(subJenis) - 1);
    subJenis[sizeof(subJenis) - 1] = '\0';
  }
  
  String getDisplayName() const {
    if (strcmp(jenis, "Anorganik") == 0 && strcmp(subJenis, "--") != 0) {
      return (strcmp(subJenis, "Umum") == 0) ? "Anorganik" : String(subJenis);
    }
    return String(jenis);
  }
};

struct SystemState {
  AppState appState = AppState::IDLE;
  WasteData waste;
  char fakultas[8] = "FPsi";
  float currentWeight = 0.0f;
  float lastDisplayedWeight = -1.0f;
  bool offlineMode = false;
  bool isOnline = false;
  bool newDataReady = false;
};

#endif