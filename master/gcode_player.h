#pragma once

#include "controller.h"
#include "gcode_parser.h"
#include "motors.h"

class ProgramPlayer {
public:
  ProgramPlayer(Controller& controller)
    : controller_(controller), index_(0), paused_(false), pen_was_down_(false), program_size_(0) {}

  bool loadProgram(std::string_view input) {
    program_size_ = GCodeParser::parse(input, program_);
    reset();
    return input.empty();
  }

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
        ++index_;
        break;
      case GCommand::PEN_UP:
        movePenDown_(false);
        ++index_;
        break;
      case GCommand::DWELL:
        // For simplicity, skip timing logic for now
        ++index_;
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
  size_t program_size_;
};

ProgramPlayer gcode_player{ controller };
