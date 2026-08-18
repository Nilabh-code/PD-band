#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include "config.h"
#include "mpu6050.h"
#include "detection.h"
#include "fft.h"
#include "temp.h"
#include "comms.h"

DeviceConfig cfg;
DetectionEngine engine;
Spectrum spectrum;
TwoWire i2c(0);

#define CV_WINDOW 30  // 30 x 2s windows = 60s of RMS history for CV%
float rms_hist[CV_WINDOW];
uint8_t rms_hist_n = 0, rms_hist_idx = 0;
float cv_pct = 0;
uint8_t spec_bins[32];  // 0-12.5 Hz magnitude spectrum for the dashboard

volatile bool mpu_ok = false;
volatile float last_ax = 0, last_ay = 0, last_az = 0, last_mpu_t = 0;
Features lastFeats;
float lastDomFreq = 0, lastDomAmp = 0;
uint32_t last_alert_ms[8] = {0};
String active_alert;
uint32_t alert_until = 0;

bool ap_mode = true;
bool wifi_ready = false;
uint32_t last_wifi_check = 0;

// Fall buzzer: 3 short beeps, non-blocking state machine.
static uint32_t buzz_end = 0;
static bool buzz_on = false;
static uint8_t buzz_reps = 0;

void beepStart() {
  buzz_reps = 3; buzz_on = true; buzz_end = millis() + 200;
  digitalWrite(PIN_BUZZER, HIGH);
}

void buzzerTick() {
  uint32_t now = millis();
  if (buzz_reps == 0) { digitalWrite(PIN_BUZZER, LOW); return; }
  if (now < buzz_end) return;
  if (buzz_on) {
    buzz_on = false; buzz_end = now + 200;
    digitalWrite(PIN_BUZZER, LOW);
  } else {
    buzz_on = true; buzz_end = now + 200; buzz_reps--;
    digitalWrite(PIN_BUZZER, HIGH);
  }
}

void raiseAlert(AlertType t, const String& msg) {
  int idx = (int)t;
  uint32_t now = millis();
  if (now - last_alert_ms[idx] < ALERT_COOLDOWN_MS) return;
  last_alert_ms[idx] = now;
  active_alert = msg;
  alert_until = now + 30000;
  if (t == A_FALL) beepStart();
  if (cfg.hasTelegram()) Comms::sendTelegram("[PD Monitor] " + msg);
  Serial.println("ALERT: " + msg);
}

void sampleTask(void* arg) {
  uint32_t sim_t_ms = 0;
  for (;;) {
    uint32_t t0 = micros();
    float ax, ay, az, gx, gy, gz, t;
    if (mpu_ok && MPU::read(i2c, ax, ay, az, gx, gy, gz, t)) {
      float hp = engine.push(ax, ay, az);
      spectrum.push(hp);
      last_ax = ax; last_ay = ay; last_az = az; last_mpu_t = t;
    } else if (!mpu_ok) {
      // Bench SIM motion: 5 Hz tremor burst + low rumble so the whole
      // stream + dashboard pipeline runs without the MPU connected.
      // Every ~75s inject a fake fall so buzzer + alert can be verified.
      static bool sim_falling = false;
      static uint32_t sim_fall_start = 0;
      static uint32_t sim_fall_next = 40000;
      uint32_t now = millis();
      sim_t_ms += 10;
      float ax, ay, az;
      if (!sim_falling && now >= sim_fall_next) {
        sim_falling = true; sim_fall_start = now;
      }
      if (sim_falling) {
        uint32_t ph = now - sim_fall_start;
        if (ph < 150) {        // impact impulse > FALL_IMPACT_G
          az = 4.5f; ax = 0; ay = 0;
        } else if (ph < 2000) {// still, tilted ~53deg so fall detector fires
          az = 0.6f; ax = 0; ay = 0.8f;
        } else {               // back upright
          az = 1.0f; ax = 0; ay = 0;
          sim_falling = false;
          sim_fall_next = now + 75000;
        }
      } else {
        float tm = sim_t_ms * 1e-3f;
        float trem = 0.25f * sinf(2.0f * PI * 5.0f * tm);
        float rum = 0.06f * sinf(2.0f * PI * 1.3f * tm);
        az = 1.0f + trem + rum; ax = 0; ay = 0;
      }
      float hp = engine.push(ax, ay, az);
      spectrum.push(hp);
      last_ax = ax; last_ay = ay; last_az = az; last_mpu_t = 0;
    }
    uint32_t dt = micros() - t0;
    if (dt < 10000) delayMicroseconds(10000 - dt);
    else delayMicroseconds(100);
  }
}

