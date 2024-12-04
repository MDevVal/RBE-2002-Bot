#include "robot.h"
// #include <IRdecoder.h>

void Robot::InitializeRobot(void)
{
    chassis.InititalizeChassis();

     /**
     * Initialize the IR decoder. Declared extern in IRdecoder.h; see robot-remote.cpp
     * for instantiation and setting the pin.
     */
    // decoder.init();

    /**
     * Initialize the IMU and set the rate and scale to reasonable values.
     */
    imu.init();

    /**
     * TODO: Add code to set the data rate and scale of IMU (or edit LSM6::setDefaults())
     */
    imu.setFullScaleGyro(imu.GYRO_FS500);
    imu.setGyroDataOutputRate(imu.ODR208);

    imu.setFullScaleAcc(imu.ACC_FS4);
    imu.setAccDataOutputRate(imu.ODR208);

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
void Robot::EnterTurn(int8_t numTurns)
{
    Serial.println(" -> TURN");
    robotState = ROBOT_TURNING;

    /**
     * TODO: Add code to initiate the turn and set the target
     */
    targetDirection = (numTurns + currDirection) % 4;
    targetHeading = fmod((eulerAngles.z + numTurns * 90), 360);
    chassis.SetTwist(0, 1 * abs(numTurns) / numTurns);
}

bool Robot::CheckTurnComplete(void)
{
    
    bool retVal = abs(eulerAngles.z - targetHeading) < 1.5;
    turnPIDCount += 1;
    if (!retVal) {
        turnPIDCount = 0;
        // PID (but only kP, also dont make it go too fast or gyro bad)
        float effort = .32 * (targetHeading - eulerAngles.z);
        chassis.SetTwist(0, effort/abs(effort) * min(abs(effort), 5));
    }

    return retVal && turnPIDCount > 2;
}

void Robot::HandleTurnComplete(void)
{
    /**
     * TODO: Add code to handle the completed turn
     */
    currDirection = targetDirection;
    // EnterIdleState();
    robotState = ROBOT_LINING;
    Serial.println("DONE TURNING");
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
        imu.updateAccBias();
    }

    else // update orientation
    {

        // ACCEL ANGLES
        float accX = (imu.a.x - imu.accBias.y) * imu.mgPerLSB / 1000;
        float accY = (imu.a.y - imu.accBias.x) * imu.mgPerLSB / 1000;
        float accZ = (imu.a.z - imu.accBias.z) * imu.mgPerLSB / 1000;
        
        float accPitch = atan2(-accX, accZ) * 180 / PI;
        float accRoll = atan2(accY, accZ) * 180 / PI;

        // GYRO ANGLES
        float sdt = 1. / (imu.gyroODR) * imu.mdpsPerLSB / 1000.;
        float gyroX = (imu.g.x - imu.gyroBias.x) * sdt  + prevEulerAngles.x;
        float gyroY = (imu.g.y - imu.gyroBias.y) * sdt + prevEulerAngles.y;
        float gyroZ = (imu.g.z - imu.gyroBias.z) * sdt + prevEulerAngles.z;

        
        // COMPLEMENTARY FILTER
        float kappa = 0.01;
        eulerAngles.x = kappa * accPitch + (1 - kappa) * gyroX;
        eulerAngles.y = kappa * accRoll + (1 - kappa) * gyroY;
        eulerAngles.z = gyroZ; // accelerometer doesnt give yaw

        float epsilon = 0.0001;
        imu.gyroBias.x = imu.gyroBias.x - epsilon / sdt * (accPitch - gyroX);
        imu.gyroBias.y = imu.gyroBias.y - epsilon / sdt * (accRoll - gyroY);

    
        // Keep the heading between 0 and 360
        eulerAngles.z = fmod(eulerAngles.z, 360);
        
        // Serial.println("pitch: \t" + String(pitch));
        // Serial.println("roll: \t" + String(roll));
    }

#ifdef __IMU_DEBUG__
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

void Robot::EnterRamping(float speed) {
    Serial.println(" -> RAMPING");
    baseSpeed = speed;
    onRamp = false;
    robotState = ROBOT_RAMPING;
}

void Robot::RampingUpdate(void) {
    if (robotState == ROBOT_RAMPING) {
        // Serial.println("doing ramp things: \t" + String(eulerAngles.x));
        LineFollowingUpdate();
        if (onRamp) {
            if (abs(eulerAngles.x) < 2) {
                EnterIdleState();
                onRamp = false;
                Serial.println("WE HAVE DESCENDED");
            }
        } else {
            if (abs(eulerAngles.x) > 5) {
                onRamp = true;
                Serial.println("FLYING TO THE GODS");
            }
        }

        digitalWrite(13, onRamp);
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
    // int16_t keyCode = decoder.getKeyCode();
    // if(keyCode != -1) HandleKeyCode(keyCode);

    /**
     * Check the Chassis timer, which is used for executing motor control
     */
    if(chassis.CheckChassisTimer())
    {
        // add synchronous, pre-motor-update actions here
        if(robotState == ROBOT_LINING) LineFollowingUpdate();
        if (robotState == ROBOT_RAMPING) RampingUpdate();

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

