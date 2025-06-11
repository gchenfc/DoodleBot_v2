#include <memory>
#pragma once

#include <array>
#include <string>
#include <string_view>

#include <ArduinoEigen.h>

#include "controller.h"
#include "string_parsing.h"

constexpr size_t MAX_COMMANDS = 100;

// Parsed representation of a G-code move
struct GCommand {
  enum Type : uint8_t { RAPID,
                        LINEAR,
                        PEN_UP,
                        PEN_DOWN,
                        DWELL,
                        HOME,
                        END } type;
  Eigen::Vector2d target;  // For RAPID/LINEAR
  double dwell_ms;         // For DWELL
};

class GCodeParser {
public:
  static size_t parse(std::string_view& input, std::array<GCommand, MAX_COMMANDS>& out_program) {
    trim(input);
    
    size_t count = 0;

    bool cur_is_abs = true;
    Eigen::Vector2d cur_pos(0, 0);

    while (!input.empty() && count < MAX_COMMANDS) {
      auto endline = input.find('\n');
      if (endline == std::string_view::npos) endline = input.size();
      auto line = input.substr(0, endline);

      trim(line);
      if (line.empty() || line.front() == ';') continue;

      GCommand cmd;
      if (starts_with(line, "G0 ") || starts_with(line, "G1 ")) {
        cmd.type = line[1] == '0' ? GCommand::RAPID : GCommand::LINEAR;
        if (!parseMove(line.substr(3), cur_is_abs, cur_pos)) {
          WebSerial.print(F("Failed to parse gcode line:\n\t"));
          WebSerial.write(reinterpret_cast<const uint8_t*>(line.data()), line.size());
        } else {
          cmd.target = cur_pos;
        }
      } else if (starts_with(line, "G4 ")) {
        cmd.type = GCommand::DWELL;
        line.remove_prefix(3);
        trimFront(line);
        if (line.empty() || line.front() != 'P' || !parseNumbersNotInPlace(line.substr(1), cmd.dwell_ms)) {
          WebSerial.print(F("Failed to parse gcode line:\n\t"));
          WebSerial.write(reinterpret_cast<const uint8_t*>(line.data()), line.size());
        }
      } else if (starts_with(line, "G28 ")) {
        cmd.type = GCommand::HOME;
      } else if (starts_with(line, "G90 ")) {
        cur_is_abs = true;
      } else if (starts_with(line, "G91 ")) {
        cur_is_abs = false;
      } else if (starts_with(line, "M2 ") || starts_with(line, "M30 ")) {
        cmd.type = GCommand::END;
      } else if (starts_with(line, "M3 ")) {
        cmd.type = GCommand::PEN_DOWN;
      } else if (starts_with(line, "M5 ")) {
        cmd.type = GCommand::PEN_UP;
      } else {
        WebSerial.print(F("Failed to parse gcode line:\n\t"));
        WebSerial.write(reinterpret_cast<const uint8_t*>(line.data()), line.size());
        continue;
      }
      out_program[count++] = cmd;

      input.remove_prefix(endline + 1);
    }
    WebSerial.printf("Parsed %u gcode lines.\n", count);
    if (count < MAX_COMMANDS) {
      out_program[count++].type = GCommand::END;
    } else {
      WebSerial.printf("Ran out of gcode storage space!  Still %u characters remaining.\n", input.size());
    }
    return count;
  }

private:
  static bool parseMove(std::string_view line, bool is_abs, Eigen::Vector2d& pos) {
    std::optional<double> x, y;
    while (!line.empty()) {
      trimFront(line);
      std::optional<double>* target;
      switch (line.front()) {
        case 'x':
        case 'X':
          target = &x;
          break;
        case 'y':
        case 'Y':
          target = &y;
          break;
        case ';':
          line.remove_suffix(line.size());
          break;
        default:
          return false;
      }
      if (!target) continue;  // ";" case
      line.remove_prefix(1);
      std::optional<double> val = parseFloat<double>(line);
      if (!val) return false;
      *target = *val;
      trimBack(line);
    }

    // Write output
    if (is_abs) {
      if (x) pos(0) = *x;
      if (y) pos(1) = *y;
    } else {
      if (x) pos(0) += *x;
      if (y) pos(1) += *y;
    }
    return true;
  }
};
