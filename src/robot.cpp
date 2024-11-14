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
  imu.setGyroDataOutputRate(LSM6::ODR::ODR104);
  imu.setFullScaleGyro(LSM6::GYRO_FS::GYRO_FS2000);
  imu.setAccDataOutputRate(LSM6::ODR::ODR104);
  imu.setFullScaleAcc(LSM6::ACC_FS::ACC_FS8);
  //
  // The line sensor elements default to INPUTs, but we'll initialize anyways,
  // for completeness
  lineSensor.Initialize();

  pinMode(A7, OUTPUT);

  Serial1.begin(115200);
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
  if (robotState == ROBOT_TURNING) {
    if (abs(currentHeading - targetHeading) <= 2.5) {
      EnterIdleState();
      retVal = true;
    }
  }

  return retVal;
}

void Robot::HandleTurnComplete(void) { EnterIdleState(); }

long lastOrientationUpdate = 0;
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
    imu.g.x -= imu.gyroBias.x;
    imu.g.y -= imu.gyroBias.y;
    imu.g.z -= imu.gyroBias.z;

    float deltaT = 1.0 / imu.gyroODR;
    float accelScale = imu.mgPerLSB / 1000.0;

    float accelRoll =
        atan2(imu.a.y * accelScale, imu.a.z * accelScale) * 180.0 / M_PI;
    float accelPitch =
        atan2(imu.a.x * accelScale,
              sqrt(pow(imu.a.x, 2) + pow(imu.a.z, 2)) * accelScale) *
        180.0 / M_PI;

    float gyroScale = imu.mdpsPerLSB / 1000.0;

    float kappa = 0.02;

    eulerAngles.x =
        (1 - kappa) * (eulerAngles.x + imu.g.x * deltaT * gyroScale) +
        kappa * accelRoll;
    eulerAngles.y =
        (1 - kappa) * (eulerAngles.y + imu.g.y * deltaT * gyroScale) +
        kappa * accelPitch;
    eulerAngles.z = eulerAngles.z + -imu.g.z * deltaT * gyroScale;

    currentHeading = eulerAngles.z;

    if (currentHeading < 0)
      currentHeading = 360 + fmod(eulerAngles.z, 360);
    else
      currentHeading = fmod(eulerAngles.z, 360);

#ifdef __IMU_DEBUG__
    Serial.print("x: ");
    Serial.print(imu.a.x);
    Serial.print(" y: ");
    Serial.print(imu.a.y);
    Serial.print(" z: ");
    Serial.println(imu.a.z);
#endif
  }
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
    //
    // if (leftOffLine && rightOffLine) {
    //   eStopTicks++;
    //   emergencyKp += 0.1;
    // } else {
    //   eStopTicks = 0;
    // }
    //
    // if (eStopTicks > 25) {
    //   chassis.Stop();
    //   EnterIdleState();
    //   return;
    // }

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

  /**
   * Check for an IMU update
   */

  if (imu.checkForNewData()) {
    HandleOrientationUpdate();
  }
}
