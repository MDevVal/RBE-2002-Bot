#include "LineSensor.h"

#define DARK_THRESHOLD 500;

void LineSensor::Initialize(void) {
  pinMode(leftSensorPin, INPUT);
  pinMode(rightSensorPin, INPUT);
}

int16_t LineSensor::CalcError(void) {
  int16_t left = analogRead(leftSensorPin);
  int16_t right = analogRead(rightSensorPin);
  return right - left;
}

bool LineSensor::CheckIntersection(void) {
  bool retVal = false;

  bool isLeftDark = analogRead(leftSensorPin) > DARK_THRESHOLD;
  bool isRightDark = analogRead(rightSensorPin) > DARK_THRESHOLD;

  bool onIntersection = isLeftDark && isRightDark;
  if (onIntersection && !prevOnIntersection)
    retVal = true;

  prevOnIntersection = onIntersection;

  return retVal;
}

int16_t LineSensor::ReadLeft(void) { return analogRead(leftSensorPin); }

int16_t LineSensor::ReadRight(void) { return analogRead(rightSensorPin); }
