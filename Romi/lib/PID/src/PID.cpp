#include "PID.h"


PID::PID(float kp, float ki, float kd, float dt, float tolerance, uint8_t toleranceTime=1)
    : _kp(kp), _ki(ki), _kd(kd), _dt(dt), _tolerance(tolerance),_toleranceTime(toleranceTime), _toleranceCount(0), _previous_error(0), _integral(0), _output(0) {}

void PID::setGains(float kp, float ki, float kd) {
    _kp = kp;
    _ki = ki;
    _kd = kd;
}

float PID::calculate(float setpoint, float current_value, float dt = 0) {
    if (dt == 0) dt = _dt;

    // Compute the error
    float error = setpoint - current_value;

    // Proportional term
    float proportional = _kp * error;

    // Integral term
    _integral += error * dt;
    float integral = _ki * _integral;

    // Derivative term
    float derivative = _kd * (error - _previous_error) / dt;

    // Compute the output
    _output = proportional + integral + derivative;

    // Update previous error for next derivative calculation
    _previous_error = error;

    return _output;
}

/* 
 * Must be called AFTER calculate()
 */
bool PID::isAtSetpoint(void) {
    bool ret = abs(_previous_error) < _tolerance;
    if (ret) _toleranceCount++;
    else _toleranceCount = 0;

    return _toleranceCount >= _toleranceTime;
}

void PID::reset() {
    _previous_error = 0;
    _integral = 0;
    _output = 0;
    _toleranceCount = 0;
}
