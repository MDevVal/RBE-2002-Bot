#pragma once

#include <Arduino.h>

class LineSensor
{
protected:
    const static uint8_t sensorCount = 6;
    const byte sensors[sensorCount] = {A0,A11,A2,A3,A4,A6};

    bool prevOnIntersection = false;

public:
    float AverageReflectance();

    LineSensor(void) {}
    void Initialize(void);
    float CalcError(void); // varies between 1 and 6
    bool CheckIntersection(void);
    bool LineDetected(void);
};
