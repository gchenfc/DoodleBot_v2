
/*
 Stepper Motor Control - one revolution

 This program drives a unipolar or bipolar stepper motor.
 The motor is attached to digital pins 8 - 11 of the Arduino.

 The motor should revolve one revolution in one direction, then
 one revolution in the other direction.


 Created 11 Mar. 2007
 Modified 30 Nov. 2009
 by Tom Igoe

 */

#include <Stepper.h>
#include <Servo.h>

const int stepsPerRevolution = 200;  // change this to fit the number of steps per revolution
// for your motor

// initialize the stepper library on pins 8 through 11:
Stepper stepper1(stepsPerRevolution, D1, D3, D2, D4);
Stepper stepper2(stepsPerRevolution, D8, D6, D7, D5);
Servo servo;

void setup() {
  // set the speed at 60 rpm:
  stepper1.setSpeed(60);
  stepper2.setSpeed(60);
  servo.attach(TX);
  // initialize the serial port:
  // Serial.begin(9600);
}

void loop() {
  // step one revolution  in one direction:
  // Serial.println("clockwise");
  stepper1.step(stepsPerRevolution);
  stepper2.step(stepsPerRevolution);
  delay(100);
  servo.write(90);
  delay(500);

  // step one revolution in the other direction:
  // Serial.println("counterclockwise");
  stepper1.step(-stepsPerRevolution);
  stepper2.step(-stepsPerRevolution);
  delay(100);
  servo.write(180);
  delay(500);
}

