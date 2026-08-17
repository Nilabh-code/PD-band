#pragma once
#include <math.h>
#include "config.h"

#define WIN_SAMPLES (SAMPLE_HZ * 2)

// One feature record per 2-second window, matching the log interval.
// tremor_band : amplitude of 3-8 Hz oscillation (Goertzel at 3/4/6/8 Hz on high-passed |a|)
// freeze_ratio: power(3-8Hz) / power(0.5-3Hz), classic freezing-of-gait index
//               -> high with small overall motion = shuffling-in-place
// fall        : impact > FALL_IMPACT_G followed by >1s of stillness and >30deg
//               gravity-vector reorientation

struct Features {
  bool ready;
  float g_rms;
  float band_amp;
  float band_max;
  float freeze_ratio;
  bool fall;
};

class DetectionEngine {
public:
  void begin(float rate) {
    rate_hz = rate;
    c_lo = 2.0f * cosf(2.0f * PI * 1.7f / rate);
    c3 = 2.0f * cosf(2.0f * PI * 3.0f / rate);
    c4 = 2.0f * cosf(2.0f * PI * 4.0f / rate);
    c6 = 2.0f * cosf(2.0f * PI * 6.0f / rate);
    c8 = 2.0f * cosf(2.0f * PI * 8.0f / rate);
  }

  // called at ~SAMPLE_HZ from the sampling task; returns the high-passed signal
  float push(float ax, float ay, float az) {
    float g = sqrtf(ax * ax + ay * ay + az * az);
    float hp = (g - prev_g) + 0.95f * hp_prev;
    prev_g = g; hp_prev = hp;

    buf[idx] = hp;
    idx = (idx + 1) % WIN_SAMPLES;
    g_sum += g * g; g_cnt++;

    // gravity direction (low-pass accel)
    lpx = lpx * 0.98f + ax * 0.02f;
    lpy = lpy * 0.98f + ay * 0.02f;
    lpz = lpz * 0.98f + az * 0.02f;

    checkFall(g, hp);

    if (++win_cnt >= WIN_SAMPLES) { win_cnt = 0; snapshot(); }
    return hp;
  }

  Features peek() { return feats; }
  void clear() { feats.ready = false; }

private:
  float goertzel(int n, float coef) {
    float s1 = 0, s2 = 0;
    for (int k = 0; k < WIN_SAMPLES; k++) {
      int i = (idx + k) % WIN_SAMPLES;
      float s0 = buf[i] + coef * s1 - s2;
      s2 = s1; s1 = s0;
    }
    float mag2 = s1 * s1 + s2 * s2 - coef * s1 * s2;
    return sqrtf(fmaxf(0.0f, mag2)) * 2.0f / WIN_SAMPLES;
  }

  void snapshot() {
    feats.g_rms = g_cnt > 0 ? sqrtf(g_sum / g_cnt) : 0.0f;
    float e3 = goertzel(WIN_SAMPLES, c3);
    float e4 = goertzel(WIN_SAMPLES, c4);
    float e6 = goertzel(WIN_SAMPLES, c6);
    float e8 = goertzel(WIN_SAMPLES, c8);
    float elo = goertzel(WIN_SAMPLES, c_lo);
    float hi2 = e3 * e3 + e4 * e4 + e6 * e6 + e8 * e8;
    float lo2 = elo * elo + 1e-6f;
    feats.band_amp = sqrtf(hi2) / 2.0f;
    feats.band_max = fmaxf(fmaxf(e3, e4), fmaxf(e6, e8));
    feats.freeze_ratio = feats.band_amp > 0.02f ? hi2 / lo2 : 0.0f;
    feats.fall = fall_flag;
    feats.ready = true;

    g_sum = 0; g_cnt = 0; fall_flag = false;
    band_seen = true;
  }

  void checkFall(float g, float hp) {
    if (!impact_seen && g > FALL_IMPACT_G) {
      impact_seen = true;
      impact_ms_cache = millis();
      gx0 = lpx; gy0 = lpy; gz0 = lpz;
      still_ms = 0;
    }
    if (impact_seen) {
      if (fabsf(hp) < 0.15f) still_ms += 1000.0f / SAMPLE_HZ;
      else still_ms = 0;
      if (millis() - impact_ms_cache > 4000) { impact_seen = false; return; }
      if (still_ms >= 1000) {
        float im = sqrtf(gx0 * gx0 + gy0 * gy0 + gz0 * gz0) + 1e-3f;
        float cm = sqrtf(lpx * lpx + lpy * lpy + lpz * lpz) + 1e-3f;
        float dot = (gx0 * lpx + gy0 * lpy + gz0 * lpz) / (im * cm);
        dot = fminf(1.0f, fmaxf(-1.0f, dot));
        if (acosf(dot) * 57.2958f > 30.0f) { fall_flag = true; }
        impact_seen = false;
      }
    }
  }

  float rate_hz = 100;
  float c_lo = 0, c3 = 0, c4 = 0, c6 = 0, c8 = 0;
  float buf[WIN_SAMPLES] = {0};
  int idx = 0;
  uint32_t win_cnt = 0;
  float prev_g = 0, hp_prev = 0;
  float lpx = 0, lpy = 0, lpz = 1.0f;
  float gx0 = 0, gy0 = 0, gz0 = 1.0f;
  float g_sum = 0;
  uint32_t g_cnt = 0;
  bool impact_seen = false, fall_flag = false, band_seen = false;
  float still_ms = 0;
  uint32_t impact_ms_cache = 0;
  Features feats = {};
};
