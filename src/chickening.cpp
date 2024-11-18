#include "openmv.h"
#include "robot.h"

OpenMV camera;

const int desired_cx = 100; // Desired center x-coordinate
const int desired_w = 50;   // Desired tag width for distance control
const float angularKp = 0.025f;
const float linearKp = 0.5f;

const unsigned long TAG_LOST_TIMEOUT = 2000;
unsigned long tagLostTime = 0;
bool tagWasVisible = true;

void Robot::TrackingUpdate() {
  AprilTagDatum tag;

  if (camera.checkUART(tag)) {
    tagWasVisible = true;
    tagLostTime = 0;

    int error_x = tag.cx - desired_cx;
    int error_w = desired_w - tag.w;

    float rotSpeed = angularKp * error_x;
    float forwardSpeed = -linearKp * error_w;

    chassis.SetTwist(forwardSpeed, rotSpeed);
  } else {
    if (tagWasVisible) {
      tagWasVisible = false;
      tagLostTime = millis();
      chassis.Stop();
    } else {
      unsigned long currentTime = millis();
      if (currentTime - tagLostTime >= TAG_LOST_TIMEOUT) {
        robotState = ROBOT_SEARCHING;
        Serial.println("Tag lost for 2 seconds. Switching to SEARCHING mode.");
      } else {
        chassis.Stop();
      }
    }
  }
}

void Robot::SearchingUpdate() {
  AprilTagDatum tag;

  float searchRotSpeed = 0.2f;
  float forwardSpeed = 0.0f;

  chassis.SetTwist(forwardSpeed, searchRotSpeed);

  if (camera.checkUART(tag)) {
    chassis.Stop();
    robotState = ROBOT_TRACKING;
  }
}

void Robot::FindAprilTags(void) {
  AprilTagDatum tag;
  if (camera.checkUART(tag)) {
    Serial.print(F("Tag [cx="));
    Serial.print(tag.cx);
    Serial.print(F(", cy="));
    Serial.print(tag.cy);
    Serial.print(F(", w="));
    Serial.print(tag.w);
    Serial.print(F(", h="));
    Serial.print(tag.h);
    Serial.print(F(", id="));
    Serial.print(tag.id);
    Serial.print(F(", rot="));
    Serial.print(tag.rot);
    Serial.println(F("]"));
  }
}
