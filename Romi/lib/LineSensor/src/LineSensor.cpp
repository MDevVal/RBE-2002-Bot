#include "LineSensor.h"
#include "Arduino.h"

#define DARK_THRESHOLD 770;

void LineSensor::Initialize(void) {
  pinMode(reflectivityPins[0], INPUT);
  pinMode(reflectivityPins[1], INPUT);
  pinMode(reflectivityPins[2], INPUT);
  pinMode(reflectivityPins[3], INPUT);
  pinMode(reflectivityPins[4], INPUT);
  pinMode(reflectivityPins[5], INPUT);
}

int16_t LineSensor::ReadLeft(void) { return analogRead(reflectivityPins[2]); }

int16_t LineSensor::ReadRight(void) { return analogRead(reflectivityPins[3]); }

int16_t LineSensor::CalcError(void) {
  int16_t outerError =
      analogRead(reflectivityPins[0]) - analogRead(reflectivityPins[5]);
  int16_t middleError =
      analogRead(reflectivityPins[1]) - analogRead(reflectivityPins[4]);
  int16_t innerError =
      analogRead(reflectivityPins[2]) - analogRead(reflectivityPins[3]);

  return ((outerError * 0.70) + (middleError * 0.15) + (innerError * 0.15));
}

bool LineSensor::CheckIntersection(bool invert) {
  bool onIntersection = false;

  if (invert) {
    bool isLeftDark = analogRead(reflectivityPins[0]) < DARK_THRESHOLD;
    bool isRightDark = analogRead(reflectivityPins[5]) < DARK_THRESHOLD;

    onIntersection = isLeftDark && isRightDark;

  } else {
    bool isLeftDark = analogRead(reflectivityPins[0]) > DARK_THRESHOLD;
    bool isRightDark = analogRead(reflectivityPins[5]) > DARK_THRESHOLD;

    onIntersection = isLeftDark && isRightDark;
  }

  prevOnIntersection = onIntersection;
  return onIntersection;
}
