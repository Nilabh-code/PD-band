#pragma once
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
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

}
