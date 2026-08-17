#pragma once
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "config.h"

namespace Comms {

DeviceConfig* cfgp = nullptr;
void init(DeviceConfig& c) { cfgp = &c; }

String escapeJson(const String& s) {
  String out;
  out.reserve(s.length() + 8);
  for (unsigned i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') { out += '\\'; out += c; }
    else if (c == '\n') out += "\\n";
    else out += c;
  }
  return "\"" + out + "\"";
}

bool sendTelegram(const String& text) {
  if (!cfgp || !cfgp->hasTelegram() || WiFi.status() != WL_CONNECTED) return false;
  WiFiClientSecure sec;
  sec.setInsecure();
  HTTPClient http;
  if (!http.begin(sec, String("https://api.telegram.org/bot") + cfgp->tg_token + "/sendMessage")) return false;
  http.addHeader("Content-Type", "application/json");
  String body = "{\"chat_id\":\"" + String(cfgp->tg_chat) + "\",\"text\":" + escapeJson(text) + "}";
  int code = http.POST(body);
  http.end();
  return code == 200;
}

// OpenAI-compatible chat completion.
// Returns true on success, response text in `reply`.
bool aiChat(const JsonDocument& messages, String& reply, uint16_t timeoutMs = 30000) {
  if (!cfgp || !cfgp->hasAI() || WiFi.status() != WL_CONNECTED) { reply = "AI API not configured (set base URL and key in Settings)."; return false; }
  String url = String(cfgp->ai_base);
  url.trim();
  if (url.endsWith("/")) url = url.substring(0, url.length() - 1);
  if (!url.endsWith("/chat/completions")) url += "/chat/completions";

  WiFiClientSecure sec;
  sec.setInsecure();
  sec.setTimeout(timeoutMs);
  HTTPClient http;
  if (!http.begin(sec, url)) { reply = "Connection to AI endpoint failed."; return false; }
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", String("Bearer ") + cfgp->ai_key);
  http.setTimeout(timeoutMs);

  JsonDocument req;
  req["model"] = cfgp->ai_model;
  req["messages"] = messages;
  req["temperature"] = 0.4;
  req["max_tokens"] = 400;
  String body;
  serializeJson(req, body);

  int code = http.POST(body);
  String resp = http.getString();
  http.end();

  if (code != 200) {
    reply = "AI API error " + String(code);
    return false;
  }
  JsonDocument doc;
  if (deserializeJson(doc, resp)) { reply = "AI API: bad response."; return false; }
  reply = doc["choices"][0]["message"]["content"].as<String>();
  return !reply.isEmpty();
}

}
