#pragma once

#include <Stepper.h>
#include <AccelStepper.h>
#include <Servo.h>

#include "constants.h"
#include "estimator.h"
#include "controller.h"
#include "Metro.h"

Metro motor_timer(50);
Metro servo_timer(100);
Metro motor_report_timer(5000);

bool motors_disabled = false;
int servo_target = 180;

AccelStepper stepper1(AccelStepper::FULL4WIRE, D4, D2, D3, D1);
AccelStepper stepper2(AccelStepper::FULL4WIRE, D8, D6, D7, D5);
Servo servo;

void applyDq(int64_t d1, int64_t d2);
void movePenDown(bool down);

void setupMotors() {
  stepper1.setMaxSpeed(500.0);
  stepper1.setAcceleration(100000.0);

  stepper2.setMaxSpeed(500.0);
  stepper2.setAcceleration(100000.0);

  servo.attach(TX);

  controller.setSetpoint(estimator.state().pen());
}

void updateMotors() {
  // step one revolution  in one direction:
  if (motor_timer.check() && !motors_disabled) {
    // if (stepper1.distanceToGo() > 10) return;
    // if (stepper2.distanceToGo() > 10) return;
    const int64_t cur_stepper1 = stepper1.currentPosition();
    const int64_t cur_stepper2 = stepper2.currentPosition();

    estimator.update(Eigen::Vector2d(cur_stepper1, cur_stepper2) * UNITS_PER_STEP);
    if (!controller.done(estimator.state())) {
      Eigen::Vector2d dq = controller.getAction(estimator.state()) * STEPS_PER_UNIT;
      // estimator.print();
      // controller.print();
      applyDq(dq(0), dq(1));
    }
  }
  if (servo_timer.check() && !motors_disabled) {
    servo.write(servo_target);
  }
  stepper1.run();
  stepper2.run();
}

// Applies delta-wheel motions
void applyDq(int64_t d1, int64_t d2) {
  int64_t a1 = std::abs(d1), a2 = std::abs(d2);
  int64_t max = std::max(a1, a2);
  stepper1.setMaxSpeed(500.0 * a1 / max);
  stepper2.setMaxSpeed(500.0 * a2 / max);
  // Now update the setpoints
  stepper1.move(d1);
  stepper2.move(d2);
}

void movePenDown(bool down) {
  servo_target = down ? 0 : 160;
}
