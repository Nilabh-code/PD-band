#pragma once
#include <OneWire.h>
#include <math.h>
#include "config.h"

namespace Temp {

OneWire one(PIN_TEMP);
uint8_t addr[8] = {0};
int mode = 0;
volatile float cached = NAN;
float offset_c = 0.0f;
volatile float dbg_adc = 0, dbg_r = 0;
bool sim = false;   // simulate probe when none detected (for bench testing)

void setSim(bool s) { sim = s; }

float readNTC() {
  uint32_t sum = 0;
  for (int i = 0; i < 8; i++) sum += analogRead(PIN_TEMP);
  float adc = sum / 8.0f;
  float ratio = adc / (4095.0f - adc + 1e-6f);
  float r = 10000.0f / ratio;
  dbg_adc = adc; dbg_r = r;
  float t = 1.0f / (logf(r / 10000.0f) / 3950.0f + 1.0f / 298.15f) - 273.15f;
  return t + offset_c;
}

bool detectDS18B20() {
  one.reset();
  one.reset_search();
  delay(30);
  if (!one.search(addr)) return false;
  if (OneWire::crc8(addr, 7) != addr[7]) return false;
  if (addr[0] != 0x28) return false;
  one.reset();
  one.select(addr);
  one.write(0x4E);
  one.write(0x1F); one.write(0xFF); one.write(0x3F);
  return true;
}

void begin() {
  analogReadResolution(12);
  if (detectDS18B20()) { mode = 1; return; }
  int samples[8], mn = 4095, mx = 0;
  float sum = 0;
  for (int i = 0; i < 8; i++) { samples[i] = analogRead(PIN_TEMP); sum += samples[i]; if (samples[i] < mn) mn = samples[i]; if (samples[i] > mx) mx = samples[i]; delay(3); }
  float mean = sum / 8.0f;
  bool stable = (mx - mn) < 120;
  bool inRange = mean > 150 && mean < 3950;
  if (stable && inRange) {
    float r = readNTC();
    if (isfinite(r) && r > 5 && r < 80) mode = 2;
    else mode = 0;
  } else mode = 0;
}

const char* typeName() {
  if (mode == 1) return "DS18B20";
  if (mode == 2) return "NTC";
  if (sim) return "SIM";
  return "none";
}

void poll() {
  static uint32_t last_redetect = 0;
  if (mode == 0 && millis() - last_redetect > 10000) {
    last_redetect = millis();
    pinMode(PIN_TEMP, INPUT_PULLUP);
    delay(2);
    int pu_adc = analogRead(PIN_TEMP);
    pinMode(PIN_TEMP, INPUT);
    delay(2);
    int off_adc = analogRead(PIN_TEMP);
    uint8_t pres = one.reset();
    Serial.printf("[temp] redetect: presence=%d adc_pullup=%d adc_float=%d\n", pres, pu_adc, off_adc);
    if (detectDS18B20()) { mode = 1; cached = NAN; Serial.println("[temp] DS18B20 found"); }
  }
  if (mode == 1) {
    static uint32_t last_conv = 0;
    static bool conv_started = false;
    uint32_t now = millis();
    if (!conv_started) {
      one.reset();
      one.select(addr);
      one.write(0x44, 1);
      conv_started = true;
      last_conv = now;
    } else if (now - last_conv > 120) {
      if (one.reset()) {
        one.select(addr);
        one.write(0xBE);
        uint8_t data[9];
        for (int i = 0; i < 9; i++) data[i] = one.read();
        int16_t raw = (data[1] << 8) | data[0];
        cached = (raw / 16.0f) + offset_c;
      }
      conv_started = false;
    }
  } else if (mode == 2) {
    static uint32_t last = 0;
    if (millis() - last > 500) { cached = readNTC(); last = millis(); }
  } else if (sim) {
    // Bench-test simulation: plausible body temp with slow sinusoidal drift.
    static uint32_t last = 0;
    if (millis() - last > 500) {
      float t = millis() / 1000.0f;
      // slow wander 36.3-37.1 C with a tiny bit of noise -> realistic body temp
      cached = 36.7f + 0.4f * sinf(t / 40.0f) + 0.05f * sinf(t / 3.7f);
      last = millis();
    }
  } else {
    cached = NAN;
  }
}

float get() { return cached; }
float rawAdc() { return dbg_adc; }
float rawR() { return dbg_r; }

}
