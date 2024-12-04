#include "robot.h"
#include <IRdecoder.h>

void Robot::InitializeRobot(void)
{
    chassis.InititalizeChassis();

    /**
     * Initialize the IR decoder. Declared extern in IRdecoder.h; see robot-remote.cpp
     * for instantiation and setting the pin.
     */
    decoder.init();

    /**
     * Initialize the IMU and set the rate and scale to reasonable values.
     */
    imu.init();

    /**
     * TODO: Add code to set the data rate and scale of IMU (or edit LSM6::setDefaults())
     */

    // The line sensor elements default to INPUTs, but we'll initialize anyways, for completeness
    lineSensor.Initialize();
}

void Robot::EnterIdleState(void)
{
    Serial.println("-> IDLE");
    chassis.Stop();
    keyString = "";
    robotState = ROBOT_IDLE;
}

/**
 * Functions related to the IMU (turning; ramp detection)
 */
void Robot::EnterTurn(float angleInDeg)
{
    Serial.println(" -> TURN");
    robotState = ROBOT_TURNING;

    /**
     * TODO: Add code to initiate the turn and set the target
     */
}

bool Robot::CheckTurnComplete(void)
{
    bool retVal = false;

    /**
     * TODO: add a checker to detect when the turn is complete
     */

    return retVal;
}

void Robot::HandleTurnComplete(void)
{
    /**
     * TODO: Add code to handle the completed turn
     */
}

/**
 * Here is a good example of handling information differently, depending on the state.
 * If the Romi is not moving, we can update the bias (but be careful when you first start up!).
 * When it's moving, then we update the heading.
 */
void Robot::HandleOrientationUpdate(void)
{
    prevEulerAngles = eulerAngles;
    if(robotState == ROBOT_IDLE)
    {
        // TODO: You'll need to add code to LSM6 to update the bias
        imu.updateGyroBias();
    }

    else // update orientation
    {
        // TODO: update the orientation
    }

#ifdef __IMU_DEBUG__
    Serial.println(eulerAngles.z);
#endif
}

/**
 * Functions related to line following and intersection detection.
 */
void Robot::EnterLineFollowing(float speed) 
{
    Serial.println(" -> LINING"); 
    baseSpeed = speed; 
    robotState = ROBOT_LINING;
}

void Robot::LineFollowingUpdate(void)
{
    if(robotState == ROBOT_LINING) 
    {
        // TODO: calculate the error in CalcError(), calc the effort, and update the motion
        int16_t lineError = lineSensor.CalcError();
        float turnEffort = 0;

        chassis.SetTwist(baseSpeed, turnEffort);
    }
}

void Robot::HandleIntersection(void)
{
    Serial.print("X: \t");
    if(robotState == ROBOT_LINING) 
    {
        // move an extra 8cm
        delay(8 / baseSpeed * 1000);

        iGrid += (1 - currDirection) % 2;
        jGrid += (2 - currDirection) % 2;
        // Serial.print(" i grid: " + String(iGrid) + "\t, curr dir: \t" + String(currDirection));
        // Serial.println("\t j grid: " + String(jGrid));

        if (iGrid != iTarget)
        {
            targetDirection = (iGrid - iTarget) / abs(-iGrid + iTarget) + 1;
            EnterTurn(targetDirection - currDirection);

        }
        else if (jGrid != jTarget)
        {
            targetDirection = (jGrid - jTarget) / abs(-jGrid + jTarget) + 2;
            EnterTurn(targetDirection - currDirection);
        }
        else
        {
            EnterIdleState();
        }
    }
}

void Robot::RobotLoop(void) 
{
    /**
     * The main loop for your robot. Process both synchronous events (motor control),
     * and asynchronous events (IR presses, distance readings, etc.).
    */

    /**
     * Handle any IR remote keypresses.
     */
    int16_t keyCode = decoder.getKeyCode();
    if(keyCode != -1) HandleKeyCode(keyCode);

    /**
     * Check the Chassis timer, which is used for executing motor control
     */
    if(chassis.CheckChassisTimer())
    {
        // add synchronous, pre-motor-update actions here
        if(robotState == ROBOT_LINING)
        {
            LineFollowingUpdate();
        }

        chassis.UpdateMotors();

        // add synchronous, post-motor-update actions here

    }

    /**
     * Check for any intersections
     */
    if(robotState == ROBOT_LINING && lineSensor.CheckIntersection()) HandleIntersection();
    if(robotState == ROBOT_TURNING && TurnToAngle(targetHeading)) HandleTurnComplete();

    /**
     * Check for an IMU update
     */
    if(imu.checkForNewData())
    {
        HandleOrientationUpdate();
    }
}

