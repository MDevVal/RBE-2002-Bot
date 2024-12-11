#include "robot.h"

void Robot::InitializeRobot(void) {
  chassis.InititalizeChassis();

  /**
   * Initialize the IMU and set the rate and scale to reasonable values.
   */
  imu.init();

  // The line sensor elements default to INPUTs, but we'll initialize anyways,
  // for completeness
  lineSensor.Initialize();

  dataTimer.start(5000);
}

void Robot::EnterIdleState(void) {
  chassis.Stop();
  robotState = ROBOT_IDLE;

  // Serial.println("req data");

  // message_RomiData data = message_RomiData_init_default;
  // data.has_gridLocation = true;
  // data.gridLocation.x = iGrid;
  // data.gridLocation.y = jGrid;
  // ESPInterface.sendProtobuf(data, message_RomiData_fields,
  //                           message_RomiData_size);
}

/**
 * Here is a good example of handling information differently, depending on the
 * state. If the Romi is not moving, we can update the bias (but be careful when
 * you first start up!). When it's moving, then we update the heading.
 */
void Robot::HandleOrientationUpdate(void) {
  prevEulerAngles = eulerAngles;
  if (robotState == ROBOT_IDLE) {
    imu.updateGyroBias();
    imu.updateAccBias();
  }

  else // update orientation
  {
    // ACCEL ANGLES
    float accX = (imu.a.x - imu.accBias.y) * imu.mgPerLSB / 1000;
    float accY = (imu.a.y - imu.accBias.x) * imu.mgPerLSB / 1000;
    float accZ = (imu.a.z - imu.accBias.z) * imu.mgPerLSB / 1000;

    float accPitch = atan2(-accX, accZ) * 180 / PI;
    float accRoll = atan2(accY, accZ) * 180 / PI;

    // GYRO ANGLES
    float sdt = 1. / (imu.gyroODR) * imu.mdpsPerLSB / 1000.;
    float gyroX = (imu.g.x - imu.gyroBias.x) * sdt + prevEulerAngles.x;
    float gyroY = (imu.g.y - imu.gyroBias.y) * sdt + prevEulerAngles.y;
    float gyroZ = (imu.g.z - imu.gyroBias.z) * sdt + prevEulerAngles.z;

    // COMPLEMENTARY FILTER
    float kappa = 0.01;
    eulerAngles.x = kappa * accPitch + (1 - kappa) * gyroX;
    eulerAngles.y = kappa * accRoll + (1 - kappa) * gyroY;
    eulerAngles.z = gyroZ; // accelerometer doesnt give yaw

    float epsilon = 0.0001;
    imu.gyroBias.x = imu.gyroBias.x - epsilon / sdt * (accPitch - gyroX);
    imu.gyroBias.y = imu.gyroBias.y - epsilon / sdt * (accRoll - gyroY);

    // Keep the heading between 0 and 360
    eulerAngles.z = fmod(eulerAngles.z, 360);
  }
}

void Robot::EnterRamping(float speed) {
  baseSpeed = speed;
  onRamp = false;
  robotState = ROBOT_RAMPING;
}

void Robot::RampingUpdate() {
  if (robotState == ROBOT_RAMPING) {
    LineFollowingUpdate(true);
    if (onRamp) {
      if (abs(eulerAngles.x) < 2) {
        EnterIdleState();
        onRamp = false;
      }
    } else {
      if (abs(eulerAngles.x) > 5) {
        onRamp = true;
      }
    }
  }
}

void Robot::HandleAprilTag(message_AprilTag &tag) {
  Serial.println("Tag ID: " + String(tag.id));

  if (tag.id != 0) {
    return;
  }

  if (robotState == ROBOT_SEARCHING) {
    SetLifter(180);
    robotState = ROBOT_GIMMIE_THAT_TAG;
    chassis.Stop();
  }

  if (robotState == ROBOT_GIMMIE_THAT_TAG) {
    float fwdErr = (95. - (float)tag.h);
    float turnErr = (60. - (float)tag.cx);

    float fwdEffort = -.2 * fwdErr;
    float turnEffort = -.035 * turnErr;

    if (abs(fwdErr) < 5 && abs(turnErr) < 2) {
      lastTagId = tag.id;
      EnterLiftingState();
      return;
    }

    chassis.SetTwist(fwdEffort, turnEffort);
  }
}

void Robot::EnterLiftingState(void) {
  robotState = ROBOT_LIFTING;
  liftingTimer.start(750);
  chassis.SetTwist(-10, 0);
}

