#include "robot.h"

/**
 * Functions related to the IMU (turning; ramp detection)
 */
void Robot::EnterTurn(int8_t turns) {
  Serial.print("->TURN(");
  Serial.print(turns);
  Serial.println(')');
  robotState = ROBOT_TURNING;
  startAngle = eulerAngles.z;
  turnAngle = 90 * turns;
  if (turns > 0) {
    chassis.SetTwist(0, 0.5);
  } else if (turns < 0) {
    chassis.SetTwist(0, -0.5);
  }
}

// Now a proper checker...
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
    // Serial.print("dir: ");
    // Serial.println(currDirection);
    EnterLineFollowing(baseSpeed);
  }
}
/**
 * Functions related to line following and intersection detection.
 */
void Robot::EnterLineFollowing(float speed) {
  // Serial.println(" -> LINING");
  baseSpeed = speed;
  robotState = ROBOT_LINING;
}

void Robot::LineFollowingUpdate(bool invert) {
  if (robotState == ROBOT_LINING) {
    float lineError =
        (invert ? -lineSensor.CalcError() : lineSensor.CalcError()) / 1023.0;
    float derivative = (lineError - prevError);

    float turnEffort = lineError * lineKp + derivative * lineKd;

    // Serial.println(lineError)

    chassis.SetTwist(baseSpeed, turnEffort);
    prevError = lineError;
  }
}

void Robot::HandleIntersection(void) {
  // Serial.print("X -- ");
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
    // Serial.print("Now at: ");
    // Serial.print(iGrid);
    // Serial.print(',');
    // Serial.print(jGrid);
    // Serial.print('\n');
    /* Before we turn, we'll center the robot on the intersection. Creep at
    1.5cm/s for 3 secs. */
    chassis.SetTwist(10, 0);
    centeringTimer.start(800);
    robotState = ROBOT_CENTERING;
    // Serial.println("--> reached dest");
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
      if (iGrid == iTarget) // reached destination!
      {
        Serial.println("Reached Dest!");
        EnterIdleState();
        return;
      } else if (iGrid < iTarget) // we'll need to turn EAST
      {
        targetDirection = EAST;
      } else // need to go WEST
      {
        targetDirection = WEST;
      }
    } else if (jGrid < jTarget) {
      targetDirection = NORTH;
    } else {
      targetDirection = SOUTH;
    }
    if (currDirection == targetDirection) // we're headed in the right direction
    {
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
