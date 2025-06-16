#pragma once

#include "controller.h"
#include "gcode_parser.h"
#include "motors.h"

#define PEN_STATE_DELAY_MS 500

void movePenDown(bool down);

class ProgramPlayer {
 public:
  ProgramPlayer(Controller& controller)
      : controller_(controller),
        index_(0),
        paused_(false),
        pen_was_down_(false),
        program_size_(0) {}

  bool loadProgram(std::string_view input) {
    program_size_ = GCodeParser::parse(input, program_);
    reset();
    return input.empty();
  }
  void loadLine(std::string_view line) {
    GCodeParser::parse(line, program_, program_size_);
  }
  void startUpload() {
    disabled_for_upload_ = true;
    program_size_ = 0;
    WebSerial.println(F("Upload starting..."));
    reset();
  }
  bool isUploading() const { return disabled_for_upload_; }
  void endUpload() {
    disabled_for_upload_ = false;
    WebSerial.println(F("Upload finished, resetting program player."));
    reset();
  }
  size_t printLine(size_t i, char* buf, size_t max_chars) const;
  void printProgram() const;
  size_t printProgram(char* buf, size_t max_chars, size_t index) const;
  void print() const {
    WebSerial.printf(R"(
GCodePlayer state:
  program size: %zu
  paused: %d
  index: %zu
  state: %d
  disabled_for_upload: %d
  pen_was_down: %d
  dwell_time_start: %zu
  Current program line:
)",
                     program_size_, paused_, index_, state_,
                     disabled_for_upload_, pen_was_down_, dwell_time_start_);
    if (index_ >= 0 && index_ < program_size_) {
      char buf[128];
      size_t cmd_written = printLine(index_, buf, sizeof(buf));
      WebSerial.write(reinterpret_cast<uint8_t*>(buf), cmd_written);
    } else {
      WebSerial.println("No current program line (index out of bounds).");
    }
  }

  void play() { paused_ = false; }
  void pause() { paused_ = true; }
  void reset() {
    index_ = 0;
    paused_ = true;
    controller_.reset();
  }

  void update(const State& state) {
    if (paused_ || disabled_for_upload_ || index_ >= program_size_) return;

    const auto& cmd = program_[index_];
    switch (cmd.type) {
      case GCommand::RAPID: {  // lift pen, move, restore pen
        bool advance = [this, &cmd, &state](int& state_) {
          switch (state_) {
            case -1:
              return true;
            case 0:
              movePenDown(false);
              return true;
            case 1:
              return (!pen_was_down_) || (PEN_STATE_DELAY_MS);
            case 2:
              controller_.setSetpoint(cmd.target);
              return true;
            case 3:
              return controller_.done(state);
            case 4:
              if (pen_was_down_) movePenDown(true);
              return true;
            case 5:
              return (!pen_was_down_) || dwell(PEN_STATE_DELAY_MS);
            case 6:
              ++index_;
              state_ = -1;
              return false;
            default:
              return false;  // Invalid state, do not advance
          }
        }(state_);
        if (advance) {
          ++state_;
        }
        break;
      }
      case GCommand::LINEAR:
        // move (leaving pen at whatever state it was at before this move)
        controller_.setSetpoint(cmd.target);
        if (controller_.done(state)) ++index_;
        break;
      case GCommand::PEN_DOWN:
        movePenDown_(true);
        if (dwell(PEN_STATE_DELAY_MS)) ++index_;
        break;
      case GCommand::PEN_UP:
        movePenDown_(false);
        if (dwell(PEN_STATE_DELAY_MS)) ++index_;
        ++index_;
        break;
      case GCommand::DWELL:
        if (dwell(cmd.dwell_ms)) ++index_;
        break;
      case GCommand::HOME:
        controller_.setSetpoint(Eigen::Vector2d::Zero());
        if (controller_.done(state)) ++index_;
        break;
      case GCommand::END:
        ++index_;
        paused_ = true;
        break;
    }
  }

  // Returns true if finished
  bool dwell(size_t duration_ms) {
    if (dwell_time_start_ == static_cast<size_t>(-1)) {
      dwell_time_start_ = millis();
    }
    bool ret = (millis() - dwell_time_start_) > duration_ms;
    if (ret) dwell_time_start_ = static_cast<size_t>(-1);
    return ret;
  }

  void movePenDown_(bool down) {
    movePenDown(down);
    pen_was_down_ = down;
  }

  bool isFinished() const { return index_ >= program_size_; }

 private:
  std::array<GCommand, MAX_COMMANDS> program_;
  Controller& controller_;
  size_t index_;
  int state_ = -1;
  bool paused_;
  bool pen_was_down_;
  size_t dwell_time_start_ = -1;
  size_t program_size_;
  bool disabled_for_upload_ = false;  // Used to disable execution during upload
};

