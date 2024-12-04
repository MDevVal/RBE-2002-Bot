#pragma once

#include <Arduino.h>

class PID {
public:
    /**
     * @brief Constructor for the PID controller.
     * 
     * Initializes the PID controller with the specified proportional (kp), integral (ki),
     * and derivative (kd) gains, the time step (dt) for calculations, and the tolerance for
     * the setpoint condition.
     * 
     * @param kp Proportional gain.
     * @param ki Integral gain.
     * @param kd Derivative gain.
     * @param dt Time step for calculations, used to compute derivative and integral terms.
     * @param tolerance The error tolerance for the system to consider as "at setpoint".
     */
    PID(float kp, float ki, float kd, float dt, float tolerance, uint8_t toleranceTime=1);

    /**
     * @brief Sets the PID gains.
     * 
     * Updates the proportional, integral, and derivative gains used in the PID controller.
     * 
     * @param kp New proportional gain.
     * @param ki New integral gain.
     * @param kd New derivative gain.
     */
    void setGains(float kp, float ki, float kd);

    /**
     * @brief Calculates the control output based on the PID algorithm.
     * 
     * This method computes the control output based on the current error (setpoint - current_value),
     * the PID gains, and the previous error. It also updates the integral and derivative terms.
     * 
     * @param setpoint The desired target value.
     * @param current_value The current measured value of the system.
     * @param dt The time step since the last calculation (optional; default is 0).
     * 
     * @return The control output based on the PID calculation.
     */
    float calculate(float setpoint, float current_value, float dt=0);

    /**
     * @brief Checks if the system is within tolerance of the setpoint.
     * 
     * This method compares the current error to the set tolerance. If the error is smaller
     * than the tolerance, it will return true, indicating the system has reached the setpoint.
     * 
     * @return True if the system is within tolerance of the setpoint, false otherwise.
     */
    bool isAtSetpoint(void);

    /**
     * @brief Resets the PID controller.
     * 
     * This method resets the integral term and the previous error, effectively clearing
     * any accumulated error or derivative calculations. This can be useful to restart the control process.
     */
    void reset();

private:
    float _kp;     ///< Proportional gain
    float _ki;     ///< Integral gain
    float _kd;     ///< Derivative gain
    float _dt;     ///< Time step (delta time)
    
    float _tolerance;       ///< Tolerance for error
    uint8_t _toleranceTime; ///< Time to be within tolerance (in cycles)
    uint8_t _toleranceCount; ///< Counter for tolerance time
    
    float _previous_error;  ///< Previous error for derivative term
    float _integral;        ///< Integral term

    float _output;           ///< Controller output
};
