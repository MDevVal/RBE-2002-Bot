#include "robot.h"
#include "Arduino.h"
#include "LSM6.h"

void Robot::InitializeRobot(void) {
  chassis.InititalizeChassis();

  /**
   * Initialize the IR decoder. Declared extern in IRdecoder.h; see
   * robot-remote.cpp for instantiation and setting the pin.
   */
  decoder.init();

  /**
   * Initialize the IMU and set the rate and scale to reasonable values.
   */
  imu.init();
  imu.setGyroDataOutputRate(LSM6::ODR::ODR208);
  imu.setFullScaleGyro(LSM6::GYRO_FS::GYRO_FS1000);

  // The line sensor elements default to INPUTs, but we'll initialize anyways,
  // for completeness
  lineSensor.Initialize();
}

void Robot::EnterIdleState(void) {
  if (robotState == ROBOT_IDLE)
    return;

  Serial.println("-> IDLE");

  robotState = ROBOT_IDLE;
  chassis.Stop();
  idleTime = millis();
}

void Robot::EnterTurn(float angle) {
  Serial.println(" -> TURNING");
  robotState = ROBOT_TURNING;

  bool clockwise = false;
  if (angle < 0) {
    angle = 360 + angle;
    clockwise = true;
  }

  targetHeading = fmod(currentHeading + angle, 360);

  if (clockwise) {
    chassis.SetTwist(0, 1);
  } else {
    chassis.SetTwist(0, -1);
  }
}

int lastTime = 0;
bool Robot::CheckTurnComplete(void) {
  bool retVal = false;

  if (robotState != ROBOT_TURNING)
    return retVal;

  if (millis() - lastTime > 0) {
    lastTime = millis();
    Serial.print("Current: ");
    Serial.println(currentHeading);
    Serial.print("Target: ");
    Serial.println(targetHeading);
  }

  if (robotState == ROBOT_TURNING) {
    if (abs(currentHeading - targetHeading) <= 0.1) {
      EnterIdleState();
      retVal = true;
    }
  }

  return retVal;
}

void Robot::HandleTurnComplete(void) { EnterIdleState(); }

/**
 * Here is a good example of handling information differently, depending on
 * the state. If the Romi is not moving, we can update the bias (but be
 * careful when you first start up!). When it's moving, then we update the
 * heading.
 */
void Robot::HandleOrientationUpdate(void) {
  prevEulerAngles = eulerAngles;

  if (robotState == ROBOT_IDLE) {
    if (millis() - idleTime > 1000) {
      imu.updateGyroBias();
    }
  } else {
    // Subtract gyro bias
    imu.g.x -= imu.gyroBias.x;
    imu.g.y -= imu.gyroBias.y;
    imu.g.z -= imu.gyroBias.z;

    float deltaT = 1.0 / imu.gyroODR;
    eulerAngles.x =
        prevEulerAngles.x + imu.g.x * deltaT * (imu.mdpsPerLSB / 1000.0);
    eulerAngles.y =
        prevEulerAngles.y + imu.g.y * deltaT * (imu.mdpsPerLSB / 1000.0);
    eulerAngles.z =
        prevEulerAngles.z + -imu.g.z * deltaT * (imu.mdpsPerLSB / 1000.0);

    float rollAcc = atan2(imu.a.y, imu.a.z) * (180.0 / M_PI);
    float pitchAcc =
        atan2(-imu.a.x, sqrt(imu.a.y * imu.a.y + imu.a.z * imu.a.z)) *
        (180.0 / M_PI);

    float alpha = 0.98;
    eulerAngles.x = alpha * eulerAngles.x + (1 - alpha) * rollAcc;  // Roll
    eulerAngles.y = alpha * eulerAngles.y + (1 - alpha) * pitchAcc; // Pitch

    currentHeading = eulerAngles.z;

    if (currentHeading < 0)
      currentHeading = 360 + fmod(eulerAngles.z, 360);
    else
      currentHeading = fmod(eulerAngles.z, 360);
  }
#ifdef __IMU_DEBUG__
  Serial.print("Current roll: ");
  Serial.println(eulerAngles.z);
  Serial.print("Current heading: ");
  Serial.println(currentHeading);
#endif
}

/**
 * Functions related to line following and intersection detection.
 */
void Robot::EnterLineFollowing(float speed) {
  Serial.println(" -> LINING");
  baseSpeed = speed;
  robotState = ROBOT_LINING;
  prevError = 0;

  ResetElapsedDistance();
}

float Robot::GetDistanceElapsed() {
  return chassis.GetDistanceElapsed() - elapsedDistanceSetPoint;
}

void Robot::ResetElapsedDistance() {
  elapsedDistanceSetPoint = chassis.GetDistanceElapsed();
}

uint8_t eStopTicks = 0;
void Robot::LineFollowingUpdate(bool invert) {
  if (robotState == ROBOT_LINING) {
    float lineError =
        (invert ? -lineSensor.CalcError() : lineSensor.CalcError()) / 1023.0;
    float derivative = (lineError - prevError);

    bool leftOffLine =
        invert ? lineSensor.ReadLeft() > 800 : lineSensor.ReadLeft() < 200;
    bool rightOffLine =
        invert ? lineSensor.ReadRight() > 800 : lineSensor.ReadRight() < 200;

    if (leftOffLine && rightOffLine) {
      eStopTicks++;
      emergencyKp += 0.1;
    } else {
      eStopTicks = 0;
    }

    if (eStopTicks > 25) {
      chassis.Stop();
      EnterIdleState();
      return;
    }

    float lineError2 = lineError * abs(lineError);

    float turnEffort = lineError * (lineKp + emergencyKp) +
                       lineKp2 * lineError2 + derivative * lineKd;

    chassis.SetTwist(baseSpeed, turnEffort);
    prevError = lineError;
  }
}

void Robot::PrintLapStats() {
  Serial.print("---LAP COMPLETE---");
  Serial.print("\nLAP TIME: ");
  Serial.print(lastLapTime);
  Serial.print("s");

  Serial.print("\nLAP SPEED: ");
  Serial.print(lapDistance / lastLapTime);
  Serial.println("cm/s");
}

void Robot::RobotLoop(void) {
  /**
   * The main loop for your robot. Process both synchronous events (motor
   * control), and asynchronous events (IR presses, distance readings, etc.).
   */

  /**
   * Handle any IR remote keypresses.
   */
  int16_t keyCode = decoder.getKeyCode();
  if (keyCode != -1) {
    HandleKeyCode(keyCode);
  }

  /**
   * Check the Chassis timer, which is used for executing motor control
   */
  if (chassis.CheckChassisTimer()) {
    // add synchronous, pre-motor-update actions here

    HandleAutonRoutine(robotAutonRoutine);
    chassis.UpdateMotors();
  }

  if (CheckTurnComplete())
    HandleTurnComplete();

  /**
   * Check for an IMU update
   */
  if (imu.checkForNewData()) {
    HandleOrientationUpdate();
  }
}
