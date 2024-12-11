#include "robot.h"

/**
 * Functions related to the IMU (turning; ramp detection)
 */
void Robot::EnterTurn(int8_t turns) {
  robotState = ROBOT_TURNING;
  startAngle = eulerAngles.z;
  turnAngle = 90 * turns;
  if (turns > 0) {
    chassis.SetTwist(0, 0.5);
  } else if (turns < 0) {
    chassis.SetTwist(0, -0.5);
  }
}

bool Robot::CheckTurnComplete(void) {
  bool retVal = false;
  static bool prevPast = false;
  bool past = false;
  if (turnAngle > 0) {
    past = (eulerAngles.z - startAngle > turnAngle);
    if (past && !prevPast)
      retVal = true;
  } else {
    past = (eulerAngles.z - startAngle < turnAngle);
    if (past && !prevPast)
      retVal = true;
  }
  prevPast = past;
  return retVal;
}
void Robot::HandleTurnComplete(void) {
  if (robotState == ROBOT_TURNING) {
    currDirection = targetDirection;
    EnterLineFollowing(baseSpeed);
  }
}
/**
 * Functions related to line following and intersection detection.
 */
void Robot::EnterLineFollowing(float speed) {
  baseSpeed = speed;
  robotState = ROBOT_LINING;
}

void Robot::LineFollowingUpdate(bool invert) {
  float lineError =
      (invert ? -lineSensor.CalcError() : lineSensor.CalcError()) / 1023.0;
  float derivative = (lineError - prevError);

  float turnEffort = lineError * lineKp + derivative * lineKd;

  chassis.SetTwist(baseSpeed, turnEffort);
  prevError = lineError;
}

void Robot::HandleIntersection(void) {
  if (robotState == ROBOT_LINING) {
    switch (currDirection) {
    case EAST:
      iGrid++;
      break;
    case NORTH:
      jGrid++;
      break;
    case WEST:
      iGrid--;
      break;
    case SOUTH:
      jGrid--;
      break;
    default:
      break;
    }
    chassis.SetTwist(10, 0);
    centeringTimer.start(800);
    robotState = ROBOT_CENTERING;
  }
}
bool Robot::CheckCenteringComplete(void) {
  return centeringTimer.checkExpired();
}
void Robot::HandleCenteringComplete(void) {
  if (robotState == ROBOT_CENTERING) {
    /**
     * Now that we're centered, we can work through the logic of where to go.
     *
     * I'll drive to the correct j first, then i.
     */
    if (jGrid == jTarget) {
      if (iGrid == iTarget) {
        EnterIdleState();
        return;
      } else if (iGrid < iTarget) {
        targetDirection = EAST;
      } else {
        targetDirection = WEST;
      }
    } else if (jGrid < jTarget) {
      targetDirection = NORTH;
    } else {
      targetDirection = SOUTH;
    }
    if (currDirection == targetDirection) {
      EnterLineFollowing(baseSpeed);
    } else {
      int8_t turnCount = targetDirection - currDirection;
      // take the shortest path
      if (turnCount > 2)
        turnCount -= 4;
      if (turnCount < -2)
        turnCount += 4;
      EnterTurn(turnCount);
    }
  }
}
