/**
 * robot-remote.cpp implements features of class Robot that are related to
 * processing remote control commands. It also manages modes. You might want to
 * trim away some of the code that isn't needed later in the term.
 */
#include "robot.h"

#include <ir_codes.h>

/**
 * IRDecoder decoder is declared extern in IRdecoder.h (for ISR purposes),
 * so we _must_ name it decoder.
 */
#define IR_PIN 17
IRDecoder decoder(IR_PIN);

void Robot::HandleKeyCode(int16_t keyCode) {
  Serial.println(keyCode);

  // Regardless of current state, if ENTER is pressed, go to idle state
  if (keyCode == STOP_MODE) {
    EnterIdleState();
    if (robotCtrlMode == CTRL_AUTO) {
      robotAutonRoutine = ROUTINE_NONE;
    }
  }

  // If PLAY is pressed, it toggles control mode (setup -> teleop)
  else if (keyCode == PLAY_PAUSE) {
    if (robotCtrlMode == CTRL_AUTO) {
      EnterTeleopMode();
      EnterIdleState();
    } else if (robotCtrlMode == CTRL_TELEOP) {
      EnterAutoMode();
      EnterIdleState();
    }
  }

  /**
   * AUTO commands
   */
  if (robotCtrlMode == CTRL_AUTO) {
    switch (keyCode) {
    case NUM_1:
      robotAutonRoutine = ROUTINE_LINE_FOLLOWER;
      break;
    case NUM_2:
      robotAutonRoutine = ROUTINE_TIMED_LAP;
      break;
    case NUM_3:
      robotAutonRoutine = ROUTINE_MANHATTANER;
      break;
    case NUM_4:
      robotAutonRoutine = ROUTINE_NONE;
      EnterTurn(-90.f);
      break;
    case NUM_5:
      robotAutonRoutine = ROUTINE_RAMPER;
      break;
    case NUM_6:
      robotAutonRoutine = ROUTINE_CHICKEN_HEAD;
      break;
    }
  }
  /**
   * TELEOP commands
   */
  else if (robotCtrlMode == CTRL_TELEOP) {
    switch (keyCode) {
    case UP_ARROW:
      robotState = ROBOT_MANUAL;
      chassis.SetTwist(10, 0);
      break;
    case RIGHT_ARROW:
      robotState = ROBOT_MANUAL;
      chassis.SetTwist(0, 1);
      break;
    case DOWN_ARROW:
      robotState = ROBOT_MANUAL;
      chassis.SetTwist(-10, 0);
      break;
    case LEFT_ARROW:
      robotState = ROBOT_MANUAL;
      chassis.SetTwist(0, -1);
      break;
    case ENTER_SAVE:
      robotState = ROBOT_IDLE;
      chassis.SetTwist(0, 0);
      break;
    }
  }
}

void Robot::EnterTeleopMode(void) {
  chassis.Stop();
  Serial.println("-> TELEOP");
  robotCtrlMode = CTRL_TELEOP;
}

void Robot::EnterAutoMode(void) {
  Serial.println("-> AUTO");
  robotCtrlMode = CTRL_AUTO;
}

void Robot::EnterSetupMode(void) {
  Serial.println("-> SETUP");
  robotCtrlMode = CTRL_SETUP;
}
