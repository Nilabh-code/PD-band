#!/usr/bin/env python3
"""PD-band PC host: receives sensor streams from the ESP32 and serves the dashboard.

Run:  python server.py [port]
Config is read from config.json (created with defaults on first run).

Endpoints:
  POST /data          -> ESP streams one feature window here
  GET  /              -> dashboard (dashboard.html)
  GET  /api/state     -> latest readings
  GET  /api/history   -> chart series
  GET  /api/log       -> recent rows as text
  GET  /api/logs      -> CSV download
  POST /api/chat      -> proxy to an OpenAI-compatible chat API
  GET/POST /api/config-> read/save AI + threshold settings
"""

import json
import os
import re
import socket
import sys
import time
import urllib.request
import urllib.error
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

BASE = os.path.dirname(os.path.abspath(__file__))
CONFIG_FILE = os.path.join(BASE, "config.json")
MAX_ROWS = 900  # 900 x 2s windows = 30 min


def default_config():
    return {
        "ai_base": "https://api.openai.com/v1",
        "ai_model": "gpt-4o-mini",
        "ai_key": "",
        "temp_low": 35.0,
        "temp_high": 38.0,
        "tremor_thr": 1.8,
        "tg_token": "",
        "tg_chat": "",
        "ssid": "",
        "pass": "",
    }


def load_config():
    if not os.path.exists(CONFIG_FILE):
        cfg = default_config()
        save_config(cfg)
        return cfg
    try:
        with open(CONFIG_FILE, "r", encoding="utf-8") as f:
            data = json.load(f)
        merged = default_config()
        merged.update(data)
        return merged
    except Exception:
        return default_config()


def save_config(cfg):
    with open(CONFIG_FILE, "w", encoding="utf-8") as f:
        json.dump(cfg, f, indent=2)


STATE = {"ts": 0.0, "data": None}
HISTORY = {"temp": [], "band": [], "cv": [], "spec": [], "rows": []}
CONFIG = load_config()
ALERT = {"text": "", "until": 0.0}


def keep(seq, item, cap=MAX_ROWS):
    seq.append(item)
    if len(seq) > cap:
        del seq[: len(seq) - cap]


def handle_data(payload):
    d = payload if isinstance(payload, dict) else {}
    now = time.time()
    STATE["ts"] = now
    STATE["data"] = d

    temp = d.get("temp")
    if not isinstance(temp, (int, float)) or not (15.0 < temp < 45.0):
        temp = float("nan")
    band = float(d.get("band", 0) or 0)
    cv = float(d.get("cv", 0) or 0)
    spec = d.get("spec")
    if not isinstance(spec, list):
        spec = []

    keep(HISTORY["temp"], temp)
    keep(HISTORY["band"], band)
    keep(HISTORY["cv"], cv)
    keep(HISTORY["spec"], spec)
    keep(HISTORY["rows"], d, cap=MAX_ROWS)

    alert = d.get("alert")
    if isinstance(alert, str) and alert:
        ALERT["text"] = alert
        ALERT["until"] = now + 30
    return {"ok": True}


