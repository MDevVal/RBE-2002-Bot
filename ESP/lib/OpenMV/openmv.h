#ifndef __CAMERA_H
#define __CAMERA_H

#include <Arduino.h>

#include <apriltagdatum.h>

class Camera {};

class OpenMV : public Camera
{
protected:
    uint8_t mvArray[sizeof(AprilTagDatum)]; //array for receiving data from the OpenMV cam
    uint8_t mvIndex = 0; //for counting bytes

public:

    bool checkUART(AprilTagDatum& tag);
    bool handleUART(uint8_t b);

    OpenMV(HardwareSerial& port) : port(port) {}

private:
    HardwareSerial& port;
};

#endif
