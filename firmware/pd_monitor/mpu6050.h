#pragma once
#include <Wire.h>

#define MPU_ADDR 0x68

namespace MPU {

bool begin(TwoWire& w) {
  w.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
  w.beginTransmission(MPU_ADDR);
  if (w.endTransmission() != 0) return false;
  auto wr = [&](uint8_t r, uint8_t v) {
    w.beginTransmission(MPU_ADDR);
    w.write(r); w.write(v);
    w.endTransmission();
  };
  wr(0x6B, 0x00);
  wr(0x19, 0x07);
  wr(0x1A, 0x04);
  wr(0x1B, 0x00);
  wr(0x1C, 0x08);
  wr(0x23, 0x01);
  return true;
}

bool read(TwoWire& w, float& ax, float& ay, float& az, float& gx, float& gy, float& gz, float& t) {
  w.beginTransmission(MPU_ADDR);
  w.write(0x3B);
  if (w.endTransmission(false) != 0) return false;
  uint8_t n = w.requestFrom((int)MPU_ADDR, 14);
  if (n != 14) return false;
  int16_t b[7];
  for (int i = 0; i < 7; i++) {
    uint8_t hi = w.read(), lo = w.read();
    b[i] = (int16_t)((hi << 8) | lo);
  }
  ax = b[0] / 8192.0f; ay = b[1] / 8192.0f; az = b[2] / 8192.0f;
  gx = b[4] * 0.007629f; gy = b[5] * 0.007629f; gz = b[6] * 0.007629f;
  t = b[3] / 340.0f + 36.53f;
  return true;
}

}
