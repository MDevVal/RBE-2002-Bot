#include "LineSensor.h"

#define DARK_THRESHOLD 500;

#define LINE_THRESHOLD 110;
#define INTERSECTION_THRESHOLD 500;

void LineSensor::Initialize(void)
{
    for (int i = 0; i<sensorCount; i++) {
        pinMode(sensors[i], INPUT);
    }
}

float LineSensor::AverageReflectance(void) 
{ 
    float sum = 0;
    for (int i = 0; i<sensorCount; i++) {
        int ret = analogRead(sensors[i]);
        sum += ret;
    }
    return sum/sensorCount;
}

bool LineSensor::LineDetected() {
#ifdef DARK
    return AverageReflectance() > LINE_THRESHOLD;
#endif
#ifndef DARK
    return AverageReflectance() < 200;
#endif
}

float LineSensor::CalcError(void) 
{ 
    float sum_pos = 0;
    float sum = 0;
    for (int i = 0; i<sensorCount; i++) {
        int ret = analogRead(sensors[i]);
#ifndef DARK
        ret = 1000 - ret;
#endif
        sum_pos += ret * (i+1);
        sum += ret;
#ifdef __TRACK_DEBUG__
        Serial.print(ret);
        Serial.print(" ");
#endif
    }
    float pos = sum_pos / sum;
#ifdef __TRACK_DEBUG__
    Serial.print(AverageReflectance());
    Serial.print(" ");
    Serial.println(pos);
#endif
    return pos;
}
    

bool LineSensor::CheckIntersection(void)
{
#ifdef DARK
    return AverageReflectance() > INTERSECTION_THRESHOLD;
#endif
#ifndef DARK
    return AverageReflectance() < 100;
#endif
}
