#pragma once

#include <Arduino.h>

class LineSensor {
protected:
  byte reflectivityPins[6] = {A4, A3, A1, A6, A0, A11};
  bool prevOnIntersection = false;

public:
  LineSensor(void) {}
  void Initialize(void);
  int16_t CalcError(void);
  int16_t ReadLeft(void);
  int16_t ReadRight(void);
  bool CheckIntersection(bool invert = true);
};
