#pragma once

#include "controller.h"
#include "gcode_parser.h"
#include "motors.h"

void movePenDown(bool down);

class ProgramPlayer {
public:
  ProgramPlayer(Controller& controller)
    : controller_(controller), index_(0), paused_(false), pen_was_down_(false), program_size_(0) {}

  bool loadProgram(std::string_view input) {
    program_size_ = GCodeParser::parse(input, program_);
    reset();
    return input.empty();
  }
  void printProgram() const;

  void play() {
    paused_ = false;
  }
  void pause() {
    paused_ = true;
  }
  void reset() {
    index_ = 0;
    paused_ = true;
    controller_.reset();
  }

  void update(const State& state) {
    if (paused_ || index_ >= program_size_) return;

    const auto& cmd = program_[index_];
    switch (cmd.type) {
      case GCommand::RAPID:
        // lift pen, move, restore pen
        movePenDown(false);
        controller_.setSetpoint(cmd.target);
        if (controller_.done(state)) {
          if (pen_was_down_) {
            movePenDown(true);
          }
          ++index_;
        }
        break;
      case GCommand::LINEAR:
        // move (leaving pen at whatever state it was at before this move)
        controller_.setSetpoint(cmd.target);
        if (controller_.done(state)) ++index_;
        break;
      case GCommand::PEN_DOWN:
        movePenDown_(true);
        if (dwell(500)) ++index_;
        break;
      case GCommand::PEN_UP:
        movePenDown_(false);
        if (dwell(500)) ++index_;
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

  bool isFinished() const {
    return index_ >= program_size_;
  }

private:
  std::array<GCommand, MAX_COMMANDS> program_;
  Controller& controller_;
  size_t index_;
  bool paused_;
  bool pen_was_down_;
  size_t dwell_time_start_ = -1;
  size_t program_size_;
};

void ProgramPlayer::printProgram() const {
  for (size_t i = 0; i < program_size_; ++i) {
    const auto& cmd = program_.at(i);
    switch (cmd.type) {
      case GCommand::RAPID:
        WebSerial.printf("%3u: RAPID   (%.2f, %.2f)\n", i, cmd.target(0), cmd.target(1));
        break;
      case GCommand::LINEAR:
        WebSerial.printf("%3u: LINEAR  (%.2f, %.2f)\n", i, cmd.target(0), cmd.target(1));
        break;
      case GCommand::PEN_DOWN:
        WebSerial.printf("%3u: PEN DOWN\n", i);
        break;
      case GCommand::PEN_UP:
        WebSerial.printf("%3u: PEN UP\n", i);
        break;
      case GCommand::DWELL:
        // For simplicity, skip timing logic for now
        WebSerial.printf("%3u: DWELL   (%.2f)\n", i, cmd.dwell_ms);
        break;
      case GCommand::HOME:
        WebSerial.printf("%3u: HOME\n", i);
        break;
      case GCommand::END:
        WebSerial.printf("%3u: END\n", i);
        break;
    }
  }
}

ProgramPlayer gcode_player{ controller };
