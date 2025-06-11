#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <ESPAsyncWebServer.h>
#include <WebSerial.h>

#include "ota.h"

#ifndef STASSID
#define STASSID "AlexNet"
#define STAPSK "145945195"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

AsyncWebServer server(80);

void recvMsg(uint8_t *data, size_t len){
  WebSerial.println("Received Data...");
  String d = "";
  for(int i=0; i < len; i++){
    d += char(data[i]);
  }
  if (d == "ON"){
  }
  if (d=="OFF"){
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  setupOta();

  // WebSerial is accessible at "<IP Address>/webserial" in browser
  WebSerial.begin(&server);
  WebSerial.onMessage(recvMsg);
  server.begin();
}

void loop() {
  static unsigned long last_print_time = millis();
  
  // Print every 2 seconds (non-blocking)
  if ((unsigned long)(millis() - last_print_time) > 2000) {
    last_print_time = millis();
    WebSerial.print(F("IP address: "));
    WebSerial.println(WiFi.localIP());
    WebSerial.printf("Uptime: %lums\n", millis());
    WebSerial.printf("Free heap: %u\n", ESP.getFreeHeap());
  }

  updateOta();
}
