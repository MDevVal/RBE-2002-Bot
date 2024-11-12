#include "robot.h"

void Robot::HandleAutonRoutine(ROBOT_AUTON_ROUTINE routine) {
  switch (routine) {
  case ROBOT_AUTON_ROUTINE::ROUTINE_NONE:
    if (robotState != ROBOT_IDLE) {
      EnterIdleState();
    }
    break;
  case ROBOT_AUTON_ROUTINE::ROUTINE_LINE_FOLLOWER:
    if (robotState != ROBOT_LINING) {
      EnterLineFollowing(10);
    }
    LineFollowingUpdate(false);
    break;
  case ROBOT_AUTON_ROUTINE::ROUTINE_MANHATTANER:
    ManhattanerUpdate();
    break;
  case ROBOT_AUTON_ROUTINE::ROUTINE_TIMED_LAP:
    TimedLapUpdate();
    break;
  case ROBOT_AUTON_ROUTINE::ROUTINE_RAMPER:
    RamperUpdate();
    break;
  }
}

void Robot::TimedLapUpdate() {
  if (robotState == ROBOT_IDLE) {
    EnterLineFollowing(10);
    elapsedTime = millis();
  }

  if (GetDistanceElapsed() > lapDistance) {
    EnterIdleState();
    ResetElapsedDistance();

    lastLapTime = (millis() - elapsedTime) / 1000;

    PrintLapStats();

    elapsedTime = 0;
  } else {
    LineFollowingUpdate(false);
  }
}

int currI = 0;
int currJ = 0;
int targetI = 1;
int targetJ = 0;
bool movingJWard = false;
bool movingIWard = false;
float elapsedAngle = 0;
bool flipTurns = false;
bool hasIMUCalibrate = false;
bool prevOnIntersection = false;

void Robot::ManhattanerUpdate() {
  switch (robotState) {
  case ROBOT_IDLE:
    if (currI < targetI) {
      EnterLineFollowing(20);
      currI++;
    } else if (currI > targetI) {
      if (!movingIWard) {
        EnterTurn(180.0f);
        elapsedAngle += 180;
        movingIWard = true;
        flipTurns = !flipTurns;
        return;
      }
      EnterLineFollowing(20);
      currI--;
    } else if (currJ < targetJ) {
      if (!movingJWard) {
        if (flipTurns) {
          EnterTurn(-90.0f);
          elapsedAngle -= 90;
        } else {
          EnterTurn(90.0f);
          elapsedAngle += 90;
        }
        movingJWard = true;
        return;
      }
      EnterLineFollowing(20);
      currJ++;
    } else if (currJ > targetJ) {
      if (!movingJWard) {
        if (flipTurns) {
          EnterTurn(90.0f);
          elapsedAngle += 90;
        } else {
          EnterTurn(-90.0f);
          elapsedAngle -= 90;
        }
        movingJWard = true;
        return;
      }
      EnterLineFollowing(20);
      currJ--;
    } else {
      if (movingIWard || movingJWard || flipTurns) {
        Serial.print("Arrived: ");
        Serial.print(targetI);
        Serial.print(", ");
        Serial.println(targetJ);
        EnterTurn(-elapsedAngle);
        movingIWard = false;
        movingJWard = false;
        flipTurns = false;
        elapsedAngle = 0;
        return;
      }
      // targetI = 0;
      // targetJ = 0;

      robotAutonRoutine = ROBOT_AUTON_ROUTINE::ROUTINE_RAMPER;

      Serial.print("New target: ");
      Serial.print(targetI);
      Serial.print(", ");
      Serial.println(targetJ);
    }

    break;
  case ROBOT_LINING:
    LineFollowingUpdate(true);

    if (lineSensor.CheckIntersection(true)) {
      elapsedDistanceSetPoint = chassis.GetDistanceElapsed();
      prevOnIntersection = true;
    }

    if (prevOnIntersection &&
        (chassis.GetDistanceElapsed() - elapsedDistanceSetPoint) >=
            (chassis.ROBOT_RADIUS - 2)) {
      prevOnIntersection = false;
      EnterIdleState();
      return;
    }
    break;
  case ROBOT_TURNING:
    if (CheckTurnComplete())
      HandleTurnComplete();
    break;
  }
}

bool ramping = false;
bool hasRamped = false;
bool isAligned = false;
void Robot::RamperUpdate() {
  switch (robotState) {
  case ROBOT_IDLE:
    if (!isAligned) {
      EnterTurn(90);
      isAligned = true;
      return;
    }
    if (!hasRamped) {
      EnterLineFollowing(20);
    }
    break;
  case ROBOT_TURNING:
    if (CheckTurnComplete())
      HandleTurnComplete();
    break;
  case ROBOT_LINING:
    LineFollowingUpdate(true);
    if (eulerAngles.y > 3.5) {
      digitalWrite(13, HIGH);
      ramping = true;
      hasRamped = true;
    } else {
      digitalWrite(13, LOW);
      ramping = false;
    }

    if (!ramping && hasRamped) {
      currI = 3;
      currJ = -1;
      targetI = 0;
      targetJ = 0;
      EnterIdleState();
      elapsedAngle = 90;
      robotAutonRoutine = ROBOT_AUTON_ROUTINE::ROUTINE_MANHATTANER;
    }

    break;
  }
}
