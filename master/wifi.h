#pragma once

#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>

#include "secrets.h"  // Define wifi credentials here or in secrets.h

#ifndef STASSID
#define STASSID "WIFI_SSID"
#define STAPSK "WIFI_PASSWORD"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

AsyncWebServer server(80);

void setupWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.hostname("doodlebot");
  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    // Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  server.begin();

  server.onNotFound([](AsyncWebServerRequest* request) {
    request->send(404, "text/plain", "404: Not Found");
  });
}

void updateWifi() {}
