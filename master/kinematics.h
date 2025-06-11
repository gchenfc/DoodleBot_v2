#include "WebSerial.h"
#pragma once

#include <ArduinoEigen.h>

#include "constants.h"

using Matrix32d = Eigen::Matrix<double, 3, 2>;
using Matrix23d = Eigen::Matrix<double, 2, 3>;
using Q = Eigen::Vector2d;  // right wheel, left wheel

struct State {
  double x;
  double y;
  double cos, sin;

  void update(const Eigen::Vector3d& delta_state) {
    x += delta_state.x();
    y += delta_state.y();
    // cos(a + b) = cosa * cosb - sina * sinb
    // sin(a + b) = sina * cosb + cosa * sinb
    double cdelta = std::cos(delta_state.z()), sdelta = std::sin(delta_state.z());
    double new_cos = cos * cdelta - sin * sdelta;
    double new_sin = sin * cdelta + cos * sdelta;
    // re-condition
    double norm = std::sqrt(new_cos * new_cos + new_sin * new_sin);
    cos = new_cos / norm;
    sin = new_sin / norm;
  }

  Eigen::Vector2d pen() const {
    return { x + cos * LENGTH_UNIT, y + sin * LENGTH_UNIT };
  }
};

/***** Jacobians: *****/

Matrix32d state_D_q(const State& state) {
  Matrix32d ret;
  ret.row(0) << state.cos / 2, state.cos / 2;
  ret.row(1) << state.sin / 2, state.sin / 2;
  // jacobian should be: limit_{dl -> 0} [atan(dl / WIDTH) / dl]
  //                   = limit_{dl -> 0} [atan(dl / WIDTH) / (dl / WIDTH)] / WIDTH
  //                   = 1 / WIDTH
  ret.row(2) << 1.0 / WIDTH_UNIT, -1.0 / WIDTH_UNIT;
  return ret;
}

Matrix23d pen_D_state(const State& state) {
  Matrix23d ret;
  ret.row(0) << 1, 0, -state.sin * LENGTH_UNIT;
  ret.row(1) << 0, 1, state.cos * LENGTH_UNIT;
  return ret;
}
