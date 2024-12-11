#pragma once
#include "chassis.h"
#include <Interface.h>
#include <LSM6.h>
#include <LineSensor.h>
#include <PID.h>
#include <servo32u4.h>
#include <HX711.h>
#include <message.pb.h>

class Robot {
protected:
  EventTimer centeringTimer = EventTimer();

  /**
   * We define some modes for you. SETUP is used for adjusting gains and so
   * forth. Most of the activities will run in AUTO. You shouldn't need to mess
   * with these.
   */
  enum ROBOT_CTRL_MODE {
    CTRL_TELEOP,
    CTRL_AUTO,
    CTRL_SETUP,
  };

  ROBOT_CTRL_MODE robotCtrlMode = CTRL_TELEOP;

  /**
   * robotState is used to track the current task of the robot. You will add new
   * states as the term progresses.
   */
  enum ROBOT_STATE {
    ROBOT_IDLE,
    ROBOT_DRIVING,
    ROBOT_LINING,
    ROBOT_TURNING,
    ROBOT_RAMPING,
    ROBOT_SEARCHING,
    ROBOT_GIMMIE_THAT_TAG,
    ROBOT_TARGETING,
    ROBOT_WEIGHING,
    ROBOT_LIFTING,
    ROBOT_CENTERING,
  };

  enum DIRECTION {
    NORTH = 1,
    EAST = 0,
    SOUTH = 3,
    WEST = 2,
  };

  ROBOT_STATE robotState = ROBOT_IDLE;

  /* Define the chassis*/
  Chassis chassis;

  /* Line sensor */
  LineSensor lineSensor;

  float prevError = 0.0;
  float lineKp = 1.5;
  float lineKd = 0.0;

  /* To add later: rangefinder, camera, etc.*/
  PID thetaPID = PID(.32, 0.0, 0.0, 20. / 1000., 1.5, 3); // already tuned

  Servo32U4Pin5 servo;

  HX711<4, 12> loadCellHX1;
  const static uint8_t numLoadCellReadings = 50;
  int32_t loadCellReading[numLoadCellReadings];
  uint8_t loadCellIndex = 0;
  EventTimer liftingTimer;

  uint8_t lastTagId = 255;

  // For managing key presses
  String keyString;

  /**
   * The LSM6 IMU that is included on the Romi. We keep track of the orientation
   * through Euler angles (roll, pitch, yaw).
   */
  LSM6 imu;
  LSM6::vector<float> prevEulerAngles;
  LSM6::vector<float> eulerAngles;

  /* targetHeading is used for commanding the robot to turn */
  float targetHeading;

  // direciton controls
  uint8_t turnPIDCount = 0;
  int8_t currDirection = EAST, targetDirection = 0;
  uint8_t iGrid = 2, jGrid = 2, iTarget = 1, jTarget = 1;

  // ramp controls
  bool onRamp = false;

  /* baseSpeed is used to drive at a given speed while, say, line following.*/
  float baseSpeed = 0;

  float startAngle = 0;
  float turnAngle = 0;

  Interface ESPInterface = Interface(Serial1);

public:
  Robot(void) {
    keyString.reserve(8);
  } // reserve some memory to avoid reallocation
  void InitializeRobot(void);
  void RobotLoop(void);

protected:
  /* For managing IR remote key presses*/
  void HandleKeyCode(int16_t keyCode);

  /* State changes */
  void EnterIdleState(void);

  /* Mode changes */
  void EnterTeleopMode(void);
  void EnterAutoMode(void);
  void EnterSetupMode(void);

  /**
   * Line following and navigation routines.
   */
  void EnterLineFollowing(float speed);
  void LineFollowingUpdate(bool invert = false);

  bool CheckIntersection(void) { return lineSensor.CheckIntersection(); }
  void HandleIntersection(void);

  void EnterTurn(int8_t numTurns);
  bool CheckTurnComplete(void);
  void HandleTurnComplete(void);

  void EnterRamping(float speed);
  void RampingUpdate(void);

  /* IMU routines */
  void HandleOrientationUpdate(void);

  /* Controls */
  void HandleAprilTag(message_AprilTag& tag);
  

  /* For commanding the lifter servo */
  void EnterLiftingState(void);
  void SetLifter(float position);
  void HandleWeight(int32_t avg);

  bool CheckCenteringComplete(void);

  void HandleCenteringComplete(void);
};
