#include "openmv.h"
#include "robot.h"

OpenMV camera;
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
  case ROBOT_AUTON_ROUTINE::ROUTINE_CHICKEN_HEAD:
    switch (robotState) {
    case ROBOT_TRACKING:
      Robot::TrackingUpdate();
      break;

    case ROBOT_SEARCHING:
      Robot::SearchingUpdate();
      break;
    }
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

void Robot::FindAprilTags(void) {
  AprilTagDatum tag;
  if (camera.checkUART(tag)) {
    Serial.print(F("Tag [cx="));
    Serial.print(tag.cx);
    Serial.print(F(", cy="));
    Serial.print(tag.cy);
    Serial.print(F(", w="));
    Serial.print(tag.w);
    Serial.print(F(", h="));
    Serial.print(tag.h);
    Serial.print(F(", id="));
    Serial.print(tag.id);
    Serial.print(F(", rot="));
    Serial.print(tag.rot);
    Serial.println(F("]"));
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

const int desired_cx = 100; // Desired center x-coordinate
const int desired_w = 50;   // Desired tag width for distance control
const float angularKp = 0.025f;
const float linearKp = 0.5f;

const unsigned long TAG_LOST_TIMEOUT = 2000;
unsigned long tagLostTime = 0;
bool tagWasVisible = true;

void Robot::TrackingUpdate() {
  AprilTagDatum tag;

  if (camera.checkUART(tag)) {
    tagWasVisible = true;
    tagLostTime = 0;

    int error_x = tag.cx - desired_cx;
    int error_w = desired_w - tag.w;

    float rotSpeed = angularKp * error_x;
    float forwardSpeed = -linearKp * error_w;

    chassis.SetTwist(forwardSpeed, rotSpeed);
  } else {
    if (tagWasVisible) {
      tagWasVisible = false;
      tagLostTime = millis();
      chassis.Stop();
    } else {
      unsigned long currentTime = millis();
      if (currentTime - tagLostTime >= TAG_LOST_TIMEOUT) {
        robotState = ROBOT_SEARCHING;
        Serial.println("Tag lost for 2 seconds. Switching to SEARCHING mode.");
      } else {
        chassis.Stop();
      }
    }
  }
}

void Robot::SearchingUpdate() {
  AprilTagDatum tag;

  float searchRotSpeed = 0.2f;
  float forwardSpeed = 0.0f;

  chassis.SetTwist(forwardSpeed, searchRotSpeed);

  if (camera.checkUART(tag)) {
    chassis.Stop();
    robotState = ROBOT_TRACKING;
  }
}
