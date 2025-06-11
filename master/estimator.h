#pragma once

#include "constants.h"
#include "kinematics.h"

class Estimator {
public:
  Estimator()
    : state_({ .x = -LENGTH_UNIT, .y = 0, .cos = 1, .sin = 0 }),
      q_prev_(0, 0) {}

  void update(const Q& q, bool verbose = false) {
    const auto state_H_q = state_D_q(state_);
    const auto dq = q - q_prev_;
    const auto dstate = state_H_q * dq;
    state_.update(dstate);

    if (verbose) {
      WebSerial.printf(R"(
Estimator debug!
  q prev: (%.3f, %.3f)
  q curr: (%.3f, %.3f)
  dq:     (%.3f, %.3f)
  state_H_q: [%.3f, %.3f
              %.3f, %.3f
              %.3f, %.3f]
  dstate: (%.2f, %.2f, %.2f)
)",
                       q_prev_(0), q_prev_(1), q(0), q(1),
                       dq(0), dq(1),
                       state_H_q(0, 0), state_H_q(0, 1), state_H_q(1, 0), state_H_q(1, 1), state_H_q(2, 0), state_H_q(2, 1),
                       dstate(0), dstate(1), dstate(2));
    }

    q_prev_ = q;
  }

  const State& state() {
    return state_;
  }

  void reset() {
    state_ = State{ .x = -LENGTH_UNIT, .y = 0, .cos = 1, .sin = 0 };
    // Intentionally don't reset prev_q.
  }

  void print() {
    const auto pen_pos = state_.pen();
    WebSerial.printf(R"(
Estimator:
  state: (%.3f, %.3f) angle [%.3f, %.3f] - pen pos: (%.3f, %.3f)
  q_prev: (%.3f, %.3f)
)",
                     state_.x, state_.y, state_.cos, state_.sin, pen_pos(0), pen_pos(1), q_prev_(0), q_prev_(1));
  }

private:
  State state_;
  Q q_prev_;
};

Estimator estimator{};
