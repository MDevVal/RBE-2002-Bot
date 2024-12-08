#include "robot.h"


bool Robot::TurnToAngle(float angle)
{
    float effort = thetaPID.calculate(angle, eulerAngles.z);
    chassis.SetTwist(0, constrain(effort, -5, 5));

    return thetaPID.isAtSetpoint();
}

