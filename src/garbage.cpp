#include "Arduino.h"
#include "robot.h"

void Robot::SetLifter(uint16_t angle) {
  lifterServo.setTargetPos((angle * (1000 / 45)) + 1000);
}

const int bufferSize = 50;
int32_t circularBuffer[bufferSize] = {0};
int bufferIndex = 0;
int32_t sum = 0;
float average = 0;

void Robot::ReadLoadCell() {
  int32_t reading = 0;
  if (loadCellHX1.GetReading(reading)) {
    sum -= circularBuffer[bufferIndex];
    circularBuffer[bufferIndex] = reading;
    sum += reading;

    bufferIndex = (bufferIndex + 1) % bufferSize;

    average = sum / static_cast<float>(bufferSize);
  }
}

long reversingTime = 0;
long measuringTime = 0;
void Robot::PickupAndMeasureTrash() {
  if (reversingTime == 0)
    reversingTime = millis();

  if (millis() - reversingTime < 3000) {
    chassis.SetTwist(-5, 0);
    SetLifter(45);
  } else {
    chassis.SetTwist(0, 0);
    SetLifter(0);
    if (measuringTime == 0)
      measuringTime = millis();
  }

  if (millis() - measuringTime > 3000) {
    Serial.println((average * 0.001440265247) + 533.557256);
  }
}
