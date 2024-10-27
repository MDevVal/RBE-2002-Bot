/**
 * robot-remote.cpp implements features of class Robot that are related to
 * processing remote control commands. It also manages modes. You might want to
 * trim away some of the code that isn't needed later in the term.
 */
#include "robot.h"

#include <IRdecoder.h>
#include <ir_codes.h>

/**
 * IRDecoder decoder is declared extern in IRdecoder.h (for ISR purposes),
 * so we _must_ name it decoder.
 */
#define IR_PIN 17
IRDecoder decoder(IR_PIN);

float motorKp = 4.0;
float motorKi = 1.25;
float motorKd = 2.0; // 3.5;

void Robot::HandleKeyCode(int16_t keyCode) {
  Serial.println(keyCode);

  // Regardless of current state, if ENTER is pressed, go to idle state
  if (keyCode == STOP_MODE) {
    EnterIdleState();
  }

  // The SETUP key is used for tuning motor gains
  else if (keyCode == SETUP_BTN) {
    if (robotCtrlMode == CTRL_SETUP) {
      EnterTeleopMode();
      EnterIdleState();
    } else {
      EnterSetupMode();
      EnterIdleState();
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
    case REWIND:
      EnterLineFollowing(20);
      break;
    case NUM_0_10:
      Robot::baseSpeed += 5;
      break;
    case UP_ARROW:
      Robot::lineKp += 0.1;
      Serial.print("Line Kp = ");
      Serial.println(Robot::lineKp);
      break;
    case DOWN_ARROW:
      Robot::lineKp -= 0.1;
      Serial.print("Line Kp = ");
      Serial.println(Robot::lineKp);
      break;
    case RIGHT_ARROW:
      Robot::lineKd += 0.1;
      Serial.print("Line Kd = ");
      Serial.println(Robot::lineKd);
      break;
    case LEFT_ARROW:
      Robot::lineKd -= 0.1;
      Serial.print("Line Kd = ");
      Serial.println(Robot::lineKd);
      break;
    case VOLplus:
      Serial.println(chassis.GetDistanceElapsed());
      break;
    case ENTER_SAVE:
      PrintLapStats();
      break;
    case NUM_2:
    case NUM_3:
    case NUM_4:
    case NUM_5:
    case NUM_6:
    case NUM_7:
    case NUM_8:
    case NUM_9:
      break;
    }
  }

  /**
   * TELEOP commands
   */
  else if (robotCtrlMode == CTRL_TELEOP) {
    switch (keyCode) {
    case UP_ARROW:
      chassis.SetTwist(5, 0);
      break;
    case RIGHT_ARROW:
      chassis.SetTwist(0, -0.25);
      break;
    case DOWN_ARROW:
      chassis.SetTwist(-5, 0);
      break;
    case LEFT_ARROW:
      chassis.SetTwist(0, 0.25);
      break;
    case ENTER_SAVE:
      chassis.SetTwist(0, 0);
      break;
    }
  }

  /**
   * SETUP mode
   */

  else if (robotCtrlMode == CTRL_SETUP) {
    switch (keyCode) {
    case UP_ARROW:
      motorKp += 0.1;
      Serial.print("Motor Kp = ");
      Serial.println(motorKp);
      break;
    case DOWN_ARROW:
      motorKp -= 0.1;
      Serial.print("Motor Kp = ");
      Serial.println(motorKp);
      break;
    case RIGHT_ARROW:
      motorKi += 0.1;
      Serial.print("Motor Ki = ");
      Serial.println(motorKi);
      break;
    case LEFT_ARROW:
      motorKi -= 0.1;
      Serial.print("Motor Ki = ");
      Serial.println(motorKi);
      break;
    case REWIND:
      motorKd += 0.1;
      Serial.print("Motor Kd = ");
      Serial.println(motorKd);
      break;
    case NUM_0_10:
      motorKd -= 0.1;
      Serial.print("Motor Kd = ");
      Serial.println(motorKd);
      break;
    case PLAY_PAUSE:
      chassis.SetWheelSpeeds(20, 20);
      break;
    }

    chassis.SetMotorKp(motorKp);
    chassis.SetMotorKi(motorKi);
    chassis.SetMotorKd(motorKd);
  }
}

void Robot::EnterTeleopMode(void) {
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
