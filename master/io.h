#include "motors.h"
#pragma once

#include <cstddef>
#include <cstdint>
#include <cmath>
#include <string_view>

#include <ESPAsyncWebServer.h>
#include <WebSerial.h>

#include "Metro.h"
#include "kinematics.h"
#include "controller.h"
#include "motors.h"
#include "string_parsing.h"
#include "gcode_player.h"

AsyncWebServer server(80);

Metro io_timer(15000);

void recvMsg(uint8_t* data, size_t len);

void setupIo() {
  // WebSerial is accessible at "<IP Address>/webserial" in browser
  WebSerial.begin(&server);
  WebSerial.onMessage(recvMsg);
  server.begin();
}

void updateIo() {
  if (io_timer.check()) {
    WebSerial.printf("IP address: %s, Uptime: %lums,\tFree heap: %u\n", WiFi.localIP().toString().c_str(), millis(), ESP.getFreeHeap());
  }
}

/**************************************************************************/

bool parseLine(std::string_view line);
bool parseP(const std::string_view& input);

void recvMsg(uint8_t* data, size_t len) {
  std::string_view input(reinterpret_cast<char*>(data), len);

  if (starts_with(input, "GCODE")) {
    std::string_view remaining = input.substr(5);
    gcode_player.loadProgram(remaining);
    return;
  }

  while (!input.empty()) {
    size_t split_pos = std::min(input.find('\n'), input.find(';'));

    std::string_view line = input.substr(0, split_pos);
    if (!parseLine(line)) {
      WebSerial.print(F("Failed to parse line: "));
      WebSerial.write(reinterpret_cast<const uint8_t*>(line.data()), line.size());
    }

    if (split_pos == std::string_view::npos) break;
    input.remove_prefix(split_pos + 1);
  }
}

bool parseLine(std::string_view line) {
  trim(line);
  if (line.empty()) return true;

  // "pop" front
  const char c = line.front();
  line.remove_prefix(1);

  switch (c) {
    case 'P':
      return parseP(line);
    case 'S':  // Set motor angles to R,L,Servo
      {
        long s1_target, s2_target;
        bool ret = parseNumbers(line, s1_target, s2_target, servo_target);
        if (ret) {
          stepper1.moveTo(s1_target);
          stepper2.moveTo(s2_target);
        }
        return ret;
      }
    case 'u':  // pen up
      movePenDown(false);
      return true;
    case 'd':  // pen down
      movePenDown(true);
      return true;
    case 'R':
      estimator.reset();
      controller.reset();
      gcode_player.reset();
      return true;
    case 'm':
      {
        double dx, dy;
        if (parseNumbers(line, dx, dy)) {
          WebSerial.printf("Calling move relative on %.2f, %.2f\n", dx, dy);
          controller.setSetpoint(controller.setpoint() + Eigen::Vector2d(dx, dy));
          return true;
        }
        return false;
      }
    case 'M':
      {
        Eigen::Vector2d xy;
        if (parseNumbers(line, xy(0), xy(1))) {
          WebSerial.printf("Calling move absolute on %.2f, %.2f\n", xy(0), xy(1));
          controller.setSetpoint(xy);
          return true;
        }
        return false;
      }
    case '>':
      gcode_player.play();
      return true;
    case '|':
      gcode_player.pause();
      return true;
    case 'p':
      gcode_player.printProgram();
      return true;
    case '?':
      estimator.print();
      controller.print();
      return true;
  }
  return false;
}


bool parseP(const std::string_view& input) {
  if (!input.empty()) {
    if (input.front() == '0') {
      motors_disabled = true;
      return true;
    } else if (input.front() == '1') {
      motors_disabled = false;
      return true;
    }
  }
  return false;
}