void Robot::SetLifter(float position) {
  servo.setTargetPos(map(position, 0, 180, 1000, 2000));
}

void Robot::HandleWeight(int32_t avg) {
  Serial.print((avg - 267097) / 909.);
  EnterIdleState();
  // UNCOMMENT THIS WHEN DOING ATAG FR
  // LineFollowingUpdate(false);
}

/**
 * The main loop for your robot. Process both synchronous events (motor
 * control), and asynchronous events (IR presses, distance readings, etc.).
 */

void Robot::RobotLoop(void) {

  // if (robotState == ROBOT_IDLE && millis() > 5000) {
  //   if (!waiting) EnterIdleState();
  //   waiting = true;
    
  // }

  if (dataTimer.checkExpired()) {
    Serial.println("req data");

    message_RomiData data = message_RomiData_init_default;
    data.has_gridLocation = true;
    data.gridLocation.x = iGrid;
    data.gridLocation.y = jGrid;
    ESPInterface.sendProtobuf(data, message_RomiData_fields,
                              message_RomiData_size);
    dataTimer.start(1000);
  }

  /**
   * Check the Chassis timer, which is used for executing motor control
   */
  if (chassis.CheckChassisTimer()) {
    if (robotState == ROBOT_LINING)
      LineFollowingUpdate(false);

    if (robotState == ROBOT_RAMPING)
      RampingUpdate();

    if (robotState == ROBOT_TURNING && CheckTurnComplete())
      HandleTurnComplete();

    servo.update();
    int32_t reading = 0;
    if (loadCellHX1.GetReading(reading)) {

      if (robotState == ROBOT_WEIGHING) {
        HandleWeight(reading);
      }
    }

    if (robotState == ROBOT_LIFTING && liftingTimer.checkExpired()) {
      chassis.Stop();
      SetLifter(90);

      delay(1000);

      robotState = ROBOT_WEIGHING;
      loadCellIndex = 0;
    }

    chassis.UpdateMotors();

    if (robotState == ROBOT_LINING && lineSensor.CheckIntersection(false)) {
      HandleIntersection();
    }

    if (robotState == ROBOT_CENTERING && CheckCenteringComplete()) {
      EnterIdleState();
    }

    if (robotState == ROBOT_CENTERING && CheckCenteringComplete()) {
      EnterIdleState();
    }
  }
  /**
   * Check for an IMU update
   */
  if (imu.checkForNewData()) {
    HandleOrientationUpdate();
  }

  /**
   * Check for any messages from the ESP32
   */
  size_t msg_size;
  if (!ESPInterface.readUART(msg_size))
    return;

  message_AprilTag tag = message_AprilTag_init_default;
  if (msg_size == message_AprilTag_size) {
    if (!ESPInterface.readProtobuf(tag, message_AprilTag_fields))
      return;

    HandleAprilTag(tag);
  }

  message_ServerCommand data = message_ServerCommand_init_default;
  if (msg_size == message_ServerCommand_size) {

    // Decode the message from the Romi
    if (!ESPInterface.readProtobuf(data, message_ServerCommand_fields))
      return;

    Serial.println("rec data");

    if (robotState != ROBOT_IDLE)
      return;

    if (data.has_targetGridCell) {
      iTarget = data.targetGridCell.x;
      jTarget = data.targetGridCell.y;
    }

    if (data.has_state)
      Serial.println(data.state);

    switch (data.state) {
    case message_ServerCommand_State_IDLE:
      EnterIdleState();
      break;
    case message_ServerCommand_State_DRIVING:
      EnterLineFollowing(data.baseSpeed);
      break;
    case message_ServerCommand_State_LINING:
      baseSpeed = data.baseSpeed;
      robotState = ROBOT_CENTERING;
      HandleCenteringComplete();
      break;
    case message_ServerCommand_State_TURNING:
      EnterTurn(data.baseSpeed);
      break;
    case message_ServerCommand_State_RAMPING:
      EnterRamping(data.baseSpeed);
      break;
    case message_ServerCommand_State_SEARCHING:
      robotState = ROBOT_SEARCHING;
      chassis.SetTwist(0, data.baseSpeed);
      break;
    case message_ServerCommand_State_GIMMIE_THAT_TAG:
      break;
    case message_ServerCommand_State_TARGETING:
      break;
    case message_ServerCommand_State_WEIGHING:
      break;
    case message_ServerCommand_State_LIFTING:
      break;
    default:
      break;
    }
  }
}
