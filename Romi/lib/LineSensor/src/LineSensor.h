#pragma once

#include <Arduino.h>

class LineSensor {
protected:
  byte reflectivityPins[6] = {A0, A11, A2, A3, A4, A6};
  bool prevOnIntersection = false;

public:
  LineSensor(void) {}
  void Initialize(void);
  int16_t CalcError(void);
  int16_t ReadLeft(void);
  int16_t ReadRight(void);
  bool CheckIntersection(bool invert = true);
};
