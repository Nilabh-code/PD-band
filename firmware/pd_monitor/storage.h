#pragma once
#include "config.h"
#include <Arduino.h>

// In-RAM rolling data log. Replaces the SD card (which proved unreliable
// on this hardware) while keeping the dashboard's Data Log tab and CSV
// download fully functional. Capacity here matches the flash-free ring.

namespace Store {

#define STORE_CAP 900  // 900 x 2s windows = 30 min of history kept in RAM

Sample rows[STORE_CAP];
uint16_t head = 0;
uint16_t count = 0;

void begin() {
  head = 0;
  count = 0;
}

void append(const Sample& s) {
  rows[head] = s;
  head = (head + 1) % STORE_CAP;
  if (count < STORE_CAP) count++;
}

size_t size() { return count; }

// 0-indexed in chronological order.
const Sample& at(uint16_t i) {
  uint16_t idx = (head - count + STORE_CAP + i) % STORE_CAP;
  return rows[idx];
}

const char* header() { return "epoch,uptime_ms,temp_c,g_rms,band_rms,band_max,freeze_score,cv_pct,dom_freq,dom_amp,flags"; }

String line(const Sample& s) {
  char buf[112];
  snprintf(buf, sizeof(buf),
           "%ld,%lu,%.2f,%.3f,%.3f,%.3f,%.2f,%.1f,%.2f,%.3f,%u\n",
           (long)s.epoch, (unsigned long)s.uptime_ms, s.temp_c,
           s.g_rms, s.tremor_band, s.band_max, s.freeze_score,
           s.cv_pct, s.dom_freq, s.dom_amp, s.flags);
  return String(buf);
}

// Full CSV for the download endpoint.
String csv() {
  String out;
  out.reserve(count * 64 + 64);
  out += header();
  out += '\n';
  for (uint16_t i = 0; i < count; i++) out += line(at(i));
  return out;
}

// Last `n` rows as plain text for the /api/log view.
String tail(size_t n) {
  String out;
  size_t start = count > n ? count - n : 0;
  for (size_t i = start; i < count; i++) out += line(at((uint16_t)i));
  return out;
}

}