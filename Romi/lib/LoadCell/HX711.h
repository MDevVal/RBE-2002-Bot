/**
 * A library for the HX711 load cell amplifier.
 * 
 * https://cdn.sparkfun.com/assets/b/f/5/a/e/hx711F_EN.pdf
 */
#pragma once

#include <Arduino.h>
#include <LoadCell.h>
#include <FastGPIO.h>

/**
 * Though the chip has clock and data pins, it is not using I2C. You can use most 
 * any pin for connections. We use a template so we can use Pololu's FastGPIO library.
 */

template <uint8_t clk, uint8_t dat> class HX711 : public LoadCellAmp
{
private:

    // For keeping track of the gain settings
    #define GAIN_A128   1
    #define GAIN_B32    2
    #define GAIN_A64    3

    uint8_t gain = GAIN_A128;

public:
    HX711(void) {}
    
    virtual bool Init(void) 
    {
        FastGPIO::Pin<clk>::setOutputLow();
        FastGPIO::Pin<dat>::setInput();

        return true;
    }

    uint8_t SetGain(uint8_t g) { if(g >= 1 && g <= 3) gain = g; return gain; }

    bool GetReading(int32_t& reading)
    {
        bool retVal = false;
        if(!FastGPIO::Pin<dat>::isInputHigh()) // LOW on the data line indicates new reading available
        {
            reading = ReadMeasurementAndCmdNextReading();
            retVal = true;
        }

        return retVal;
    }
    
    // The chip has simple sleep and wakeup protocols
    void Sleep(void)    { FastGPIO::Pin<clk>::setOutputValueHigh(); }
    void Wakeup(void)   { FastGPIO::Pin<clk>::setOutputValueLow(); }

private:
    /**
     * ReadMeasurementAndCmdNextReading is blocking, but writing wo/blocking would 
     * require setting up a timer and interrupts and it's not worth it for ~20us.
     */
    int32_t ReadMeasurementAndCmdNextReading(void)
    {   
        // 32-bit manipulation is slow, so use three bytes to collect data
        uint8_t highByte = 0;
        uint8_t medByte = 0;
        uint8_t lowByte = 0;

        for(uint8_t i = 0; i < 8; i++)
        {
            FastGPIO::Pin<clk>::setOutputValueHigh();

            // add a little stretch to the HIGH signal -- datasheet says minimum of 0.2 us
            // (4) nop's is ~250ns, plus the pin call puts us well over the minimum
            __asm__("nop");
            __asm__("nop");
            __asm__("nop");
            __asm__("nop");

            FastGPIO::Pin<clk>::setOutputValueLow();

            highByte <<= 1;
            if(FastGPIO::Pin<dat>::isInputHigh()) highByte++;
        }

        for(uint8_t i = 0; i < 8; i++)
        {
            FastGPIO::Pin<clk>::setOutputValueHigh();

            // add a little stretch to the HIGH signal
            __asm__("nop");
            __asm__("nop");
            __asm__("nop");
            __asm__("nop");

            FastGPIO::Pin<clk>::setOutputValueLow();

            medByte <<= 1;
            if(FastGPIO::Pin<dat>::isInputHigh()) medByte++;
        }

        for(uint8_t i = 0; i < 8; i++)
        {
            FastGPIO::Pin<clk>::setOutputValueHigh();

            // add a little stretch to the HIGH signal
            __asm__("nop");
            __asm__("nop");
            __asm__("nop");
            __asm__("nop");

            FastGPIO::Pin<clk>::setOutputValueLow();

            lowByte <<= 1;
            if(FastGPIO::Pin<dat>::isInputHigh()) lowByte++;
        }

        // command the next reading -- see datasheet for how gain is set
        for(uint8_t i = 0; i < gain; i++)
        {
            FastGPIO::Pin<clk>::setOutputHigh();
            FastGPIO::Pin<clk>::setOutputLow();
        }

        /**
         * Now assemble the result. Put into a 32-bit variable, _left aligned_ to maintain
         * sign. Then divide by 256, also to maintain sign.
         */
        int32_t value = ((uint32_t)highByte << 24) | ((uint32_t)medByte << 16) | ((uint32_t)lowByte << 8); 
        return value / 256;
    }

};