// Build the small JSON window and stream it to the PC host.
void streamToPC(const Features& fe, uint16_t flags, float temp, bool tempPlausible) {
  if (WiFi.status() != WL_CONNECTED) return;
  JsonDocument doc;
  doc["fw"] = FW_VERSION;
  doc["ttype"] = Temp::typeName();
  doc["epoch"] = time(nullptr);
  doc["uptime_ms"] = millis();
  doc["temp"] = tempPlausible ? temp : NAN;
  doc["rms"] = fe.g_rms;
  doc["band"] = fe.band_amp;
  doc["band_max"] = fe.band_max;
  doc["freeze"] = fe.freeze_ratio;
  doc["flags"] = flags;
  doc["cv"] = cv_pct;
  doc["df"] = lastDomFreq;
  doc["da"] = lastDomAmp;
  doc["ax"] = (float)last_ax; doc["ay"] = (float)last_ay; doc["az"] = (float)last_az;
  doc["p_adc"] = Temp::rawAdc(); doc["p_r"] = Temp::rawR();
  JsonArray spec = doc["spec"].to<JsonArray>();
  for (int k = 0; k < 32; k++) spec.add(spec_bins[k]);
  if (millis() < alert_until && active_alert.length()) doc["alert"] = active_alert;

  String body;
  serializeJson(doc, body);

  static bool busy = false;
  if (busy) return;
  busy = true;
  HTTPClient http;
  http.setTimeout(2000);
  if (http.begin(String("http://") + PC_HOST + ":" + String(PC_PORT) + PC_PATH)) {
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(body);
    if (code != 200 && Serial) Serial.printf("stream: HTTP %d\n", code);
    http.end();
  } else {
    if (Serial) Serial.println("stream: begin failed");
  }
  busy = false;
}

void setupAP() {
  ap_mode = true;
  WiFi.mode(WIFI_AP);
  WiFi.softAP("PD_Monitor", "monitor123");
  Serial.println("AP mode: SSID=PD_Monitor pass=monitor123");
}

// Scan and connect by BSSID so we match the exact hotspot SSID bytes.
bool connectSTA() {
  int n = WiFi.scanNetworks();
  int best = -1, bestRssi = -999;
  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i) == String(cfg.wifi_ssid) || WiFi.SSID(i).startsWith("Nilabh")) {
      if (WiFi.RSSI(i) > bestRssi) { bestRssi = WiFi.RSSI(i); best = i; }
    }
  }
  if (best >= 0) {
    Serial.printf("Found '%s' BSSID=%s RSSI=%d\n", WiFi.SSID(best).c_str(), WiFi.BSSIDstr(best).c_str(), bestRssi);
    WiFi.begin(WiFi.SSID(best).c_str(), cfg.wifi_pass, 0, WiFi.BSSID(best));
  } else {
    Serial.println("Scan: target not found, trying blind connect");
    WiFi.begin(cfg.wifi_ssid, cfg.wifi_pass);
  }
  Serial.print("Connecting");
  for (int i = 0; i < 80 && WiFi.status() != WL_CONNECTED; i++) {
    delay(250); Serial.print(".");
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.printf(" failed (status=%d)\n", WiFi.status());
    return false;
  }
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  Serial.println(" ok: " + WiFi.localIP().toString());
  wifi_ready = true;
  configTzTime("UTC", "pool.ntp.org", "time.nist.gov");
  return true;
}

