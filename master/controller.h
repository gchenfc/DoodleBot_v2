#pragma once

#include "kinematics.h"

class Controller {
public:
  Controller()
    : setpoint_(Eigen::Vector2d::Zero()) {}

  bool done(const State& state, double tol = 1.0) const {
    return (setpoint_ - state.pen()).norm() < tol;
  }

  auto setpoint() const {
    return setpoint_;
  }

  void setSetpoint(const Eigen::Vector2d& setpoint) {
    setpoint_ = setpoint;
  }

  void reset() {
    setpoint_ = Eigen::Vector2d::Zero();
  }

  Eigen::Vector2d getAction(const State& state, double max_step_size_unit = 5.0, bool verbose = false) const {
    const auto state_H_q = state_D_q(state);
    const auto pen_H_state = pen_D_state(state);
    const Eigen::Matrix2d pen_H_q = pen_H_state * state_H_q;
    Eigen::Matrix2d q_H_pen = pen_H_q.inverse();

    const Eigen::Vector2d error = setpoint_ - state.pen();
    const Eigen::Vector2d dq = q_H_pen * error;

    // Clamp if too big
    const double dq_norm = dq.norm();
    const Eigen::Vector2d dq_cmd = dq_norm > max_step_size_unit ? (dq * max_step_size_unit / dq_norm) : dq;

    if (verbose) {
      WebSerial.printf(R"(
Controller debug!
  pen error: (%.3f, %.3f)
  pen_H_q: [%.3f, %.3f
            %.3f, %.3f]
  q_H_pen: [%.3f, %.3f
            %.3f, %.3f]
  dq raw: (%.2f, %.2f)
  dq command: (%.2f, %.2f)
)",
                       error(0), error(1),
                       pen_H_q(0, 0), pen_H_q(0, 1), pen_H_q(1, 0), pen_H_q(1, 1),
                       q_H_pen(0, 0), q_H_pen(0, 1), q_H_pen(1, 0), q_H_pen(1, 1),
                       dq(0), dq(1),
                       dq_cmd(0), dq_cmd(1));
    }

    return dq_cmd;
  }

  void print() {
    WebSerial.printf(R"(
Controller:
  setpoint: (%.3f, %.3f)
)",
                     setpoint_(0), setpoint_(1));
  }

private:
  Eigen::Vector2d setpoint_;
};

Controller controller{};