def chat_proxy(q):
    base = (CONFIG.get("ai_base") or "").strip().rstrip("/")
    key = CONFIG.get("ai_key") or ""
    model = CONFIG.get("ai_model") or "gpt-4o-mini"
    if not base or not key:
        return None, "AI API not configured (set base URL and key in Settings)."

    url = base + "/chat/completions"
    sys_prompt = (
        "You are a health-assistant AI in a wearable Parkinson's screening "
        "device. Current readings: "
        + json.dumps(STATE["data"] or {})
        + ". Interpret findings in plain language, flag concerning patterns, "
        "and ALWAYS recommend consulting a doctor. Never diagnose. Keep it short."
    )
    body = json.dumps({
        "model": model,
        "temperature": 0.4,
        "max_tokens": 400,
        "messages": [
            {"role": "system", "content": sys_prompt},
            {"role": "user", "content": q},
        ],
    }).encode("utf-8")

    req = urllib.request.Request(url, data=body, headers={
        "Content-Type": "application/json",
        "Authorization": "Bearer " + key,
    })
    try:
        with urllib.request.urlopen(req, timeout=30) as r:
            j = json.loads(r.read().decode("utf-8"))
        return j["choices"][0]["message"]["content"], None
    except urllib.error.HTTPError as e:
        return None, "AI API error %d" % e.code
    except Exception as e:
        return None, "AI request failed: %s" % e


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def _send(self, code, ctype, body, extra=None):
        if isinstance(body, str):
            body = body.encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        if extra:
            for k, v in extra.items():
                self.send_header(k, v)
        self.end_headers()
        self.wfile.write(body)

    def _json(self, code, obj):
        self._send(code, "application/json", json.dumps(obj))

    def _read_body(self):
        length = int(self.headers.get("Content-Length") or 0)
        if length > 100000:
            length = 100000
        return self.rfile.read(length)

    def do_GET(self):
        path = self.path.split("?", 1)[0]
        if path == "/":
            return self._serve_dashboard()
        if path == "/api/state":
            return self._api_state()
        if path == "/api/history":
            return self._api_history()
        if path == "/api/log":
            return self._api_log()
        if path == "/api/logs":
            return self._api_logs()
        if path == "/api/config":
            return self._api_config_get()
        self._json(404, {"e": "not found"})

    def do_POST(self):
        path = self.path.split("?", 1)[0]
        if path == "/data":
            return self._api_data()
        if path == "/api/chat":
            return self._api_chat()
        if path == "/api/config":
            return self._api_config_post()
        self._json(404, {"e": "not found"})

    # ---- endpoints ----

    def _serve_dashboard(self):
        try:
            with open(os.path.join(BASE, "dashboard.html"), "rb") as f:
                self._send(200, "text/html; charset=utf-8", f.read())
        except FileNotFoundError:
            self._json(500, {"e": "dashboard.html missing"})

    def _api_data(self):
        try:
            payload = json.loads(self._read_body().decode("utf-8"))
        except Exception:
            return self._json(400, {"ok": False, "e": "bad json"})
        handle_data(payload)
        return self._json(200, {"ok": True})

    def _api_state(self):
        d = STATE["data"] or {}
        flags = int(d.get("flags", 0) or 0)
        out = {
            "fw": d.get("fw", "?"),
            "ttype": d.get("ttype", "?"),
            "sd": len(HISTORY["rows"]),
            "wifi": "pc-host",
            "up": int(time.time() - STATE["ts"]) if STATE["data"] else 0,
            "temp": d.get("temp"),
            "rms": d.get("rms", 0),
            "band": d.get("band", 0),
            "cv": d.get("cv", 0),
            "df": d.get("df", 0),
            "da": d.get("da", 0),
            "flags": flags,
            "ax": d.get("ax", 0),
            "ay": d.get("ay", 0),
            "az": d.get("az", 0),
            "p_adc": d.get("p_adc", 0),
            "p_r": d.get("p_r", 0),
        }
        if ALERT["until"] > time.time():
            out["alert"] = ALERT["text"]
        return self._json(200, out)

    def _api_history(self):
        out = {
            "temp": HISTORY["temp"][-MAX_ROWS:],
            "band": HISTORY["band"][-MAX_ROWS:],
            "cv": HISTORY["cv"][-MAX_ROWS:],
            "spec": HISTORY["spec"][-1] if HISTORY["spec"] else [],
        }
        return self._json(200, out)

    def _api_log(self):
        rows = HISTORY["rows"]
        if not rows:
            return self._send(200, "text/plain", "no data received yet\n")
        lines = []
        for d in rows[-140:]:
            lines.append(
                "%d,%d,%.2f,%.3f,%.3f,%.3f,%.2f,%.1f,%.2f,%.3f,%d" % (
                    d.get("epoch", 0),
                    d.get("uptime_ms", 0),
                    float(d.get("temp", 0) or 0),
                    float(d.get("rms", 0) or 0),
                    float(d.get("band", 0) or 0),
                    float(d.get("band_max", 0) or 0),
                    float(d.get("freeze", 0) or 0),
                    float(d.get("cv", 0) or 0),
                    float(d.get("df", 0) or 0),
                    float(d.get("da", 0) or 0),
                    int(d.get("flags", 0) or 0),
                )
            )
        return self._send(200, "text/plain", "\n".join(lines) + "\n")

    def _api_logs(self):
        header = "epoch,uptime_ms,temp_c,g_rms,band_rms,band_max,freeze_score,cv_pct,dom_freq,dom_amp,flags"
        lines = [header]
        for d in HISTORY["rows"]:
            lines.append(
                "%d,%d,%.2f,%.3f,%.3f,%.3f,%.2f,%.1f,%.2f,%.3f,%d" % (
                    d.get("epoch", 0),
                    d.get("uptime_ms", 0),
                    float(d.get("temp", 0) or 0),
                    float(d.get("rms", 0) or 0),
                    float(d.get("band", 0) or 0),
                    float(d.get("band_max", 0) or 0),
                    float(d.get("freeze", 0) or 0),
                    float(d.get("cv", 0) or 0),
                    float(d.get("df", 0) or 0),
                    float(d.get("da", 0) or 0),
                    int(d.get("flags", 0) or 0),
                )
            )
        self._send(200, "text/csv", "\n".join(lines) + "\n",
                   extra={"Content-Disposition": "attachment; filename=pd_log.csv"})

    def _api_chat(self):
        try:
            payload = json.loads(self._read_body().decode("utf-8"))
        except Exception:
            return self._json(400, {"e": "bad json"})
        q = payload.get("q", "")
        if not q:
            return self._json(400, {"e": "empty"})
        reply, err = chat_proxy(q)
        return self._json(200, {"a": reply} if reply else {"e": err})

    def _public_config(self):
        c = dict(CONFIG)
        c["ai_key"] = "*" if c.get("ai_key") else ""
        c["tg_token"] = "*" if c.get("tg_token") else ""
        c["ai_key_set"] = bool(c.get("ai_key"))
        c["tg_token_set"] = bool(c.get("tg_token"))
        return c

    def _api_config_get(self):
        return self._json(200, self._public_config())

    def _api_config_post(self):
        try:
            payload = json.loads(self._read_body().decode("utf-8"))
        except Exception:
            return self._json(400, {"ok": False})
        for k in ("ai_base", "ai_model", "temp_low", "temp_high", "tremor_thr",
                  "tg_chat", "tg_token", "ssid", "pass"):
            if k in payload:
                CONFIG[k] = payload[k]
        for k in ("ai_key", "tg_token"):
            v = payload.get(k)
            if v and v != "*":
                CONFIG[k] = v
        save_config(CONFIG)
        return self._json(200, {"ok": True})

    def log_message(self, fmt, *args):
        line = fmt % args
        if "/api/state" in line or "/api/history" in line or "/api/log" in line:
            return
        sys.stderr.write("%s %s\n" % (time.strftime("%H:%M:%S"), line))


def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
    server = ThreadingHTTPServer(("0.0.0.0", port), Handler)
    print("PD-band PC host listening on http://0.0.0.0:%d" % port)
    print("Dashboard:    http://localhost:%d" % port)
    print("ESP should POST /data to this machine's IP on port %d" % port)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
