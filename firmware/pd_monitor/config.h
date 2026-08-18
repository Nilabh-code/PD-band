#pragma once
#include <Arduino.h>
#include <Preferences.h>

#define FW_VERSION "1.0.0"

#define PIN_I2C_SDA 5
#define PIN_I2C_SCL 6
#define PIN_TEMP    1   // D0 = GPIO1 (1-Wire data, needs 2k-10k pull-up to 3V3)

#define SAMPLE_HZ 100
#define FEATURE_WINDOW_MS 2000
#define TREMOR_ALERT_THR 1.8f
#define FREEZE_ALERT_THR 0.55f
#define FALL_IMPACT_G 3.0f
#define TEMP_LOW_C 35.0f
#define TEMP_HIGH_C 38.0f
#define ALERT_COOLDOWN_MS 60000UL

enum Flag : uint16_t {
  F_TEMP_LOW    = 0x01,
  F_TEMP_HIGH   = 0x02,
  F_TREMOR      = 0x04,
  F_FREEZE      = 0x08,
  F_FALL        = 0x10,
  F_PROBE_ERR   = 0x20,
  F_WIFI_DOWN   = 0x80,
};

enum AlertType { A_NONE, A_TEMP_LOW, A_TEMP_HIGH, A_TREMOR, A_FREEZE, A_FALL };

struct Sample {
  uint32_t uptime_ms;
  time_t epoch;
  float temp_c;
  float g_rms;
  float tremor_band;
  float band_max;
  float freeze_score;
  float cv_pct;       // coefficient of variation % of RMS over rolling window
  float dom_freq;     // dominant frequency from FFT (Hz)
  float dom_amp;      // amplitude at dominant frequency (g)
  uint16_t flags;
};

struct DeviceConfig {
  // Leave blank for AP-only mode; set here or via the web Settings page.
  // Never commit real credentials to a public repo.
  char wifi_ssid[33] = "";
  char wifi_pass[65] = "";
  char tg_token[64] = "";
  char tg_chat[32] = "";
  char ai_base[160] = "https://api.tokenrouter.com/v1";
  char ai_key[128] = "";
  char ai_model[64] = "gpt-4o-mini";
  float temp_low = TEMP_LOW_C;
  float temp_high = TEMP_HIGH_C;
  float tremor_thr = TREMOR_ALERT_THR;

  void load() {
    Preferences p;
    p.begin("pdm", true);
    p.getString("ssid", wifi_ssid, sizeof(wifi_ssid));
    p.getString("pass", wifi_pass, sizeof(wifi_pass));
    p.getString("tgt", tg_token, sizeof(tg_token));
    p.getString("tgc", tg_chat, sizeof(tg_chat));
    p.getString("aib", ai_base, sizeof(ai_base));
    p.getString("aik", ai_key, sizeof(ai_key));
    p.getString("aim", ai_model, sizeof(ai_model));
    temp_low = p.getFloat("tlo", TEMP_LOW_C);
    temp_high = p.getFloat("thi", TEMP_HIGH_C);
    tremor_thr = p.getFloat("ttr", TREMOR_ALERT_THR);
    p.end();
  }

  void save() {
    Preferences p;
    p.begin("pdm", false);
    p.putString("ssid", wifi_ssid);
    p.putString("pass", wifi_pass);
    p.putString("tgt", tg_token);
    p.putString("tgc", tg_chat);
    p.putString("aib", ai_base);
    p.putString("aik", ai_key);
    p.putString("aim", ai_model);
    p.putFloat("tlo", temp_low);
    p.putFloat("thi", temp_high);
    p.putFloat("ttr", tremor_thr);
    p.end();
  }

  bool hasWifi() const { return wifi_ssid[0] != 0; }
  bool hasTelegram() const { return tg_token[0] != 0 && tg_chat[0] != 0; }
  bool hasAI() const { return ai_base[0] != 0; }
};