size_t ProgramPlayer::printLine(size_t i, char* buf, size_t max_chars) const {
  const auto& cmd = program_.at(i);
  switch (cmd.type) {
    case GCommand::RAPID:
      return snprintf(buf, max_chars, "%3u: RAPID   (%.2f, %.2f)\n", i,
                      cmd.target(0), cmd.target(1));
      break;
    case GCommand::LINEAR:
      return snprintf(buf, max_chars, "%3u: LINEAR  (%.2f, %.2f)\n", i,
                      cmd.target(0), cmd.target(1));
      break;
    case GCommand::PEN_DOWN:
      return snprintf(buf, max_chars, "%3u: PEN DOWN\n", i);
      break;
    case GCommand::PEN_UP:
      return snprintf(buf, max_chars, "%3u: PEN UP\n", i);
      break;
    case GCommand::DWELL:
      // For simplicity, skip timing logic for now
      return snprintf(buf, max_chars, "%3u: DWELL   (%.2f)\n", i, cmd.dwell_ms);
      break;
    case GCommand::HOME:
      return snprintf(buf, max_chars, "%3u: HOME\n", i);
      break;
    case GCommand::END:
      return snprintf(buf, max_chars, "%3u: END\n", i);
      break;
  }
  return 0;
}

void ProgramPlayer::printProgram() const {
  char buf[128];
  for (size_t i = 0; i < program_size_; ++i) {
    size_t cmd_written = printLine(i, buf, sizeof(buf));
    WebSerial.write(reinterpret_cast<uint8_t*>(buf), cmd_written);
  }
}

// Given program string `str`, populate `buf` with `str[index:index+max_chars]`
size_t ProgramPlayer::printProgram(char* buf, size_t max_chars,
                                   size_t index) const {
  // A "cache" of where we previously left off.
  static size_t prev_byte = 0;
  static size_t start_cmd_index = 0;

  char tmp_buf[128];
  char* original_buf = buf;  // for counting how much we wrote

  // If start at the beginning, we can reset our "cache" of where we are.
  if (index == 0) {
    prev_byte = 0;
    start_cmd_index = 0;
  }

  // If our "cache" is invalid, we need to re-scan from the start.
  if (index != prev_byte) {
    for (start_cmd_index = 0; start_cmd_index < program_size_;
         ++start_cmd_index) {
      size_t cmd_written = printLine(start_cmd_index, tmp_buf, sizeof(tmp_buf));
      if (cmd_written > sizeof(tmp_buf)) {
        // Failure.  Just return.
        WebSerial.printf(
            "Failed to print program, command %zu too long for buffer (%zu "
            "bytes)\n",
            start_cmd_index, cmd_written);
        return 0;
      }
      if (prev_byte + cmd_written > index) {
        // We got truncated.  Here is where we start.
        // start_cmd_index is already set.
        break;
      }
      prev_byte += cmd_written;
    }
  }

  // Now print the rest of the program.
  for (; start_cmd_index < program_size_; ++start_cmd_index) {
    size_t cmd_written = printLine(start_cmd_index, buf, max_chars);
    if (cmd_written > max_chars) {
      // we got truncated.  Just pretend this entire line never got printed.
      prev_byte += buf - original_buf;
      return buf - original_buf;
    }
    buf += cmd_written;
    max_chars -= cmd_written;
  }

  // We finished printing the program.
  prev_byte += buf - original_buf;
  return buf - original_buf;
}

ProgramPlayer gcode_player{controller};
