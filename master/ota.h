#pragma once

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

void setupOta() {
  // Port defaults to 8266
  ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    WebSerial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    WebSerial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    WebSerial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    WebSerial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      WebSerial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      WebSerial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      WebSerial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      WebSerial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      WebSerial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}

void updateOta() {
  ArduinoOTA.handle();
}
