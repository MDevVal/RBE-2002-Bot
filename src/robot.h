#pragma once
#include "IRdecoder.h"
#include "chassis.h"
#include <LSM6.h>
#include <LineSensor.h>

class Robot {
protected:
  float prevError = 0.0;
  float lineKp = 1.5;
  float lineKp2 = 0;
  float lineKd = 0.0;
  float emergencyKp = 0.0;

  unsigned long lastLapTime = 0;
  float lapDistance = 523;
  float elapsedDistanceSetPoint = 0;
  unsigned long elapsedTime = 0;

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

  enum ROBOT_AUTON_ROUTINE {
    ROUTINE_NONE,
    ROUTINE_LINE_FOLLOWER,
    ROUTINE_TIMED_LAP,
    ROUTINE_MANHATTANER,
    ROUTINE_RAMPER,
    ROUTINE_CHICKEN_HEAD,
  };

  ROBOT_CTRL_MODE robotCtrlMode = CTRL_TELEOP;
  ROBOT_AUTON_ROUTINE robotAutonRoutine = ROUTINE_NONE;

  /**
   * robotState is used to track the current task of the robot. You will add new
   * states as the term progresses.
   */
  enum ROBOT_STATE {
    ROBOT_IDLE,
    ROBOT_LINING,
    ROBOT_TURNING,
    ROBOT_MANUAL,
    ROBOT_ARRIVED,
    ROBOT_TRACKING,
    ROBOT_SEARCHING
  };
  ROBOT_STATE robotState = ROBOT_IDLE;

  /* Define the chassis*/
  Chassis chassis;

  /* Line sensor */
  LineSensor lineSensor;

  /* Buzzer */

  /* To add later: rangefinder, camera, etc.*/

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

  float currentHeading;

  /* baseSpeed is used to drive at a given speed while, say, line following.*/
  float baseSpeed = 0;

  int8_t currDirection;
  int8_t targetDirection;
  long idleTime = 0;

  enum DIRECTION { EAST = 0, NORTH = 1, WEST = 2, SOUTH = 3 };

  /**
   * For tracking the motion of the Romi. We keep track of the intersection we
   * came from and the one we're headed to. You'll program in the map in
   * handleIntersection() and other functions.
   */
  enum INTERSECTION {
    NODE_START,
    NODE_1,
    NODE_2,
    NODE_3,
  };
  INTERSECTION nodeFrom = NODE_START;
  INTERSECTION nodeTo = NODE_1;

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

  void HandleAutonRoutine(ROBOT_AUTON_ROUTINE routine);

  /**
   * Line following and navigation routines.
   */
  void EnterLineFollowing(float speed);
  void LineFollowingUpdate(bool invert);

  void ManhattanerUpdate(void);
  void TimedLapUpdate(void);
  void RamperUpdate();
  void TrackingUpdate();
  void SearchingUpdate();

  void PrintLapStats();
  float GetDistanceElapsed();
  void ResetElapsedDistance();

  void HandleIntersection(void);

  void EnterTurn(float angle);
  bool CheckTurnComplete(void);
  void HandleTurnComplete(void);

  /* IMU routines */
  void HandleOrientationUpdate(void);

  /* For commanding the lifter servo */
  void SetLifter(uint16_t position);

  void FindAprilTags();
};
