#pragma once

// Datasheet: https://www.pololu.com/file/0J1087/LSM6DS33.pdf

#include <Arduino.h>
// #include "LSM6Proto.pb.h"

class LSM6 {
public:
  template <typename T> struct vector {
    T x, y, z;
  };

  enum deviceType { device_DS33, device_auto };
  enum sa0State { sa0_low, sa0_high, sa0_auto };

  // register addresses
  enum regAddr {
    FUNC_CFG_ACCESS = 0x01,

    FIFO_CTRL1 = 0x06,
    FIFO_CTRL2 = 0x07,
    FIFO_CTRL3 = 0x08,
    FIFO_CTRL4 = 0x09,
    FIFO_CTRL5 = 0x0A,
    ORIENT_CFG_G = 0x0B,

    INT1_CTRL = 0x0D,
    INT2_CTRL = 0x0E,
    WHO_AM_I = 0x0F,
    CTRL1_XL = 0x10,
    CTRL2_G = 0x11,
    CTRL3_C = 0x12,
    CTRL4_C = 0x13,
    CTRL5_C = 0x14,
    CTRL6_C = 0x15,
    CTRL7_G = 0x16,
    CTRL8_XL = 0x17,
    CTRL9_XL = 0x18,
    CTRL10_C = 0x19,

    WAKE_UP_SRC = 0x1B,
    TAP_SRC = 0x1C,
    D6D_SRC = 0x1D,
    STATUS_REG = 0x1E,

    OUT_TEMP_L = 0x20,
    OUT_TEMP_H = 0x21,
    OUTX_L_G = 0x22,
    OUTX_H_G = 0x23,
    OUTY_L_G = 0x24,
    OUTY_H_G = 0x25,
    OUTZ_L_G = 0x26,
    OUTZ_H_G = 0x27,
    OUTX_L_XL = 0x28,
    OUTX_H_XL = 0x29,
    OUTY_L_XL = 0x2A,
    OUTY_H_XL = 0x2B,
    OUTZ_L_XL = 0x2C,
    OUTZ_H_XL = 0x2D,

    FIFO_STATUS1 = 0x3A,
    FIFO_STATUS2 = 0x3B,
    FIFO_STATUS3 = 0x3C,
    FIFO_STATUS4 = 0x3D,
    FIFO_DATA_OUT_L = 0x3E,
    FIFO_DATA_OUT_H = 0x3F,
    TIMESTAMP0_REG = 0x40,
    TIMESTAMP1_REG = 0x41,
    TIMESTAMP2_REG = 0x42,

    STEP_TIMESTAMP_L = 0x49,
    STEP_TIMESTAMP_H = 0x4A,
    STEP_COUNTER_L = 0x4B,
    STEP_COUNTER_H = 0x4C,

    FUNC_SRC = 0x53,

    TAP_CFG = 0x58,
    TAP_THS_6D = 0x59,
    INT_DUR2 = 0x5A,
    WAKE_UP_THS = 0x5B,
    WAKE_UP_DUR = 0x5C,
    FREE_FALL = 0x5D,
    MD1_CFG = 0x5E,
    MD2_CFG = 0x5F,
  };

  vector<int16_t> a; // accelerometer readings
  vector<int16_t> g; // gyro readings

  const float SIGMA = 0.90; // for updating bias

public:
  LSM6(void);

  bool init(deviceType device = device_auto, sa0State sa0 = sa0_auto);
  deviceType getDeviceType(void) { return _device; }

  void enableDefault(void);

  void writeReg(uint8_t reg, uint8_t value);
  uint8_t readReg(uint8_t reg);

  void readAcc(void);
  void readGyro(void);
  void read(void);

  void setTimeout(uint16_t timeout);
  uint16_t getTimeout(void);
  bool timeoutOccurred(void);

  uint8_t getStatus(void) { return readReg(LSM6::STATUS_REG); }

  bool checkForNewData(void);
  // LSM6Proto_LSM6Msg createProtoMessage(void);

  vector<float> updateGyroBias(void) {
    /**
     * TODO: add a low-pass filter to update the bias
     */
    // gyroBias.x = SIGMA * gyroBias.x + (1 - SIGMA) * g.x;
    // gyroBias.y = SIGMA * gyroBias.y + (1 - SIGMA) * g.y;
    gyroBias.z = SIGMA * gyroBias.z + (1 - SIGMA) * g.z;

    return gyroBias;
  }

  vector<float> updateAccBias(void) {
    /**
     * TODO: add a low-pass filter to update the bias
     */
    accBias.x = SIGMA * accBias.x + (1 - SIGMA) * a.x;
    accBias.y = SIGMA * accBias.y + (1 - SIGMA) * a.y;
    // accBias.z = SIGMA * accBias.z + (1 - SIGMA) * a.z;

    return gyroBias;
  }

protected:
  deviceType _device; // chip type
  uint8_t address;

  uint16_t io_timeout;
  bool did_timeout;

  int16_t testReg(uint8_t address, regAddr reg);

  // conversion factors are set when you change ODR or FS
  float mdpsPerLSB = 0;
  float mgPerLSB = 0;
  float accODR = 0;  // Hz
  float gyroODR = 0; // Hz

  uint8_t last_status; // status of last I2C transmission

  // for the complementary filter
  // float estimatedPitchAngle = 0;
  // vector<float> eulerAngles;
  vector<float> gyroBias;
  vector<float> accBias;

  /* We make Robot a friend to avoid all the setters and getters. */
  friend class Robot;
};