void setupWiFi() {
  ap_mode = false;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("PD_Monitor", "monitor123");  // fallback access
  if (connectSTA()) ap_mode = false; else ap_mode = true;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nPD Monitor " FW_VERSION " (streamer, PC dashboard)");

  // 160 MHz is plenty for sampling + streaming and keeps current draw (and
  // brownout risk on marginal USB power) well below that at 240 MHz.
  setCpuFrequencyMhz(160);

  cfg.load();
  Comms::init(cfg);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  Temp::begin();
  if (strcmp(Temp::typeName(), "none") == 0) {
    Temp::setSim(true);
    Serial.println("Temp probe: none -> SIMulated bench mode");
  } else {
    Serial.printf("Temp probe: %s\n", Temp::typeName());
  }

  engine.begin(SAMPLE_HZ);
  spectrum.begin(SAMPLE_HZ);
  mpu_ok = MPU::begin(i2c);
  Serial.println(mpu_ok ? "MPU6050 ok" : "MPU6050 NOT FOUND");

  if (!cfg.hasWifi()) setupAP(); else setupWiFi();
  Serial.printf("Streaming to http://%s:%d%s\n", PC_HOST, PC_PORT, PC_PATH);

  // NOTE: must run at the SAME priority as the Arduino loop task (1) on the
  // same core, otherwise this busy-waiting sampler starves setup()/loop().
  xTaskCreatePinnedToCore(sampleTask, "sam", 8192, NULL, 1, NULL, 1);
}

uint32_t last_log_ms = 0;

void loop() {
  Temp::poll();
  buzzerTick();

  if (millis() - last_log_ms >= FEATURE_WINDOW_MS) {
    last_log_ms = millis();
    Features fe = engine.peek();
    if (fe.ready) {
      uint16_t flags = 0;
      float t = Temp::get();
      bool tempPlausible = isfinite(t) && t > 15.0f && t < 45.0f;

      if (tempPlausible) {
        if (t < cfg.temp_low) { flags |= F_TEMP_LOW; raiseAlert(A_TEMP_LOW, "Body temp low: " + String(t, 1) + "C"); }
        if (t > cfg.temp_high) { flags |= F_TEMP_HIGH; raiseAlert(A_TEMP_HIGH, "Body temp high: " + String(t, 1) + "C"); }
      }
      if (fe.band_amp > cfg.tremor_thr) { flags |= F_TREMOR; raiseAlert(A_TREMOR, "Tremor band spike: " + String(fe.band_amp, 3) + "g"); }
      if (fe.freeze_ratio > 1.5f && fe.g_rms < 0.5f && fe.band_amp > 0.03f) { flags |= F_FREEZE; raiseAlert(A_FREEZE, "Freezing-of-gait pattern"); }
      if (fe.fall) { flags |= F_FALL; raiseAlert(A_FALL, "Fall detected!"); }
      if (!ap_mode && WiFi.status() != WL_CONNECTED) flags |= F_WIFI_DOWN;

      // FFT: full spectrum, dominant peak
      spectrum.analyze();
      lastDomFreq = spectrum.dominantFreq();
      lastDomAmp = spectrum.dominantAmp();
      for (int k = 0; k < 32; k++) {
        float m = spectrum.binAmp(k);
        spec_bins[k] = m > 2.54f ? 255 : (uint8_t)(m * 100.0f);
      }

      // CV% of RMS over the last 60s (stride-regularity proxy)
      rms_hist[rms_hist_idx] = fe.g_rms;
      rms_hist_idx = (rms_hist_idx + 1) % CV_WINDOW;
      if (rms_hist_n < CV_WINDOW) rms_hist_n++;
      if (rms_hist_n > 1) {
        float sum = 0, sum2 = 0;
        for (int i = 0; i < rms_hist_n; i++) { sum += rms_hist[i]; sum2 += rms_hist[i] * rms_hist[i]; }
        float mean = sum / rms_hist_n;
        float var = sum2 / rms_hist_n - mean * mean;
        float sd = sqrtf(fmaxf(0.0f, var));
        cv_pct = mean > 0.05f ? (sd / mean) * 100.0f : 0.0f;
      }

      lastFeats = fe;
      lastFeats.ready = true;

      streamToPC(fe, flags, t, tempPlausible);
    }
    engine.clear();
  }

  if (cfg.hasWifi() && millis() - last_wifi_check > 30000) {
    last_wifi_check = millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi retry...");
      WiFi.disconnect();
      connectSTA();
    }
  }
  delay(20);
}