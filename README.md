# PD-band

A wearable **Parkinson's disease early-screening device** built on the **Seeed Studio XIAO ESP32-S3 Sense**. It monitors body temperature and ankle motion (via an MPU6050 IMU), logs everything to an SD card, and serves a live **glass-morphism web dashboard** with an **AI chat assistant** that interprets the findings. It raises **alerts** (web banner + Telegram) on anomalies such as abnormal body temperature, tremor-band spikes, freezing of gait, and falls.

> ⚠️ **This is a research / screening aid, not a medical device.** It does not diagnose. All findings include a recommendation to consult a doctor.

---

## What it measures

| Signal | Sensor | Metric |
|---|---|---|
| Motion intensity | MPU6050 | RMS acceleration (g) |
| Tremor band | MPU6050 | 3–8 Hz oscillation amplitude via Goertzel |
| Full spectrum | MPU6050 | 256-point FFT, dominant frequency |
| Gait variability | MPU6050 | Coefficient of Variation % (CV%) of RMS over 60 s |
| Freezing of gait | MPU6050 | High/low band-power ratio (Shine index) |
| Falls | MPU6050 | Impact + stillness + >30° tilt change |
| Body temperature | DS18B20 / NTC | °C with low/high alerts |

All features are computed on-device and logged once per 2-second window.

---

## Architecture

```
            ┌───────────────────────────────┐
 MPU6050 ──>│ Core 1: sampling task @100 Hz │
 (I2C)      │   detection engine + FFT      │
            └───────────────┬───────────────┘
                            │ 2 s feature window
 DS18B20/   ┌───────────────▼───────────────┐
 NTC (GPIO)>│ Core 0: loop                   │──> SD card (CSV log)
            │  anomaly engine, web server,   │──> WiFi: dashboard + /api/*
            │  Telegram + OpenAI-style chat  │──> Telegram alerts
            └───────────────────────────────┘
```

- **Web UI** is served directly from the ESP32 (no PC needed) — dashboard, live charts, FFT spectrum, CV% trend, data log, CSV download, settings, and AI chat.
- **AI chat** posts to any **OpenAI-compatible** endpoint (configurable base URL + key + model). The device injects the current readings as system context so the model can comment on temperature, tremor, freezing, variability, and falls.
- **Config is stored in NVS flash**; change WiFi / Telegram / AI credentials from the web Settings page without reflashing. On first boot (or if WiFi fails) the device falls back to **AP mode** (`PD_Monitor` / `monitor123`) so you can always reach it.

---

## Detection algorithms (summary)

- **High-pass filter** to remove gravity: `hp[n]=(g[n]-g[n-1])+0.95·hp[n-1]`
- **Goertzel** single-bin DFT at 3/4/6/8 Hz (tremor) and 1.7 Hz (gait) → band amplitude + **Freeze ratio** = high-band power / low-band power.
- **FFT (Cooley-Tukey, in-place)** → dominant peak frequency for the spectrum chart.
- **Fall detection**: spike > 3 g, then ≥ 1 s near-still, then gravity-vector angle change > 30°.
- **CV%** of RMS over a rolling 60 s window → stride irregularity proxy.
- **Temperature alerts**: low < 35 °C, high > 38 °C (configurable), with a plausibility gate (15–45 °C) to ignore a disconnected probe.

See `firmware/pd_monitor/detection.h` and `fft.h` for the full implementation.

---

## Hardware

| Part | Connection (XIAO ESP32-S3 Sense) |
|---|---|
| MPU6050 SDA / SCL | **D4 (GPIO5)** / **D5 (GPIO6)**, I²C |
| Temp probe signal | **D3 (GPIO4)** — 1-Wire. Add a **4.7 kΩ pull-up to 3V3**. Auto-detects DS18B20 or NTC |
| Temp probe VCC / GND | 3V3 / GND |
| SD card | **Onboard slot** on the Sense expansion board (CS=D2, SCK=D8, MISO=D9, MOSI=D10) |
| Battery | 3.7 V LiPo to BAT pins |

> The firmware will auto-detect the temp probe type. If nothing is found it optionally falls into a **SIM** bench mode (labelled `SIM`) so you can test the dashboard before wiring the probe.

---

## Build & flash

Uses **arduino-cli** with the ESP32 core (≥ 3.3.x). Required libraries: `OneWire`, `ArduinoJson`.

```bash
arduino-cli lib install OneWire ArduinoJson

arduino-cli compile \
  --fqbn esp32:esp32:XIAO_ESP32S3:PSRAM=opi,PartitionScheme=default_8MB,DebugLevel=warn \
  firmware/pd_monitor

arduino-cli upload -p /dev/ttyACM0 \
  --fqbn esp32:esp32:XIAO_ESP32S3:PSRAM=opi,PartitionScheme=default_8MB \
  firmware/pd_monitor
```

### First run
1. Open WiFi and connect to **`PD_Monitor`** (password **`monitor123`**).
2. Browse to **`http://192.168.4.1`**.
3. Open **Settings** and enter your home/phone WiFi SSID + password (optional, for internet + Telegram + AI), Telegram bot token + chat ID, and your AI endpoint details. Save — the device reboots.

If the device is on your local network you can also reach it at `http://pdmonitor.local` (mDNS) or its IP.

---

## Web dashboard

- Live cards: body temp, motion RMS, tremor band, gait CV%, dominant frequency, status flags.
- Charts: temp trend, tremor band, **FFT spectrum (0–12.5 Hz, tremor zone highlighted)**, CV% trend.
- Tabs: **AI Chat**, **Data Log**, **Download CSV**, **Settings**.
- Alert banner with a pulsing red glow on anomaly.
- Glass-morphism design: blurred translucent cards, gradient background, glowing accent buttons.

---

## AI chat

The chat endpoint `POST /api/chat` sends the user's question plus a system prompt containing the **current live readings** to an OpenAI-compatible server:

```
POST /v1/chat/completions
{ "model":"gpt-4o-mini","messages":[{system context},{user question}], ... }
```

Set the **base URL**, **key**, and **model** in the Settings page. Works with OpenAI, any compatible gateway/proxy, or a local server (e.g. Ollama).

---

## Telegram alerts

On anomaly the device posts to a Telegram bot (`sendMessage`) when a **bot token** and **chat ID** are configured. Alerts include low/high temp, tremor spike, freezing of gait, and fall, with a 60-second cooldown per type.

---

## Repository layout

```
firmware/pd_monitor/      Main sketch + modules (detection, FFT, temp, storage, comms, web UI)
firmware/pd_monitor/config.h      Pins, thresholds, config defaults (set your WiFi/AI keys here)
tools/pin_scan/           Helper sketch to find which pin a 1-Wire probe is on
```

---

## Security note

`config.h` ships with **blank** WiFi SSID/password and AI key. Fill them in locally before flashing — **do not commit real credentials** to a public repo. You can also set these at runtime via the web Settings page instead.

---

## License

MIT
