#pragma once

#include <Arduino.h>

class LoadCellAmp
{
private:

public:
    LoadCellAmp(void) {}
    virtual bool Init(void) = 0;
};