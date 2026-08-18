#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include "config.h"
#include "mpu6050.h"
#include "detection.h"
#include "fft.h"
#include "temp.h"
#include "storage.h"
#include "comms.h"
#include "web_ui.h"

DeviceConfig cfg;
DetectionEngine engine;
Spectrum spectrum;
WebServer server(80);
TwoWire i2c(0);

#define CV_WINDOW 30  // 30 x 2s windows = 60s of RMS history for CV%
float rms_hist[CV_WINDOW];
uint8_t rms_hist_n = 0, rms_hist_idx = 0;
float cv_pct = 0;
uint8_t spec_bins[32];  // 0-12.5 Hz magnitude spectrum for the dashboard

volatile bool mpu_ok = false;
volatile float last_ax = 0, last_ay = 0, last_az = 0, last_mpu_t = 0;
Features lastFeats;       // last completed 2s window, for the dashboard
float lastDomFreq = 0, lastDomAmp = 0;
uint32_t last_alert_ms[8] = {0};
String active_alert;
uint32_t alert_until = 0;

bool ap_mode = false;
bool wifi_ready = false;
uint32_t last_wifi_check = 0;

void raiseAlert(AlertType t, const String& msg) {
  int idx = (int)t;
  uint32_t now = millis();
  if (now - last_alert_ms[idx] < ALERT_COOLDOWN_MS) return;
  last_alert_ms[idx] = now;
  active_alert = msg;
  alert_until = now + 30000;
  if (cfg.hasTelegram()) Comms::sendTelegram("[PD Monitor] " + msg);
  Serial.println("ALERT: " + msg);
}

void sampleTask(void* arg) {
  for (;;) {
    uint32_t t0 = micros();
    float ax, ay, az, gx, gy, gz, t;
    if (mpu_ok && MPU::read(i2c, ax, ay, az, gx, gy, gz, t)) {
      float hp = engine.push(ax, ay, az);
      spectrum.push(hp);
      last_ax = ax; last_ay = ay; last_az = az; last_mpu_t = t;
    }
    uint32_t dt = micros() - t0;
    if (dt < 10000) delayMicroseconds(10000 - dt);
    else delayMicroseconds(100);
  }
}

String stateJson() {
  Features fe = lastFeats;
  uint16_t flags = 0;
  float temp = Temp::get();
  bool plausible = isfinite(temp) && temp > 15.0f && temp < 45.0f;
  if (fe.ready) {
    if (fe.band_amp > cfg.tremor_thr) flags |= F_TREMOR;
    if (fe.fall) flags |= F_FALL;
    if (fe.freeze_ratio > 1.5f && fe.g_rms < 0.5f) flags |= F_FREEZE;
  }
  if (plausible) {
    if (temp < cfg.temp_low) flags |= F_TEMP_LOW;
    if (temp > cfg.temp_high) flags |= F_TEMP_HIGH;
  } else if (Temp::mode != 0) flags |= F_PROBE_ERR;
  if (!ap_mode && WiFi.status() != WL_CONNECTED) flags |= F_WIFI_DOWN;

  JsonDocument doc;
  doc["fw"] = FW_VERSION;
  doc["ttype"] = Temp::typeName();
  doc["sd"] = Store::size();
  doc["wifi"] = ap_mode ? "AP:" + String(WiFi.softAPSSID()) : WiFi.localIP().toString();
  doc["up"] = millis() / 1000;
  doc["temp"] = plausible ? temp : NAN;
  doc["rms"] = fe.ready ? fe.g_rms : 0;
  doc["band"] = fe.ready ? fe.band_amp : 0;
  doc["flags"] = flags;
  doc["ax"] = (float)last_ax; doc["ay"] = (float)last_ay; doc["az"] = (float)last_az;
  doc["mpu_t"] = (float)last_mpu_t;
  doc["cv"] = cv_pct;
  doc["df"] = lastDomFreq; doc["da"] = lastDomAmp;
  doc["p_adc"] = Temp::rawAdc(); doc["p_r"] = Temp::rawR();
  if (millis() < alert_until && active_alert.length()) doc["alert"] = active_alert;
  String out;
  serializeJson(doc, out);
  return out;
}

void handleState() { server.send(200, "application/json", stateJson()); }

