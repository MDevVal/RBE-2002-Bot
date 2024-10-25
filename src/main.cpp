#include "robot.h"
#include <Arduino.h>
#include <LSM6.h>

Robot robot;

void setup() {
  Serial.begin(250000);

#ifdef __LOOP_DEBUG__
  while (!Serial) {
    delay(5);
  }
#endif

  robot.InitializeRobot();
}

void loop() { robot.RobotLoop(); }
