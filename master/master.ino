#include <WebSerial.h>

#include "motors.h"
#include "wifi.h"
#include "ota.h"
#include "io.h"

void setup() {
  // WebSerial is accessible at "<IP Address>/webserial" in browser
  setupWifi();
  setupOta();
  setupIo();
  setupMotors();
}

void loop() {
  updateWifi();
  updateOta();
  updateIo();
  updateMotors();
}
