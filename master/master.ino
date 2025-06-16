#include <WebSerial.h>

#include "motors.h"
#include "wifi.h"
#include "ota.h"
#include "io.h"
#include "ui.h"

void setup() {
  // WebSerial is accessible at "<IP Address>/webserial" in browser
  setupWifi();
  setupOta();
  setupIo();
  setupUi();
  setupMotors();
}

void loop() {
  updateWifi();
  updateOta();
  updateIo();
  updateUi();
  updateMotors();
}
