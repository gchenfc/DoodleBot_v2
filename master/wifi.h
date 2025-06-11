#pragma once

#include <ESP8266WiFi.h>

#include "secrets.h"  // Define wifi credentials here or in secrets.h

#ifndef STASSID
#define STASSID "WIFI_SSID"
#define STAPSK "WIFI_PASSWORD"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

void setupWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    // Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
}

void updateWifi() {}