void handleHistory() {
  JsonDocument doc;
  JsonArray ta = doc["temp"].to<JsonArray>();
  JsonArray ba = doc["band"].to<JsonArray>();
  JsonArray ca = doc["cv"].to<JsonArray>();
  JsonArray sa = doc["spec"].to<JsonArray>();
  for (uint16_t i = 0; i < Store::size(); i++) {
    const Sample& s = Store::at(i);
    ta.add(isfinite(s.temp_c) ? s.temp_c : 0);
    ba.add(s.tremor_band);
    ca.add(s.cv_pct);
  }
  for (int k = 0; k < 32; k++) sa.add(spec_bins[k]);
  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void handleChat() {
  if (!server.hasArg("plain")) { server.send(400, "application/json", "{\"e\":\"no body\"}"); return; }
  JsonDocument in;
  if (deserializeJson(in, server.arg("plain"))) { server.send(400, "application/json", "{\"e\":\"bad json\"}"); return; }
  String q = in["q"].as<String>();
  if (!q.length()) { server.send(400, "application/json", "{\"e\":\"empty\"}"); return; }

  float temp = Temp::get();
  Features fe = lastFeats;
  String sys = String("You are a health-assistant AI embedded in a wearable Parkinson's screening device (ankle-mounted).\n") +
    "Current readings: body_temp=" + (isfinite(temp) ? String(temp, 2) : String("n/a")) + "C, " +
    "motion_rms=" + String(fe.g_rms, 3) + "g, tremor_band(3-8Hz)=" + String(fe.band_amp, 3) + "g, " +
    "freeze_ratio=" + String(fe.freeze_ratio, 2) + ", fall_detected=" + String(fe.fall ? "yes" : "no") +
    ", gait_variability(CV%)=" + String(cv_pct, 1) +
    ", dominant_motion_freq=" + String(lastDomFreq, 2) + "Hz (amp " + String(lastDomAmp, 3) + "g).\n" +
    "Thresholds: low_temp=" + String(cfg.temp_low, 1) + "C, high_temp=" + String(cfg.temp_high, 1) +
    "C, tremor_thr=" + String(cfg.tremor_thr, 2) + "g.\n" +
    "Interpret findings in plain language, flag concerning patterns (hypothermia, tremor, freezing of gait, falls, " +
    "high gait variability CV%, rhythmic tremor peaks at 4-6Hz), " +
    "and ALWAYS recommend consulting a doctor. Never diagnose. Keep it short.";

  JsonDocument msgs;
  msgs[0]["role"] = "system";
  msgs[0]["content"] = sys;
  msgs[1]["role"] = "user";
  msgs[1]["content"] = q;

  String reply;
  bool ok = Comms::aiChat(msgs, reply);
  JsonDocument out;
  if (ok) out["a"] = reply; else out["e"] = reply;
  String resp;
  serializeJson(out, resp);
  server.send(200, "application/json", resp);
}

void handleConfigGet() {
  JsonDocument doc;
  doc["ssid"] = cfg.wifi_ssid;
  doc["tg_token_set"] = cfg.tg_token[0] != 0;
  doc["tg_token"] = cfg.tg_token[0] != 0 ? "*" : "";
  doc["tg_chat"] = cfg.tg_chat;
  doc["ai_base"] = cfg.ai_base;
  doc["ai_model"] = cfg.ai_model;
  doc["ai_key_set"] = cfg.ai_key[0] != 0;
  doc["temp_low"] = cfg.temp_low;
  doc["temp_high"] = cfg.temp_high;
  doc["tremor_thr"] = cfg.tremor_thr;
  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void handleConfigPost() {
  JsonDocument in;
  if (deserializeJson(in, server.arg("plain"))) { server.send(400, "application/json", "{\"ok\":false}"); return; }
  bool wifiChanged = false;
  auto str = [&](const char* k, char* dst, size_t n) {
    if (in[k].is<const char*>()) { strncpy(dst, in[k].as<const char*>(), n - 1); dst[n - 1] = 0; }
  };
  if (in["ssid"].is<const char*>()) {
    wifiChanged = strcmp(cfg.wifi_ssid, in["ssid"]) != 0 ||
                  (in["pass"].is<const char*>() && strcmp(cfg.wifi_pass, in["pass"]) != 0);
  }
  str("ssid", cfg.wifi_ssid, sizeof(cfg.wifi_ssid));
  str("pass", cfg.wifi_pass, sizeof(cfg.wifi_pass));
  str("tg_token", cfg.tg_token, sizeof(cfg.tg_token));
  str("tg_chat", cfg.tg_chat, sizeof(cfg.tg_chat));
  str("ai_base", cfg.ai_base, sizeof(cfg.ai_base));
  str("ai_model", cfg.ai_model, sizeof(cfg.ai_model));
  const char* k = in["ai_key"];
  if (k && strlen(k) > 0 && strcmp(k, "(set)") != 0) {
    strncpy(cfg.ai_key, k, sizeof(cfg.ai_key) - 1);
  }
  if (in["tlo"].is<float>()) cfg.temp_low = in["tlo"];
  if (in["thi"].is<float>()) cfg.temp_high = in["thi"];
  if (in["ttr"].is<float>()) cfg.tremor_thr = in["ttr"];
  cfg.save();
  server.send(200, "application/json", "{\"ok\":true}");
  if (wifiChanged) { delay(500); ESP.restart(); }
}

void handleLogs() {
  String out = Store::csv();
  server.sendHeader("Content-Disposition", "attachment; filename=pd_log.csv");
  server.send(200, "text/csv", out);
}

void handleLogView() {
  server.send(200, "text/plain", Store::tail(140));
}

void setupAP() {
  ap_mode = true;
  WiFi.mode(WIFI_AP);
  WiFi.softAP("PD_Monitor", "monitor123");
  Serial.println("AP mode: SSID=PD_Monitor pass=monitor123 -> http://192.168.4.1");
}

void setupWiFi() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("PD_Monitor", "monitor123");
  Serial.println("AP active: PD_Monitor/monitor123 (fallback access)");

  // Scan and connect by BSSID to match the exact SSID bytes (hotspots can
  // carry trailing spaces / hidden whitespace).
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
    Serial.printf(" failed (status=%d), AP fallback only, will retry\n", WiFi.status());
    return;
  }
  Serial.println(" ok: " + WiFi.localIP().toString());
  wifi_ready = true;
  configTzTime("UTC", "pool.ntp.org", "time.nist.gov");
  MDNS.begin("pdmonitor");
  MDNS.addService("http", "tcp", 80);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nPD Monitor " FW_VERSION);

  cfg.load();
  Comms::init(cfg);
  Store::begin();

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
  ap_mode = false;  // AP now runs alongside STA as fallback

  server.on("/", HTTP_GET, []() { server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/api/state", HTTP_GET, handleState);
  server.on("/api/history", HTTP_GET, handleHistory);
  server.on("/api/chat", HTTP_POST, handleChat);
  server.on("/api/config", HTTP_GET, handleConfigGet);
  server.on("/api/config", HTTP_POST, handleConfigPost);
  server.on("/api/logs", HTTP_GET, handleLogs);
  server.on("/api/log", HTTP_GET, handleLogView);
  server.begin();

  xTaskCreatePinnedToCore(sampleTask, "sam", 8192, NULL, 2, NULL, 1);
}

uint32_t last_log_ms = 0;

void loop() {
  server.handleClient();
  Temp::poll();

  if (millis() - last_log_ms >= FEATURE_WINDOW_MS) {
    last_log_ms = millis();
    Features fe = engine.peek();
    if (fe.ready) {
      Sample s;
      s.uptime_ms = millis();
      s.epoch = time(nullptr);
      float t = Temp::get();
      s.temp_c = (isfinite(t) && t > 15.0f && t < 45.0f) ? t : NAN;
      s.g_rms = fe.g_rms;
      s.tremor_band = fe.band_amp;
      s.band_max = fe.band_max;
      s.freeze_score = fe.freeze_ratio;
      s.flags = 0;

      if (isfinite(s.temp_c)) {
        if (s.temp_c < cfg.temp_low) { s.flags |= F_TEMP_LOW; raiseAlert(A_TEMP_LOW, "Body temp low: " + String(s.temp_c, 1) + "C"); }
        if (s.temp_c > cfg.temp_high) { s.flags |= F_TEMP_HIGH; raiseAlert(A_TEMP_HIGH, "Body temp high: " + String(s.temp_c, 1) + "C"); }
      }

      if (fe.band_amp > cfg.tremor_thr) { s.flags |= F_TREMOR; raiseAlert(A_TREMOR, "Tremor band spike: " + String(fe.band_amp, 3) + "g"); }
      if (fe.freeze_ratio > 1.5f && fe.g_rms < 0.5f && fe.band_amp > 0.03f) { s.flags |= F_FREEZE; raiseAlert(A_FREEZE, "Freezing-of-gait pattern"); }
      if (fe.fall) { s.flags |= F_FALL; raiseAlert(A_FALL, "Fall detected!"); }

      // FFT: full spectrum, dominant peak
      spectrum.analyze();
      s.dom_freq = spectrum.dominantFreq();
      s.dom_amp = spectrum.dominantAmp();
      for (int k = 0; k < 32; k++) {
        float m = spectrum.binAmp(k);
        spec_bins[k] = m > 2.54f ? 255 : (uint8_t)(m * 100.0f);
      }

      // CV% of RMS over the last 60s (stride-regularity proxy)
      rms_hist[rms_hist_idx] = s.g_rms;
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
      s.cv_pct = cv_pct;

      lastFeats = fe;
      lastFeats.ready = true;
      lastDomFreq = s.dom_freq;
      lastDomAmp = s.dom_amp;

      Store::append(s);
    }
    engine.clear();
  }

  if (cfg.hasWifi() && millis() - last_wifi_check > 30000) {
    last_wifi_check = millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi retry...");
      WiFi.disconnect();
      WiFi.begin(cfg.wifi_ssid, cfg.wifi_pass);
    } else if (!wifi_ready) {
      wifi_ready = true;
      Serial.println("WiFi up: " + WiFi.localIP().toString());
      configTzTime("UTC", "pool.ntp.org", "time.nist.gov");
      MDNS.begin("pdmonitor");
      MDNS.addService("http", "tcp", 80);
    }
  }
  delay(20);
}
