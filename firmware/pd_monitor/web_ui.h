#pragma once
#include <Arduino.h>

static const char INDEX_HTML[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="color-scheme" content="dark">
<title>PD Monitor</title>
<style>
:root{
  --text:#eaf1ff;
  --muted:#93a4c3;
  --line:rgba(148,163,184,.16);
  --accent:#7db2ff;
  --accent2:#a78bfa;
  --good:#34d399;
  --warn:#fbbf24;
  --bad:#fb7185;
  --radius:18px;
  --shadow:0 16px 44px rgba(2,6,23,.42), inset 0 1px 0 rgba(255,255,255,.06);
}
*{box-sizing:border-box}
html{color-scheme:dark}
body{
  margin:0;
  min-height:100vh;
  color:var(--text);
  font:16px/1.5 system-ui,-apple-system,BlinkMacSystemFont,"Segoe UI",Inter,sans-serif;
  background:
    radial-gradient(1000px 420px at 15% -10%, rgba(125,178,255,.16), transparent 55%),
    radial-gradient(800px 380px at 100% 0%, rgba(167,139,250,.12), transparent 50%),
    linear-gradient(165deg,#070b14 0%,#0b1222 48%,#070d1a 100%);
  background-color:#070b14;
  background-repeat:no-repeat;
  background-size:cover;
  -webkit-font-smoothing:antialiased;
}
.shell{
  width:min(1180px, calc(100% - 24px));
  margin:16px auto 44px;
  display:grid;
  gap:14px;
}
.skip{
  position:absolute;
  left:-9999px;
  top:auto;
  z-index:99;
  background:#0b1222;
  color:var(--text);
  padding:10px 12px;
  border-radius:10px;
  border:1px solid var(--line);
  text-decoration:none;
}
.skip:focus{left:12px;top:12px}
.glass{
  background:linear-gradient(180deg, rgba(255,255,255,.075), rgba(255,255,255,.035));
  border:1px solid var(--line);
  border-radius:var(--radius);
  box-shadow:var(--shadow);
  backdrop-filter:blur(14px) saturate(130%);
  -webkit-backdrop-filter:blur(14px) saturate(130%);
}
.top{
  display:flex;
  align-items:center;
  justify-content:space-between;
  gap:14px;
  padding:14px 16px;
}
.brand{
  display:flex;
  align-items:center;
  gap:12px;
  min-width:0;
}
.logo{
  width:46px;
  height:46px;
  flex:0 0 auto;
  border-radius:15px;
  display:grid;
  place-items:center;
  font-weight:900;
  color:#07101f;
  background:linear-gradient(135deg,var(--accent),var(--accent2));
  box-shadow:0 10px 24px rgba(125,178,255,.25);
}
h1{
  margin:0;
  font-size:1.08rem;
  line-height:1.2;
  letter-spacing:.01em;
  font-weight:850;
}
.sub{
  margin:3px 0 0;
  color:var(--muted);
  font-size:.8rem;
  white-space:nowrap;
  overflow:hidden;
  text-overflow:ellipsis;
}
.top-actions{
  display:flex;
  align-items:center;
  gap:8px;
  flex:0 0 auto;
}
.pill{
  padding:7px 10px;
  border-radius:999px;
  font-size:.75rem;
  font-weight:800;
  border:1px solid rgba(148,163,184,.2);
  background:rgba(255,255,255,.04);
  color:var(--muted);
}
.pill.ok{
  color:#c7f5e5;
  background:rgba(52,211,153,.12);
  border-color:rgba(52,211,153,.38);
}
.pill.bad{
  color:#ffd9df;
  background:rgba(251,113,133,.12);
  border-color:rgba(251,113,133,.4);
}
.pill.warn{
  color:#ffe9bd;
  background:rgba(251,191,36,.12);
  border-color:rgba(251,191,36,.38);
}
.banner{
  display:flex;
  gap:10px;
  align-items:baseline;
  padding:13px 15px;
  border-radius:16px;
  border:1px solid rgba(251,113,133,.42);
  background:rgba(251,113,133,.13);
  color:#ffe4e6;
  font-weight:700;
  box-shadow:0 10px 30px rgba(251,113,133,.08);
}
.banner strong{
  letter-spacing:.06em;
  font-size:.8rem;
}
main{display:grid;gap:14px}
.metrics{
  display:grid;
  grid-template-columns:repeat(6,minmax(0,1fr));
  gap:12px;
}
.card{padding:15px;min-width:0}
.metric h2{
  margin:0;
  font-size:.72rem;
  text-transform:uppercase;
  letter-spacing:.09em;
  color:var(--muted);
  font-weight:850;
}
.value{
  margin:8px 0 4px;
  font-size:1.75rem;
  line-height:1.15;
  font-weight:900;
  letter-spacing:-.02em;
  font-variant-numeric:tabular-nums;
}
.value small{
  margin-left:6px;
  color:var(--muted);
  font-size:.82rem;
  font-weight:750;
}
.meta{
  margin:0;
  color:var(--muted);
  font-size:.78rem;
  min-height:1.1em;
}
.flags{
  display:flex;
  flex-wrap:wrap;
  gap:6px;
  margin-top:9px;
}
.flag{
  padding:5px 8px;
  border-radius:999px;
  font-size:.68rem;
  font-weight:850;
  letter-spacing:.02em;
  border:1px solid rgba(148,163,184,.18);
  color:var(--muted);
  background:rgba(255,255,255,.03);
}
.flag.on{
  color:#ffd7dd;
  border-color:rgba(251,113,133,.42);
  background:rgba(251,113,133,.14);
}
.charts{
  display:grid;
  grid-template-columns:1fr 1fr;
  gap:12px;
}
.chart-card h2{
  margin:0;
  font-size:.82rem;
  font-weight:850;
}
.chart-card canvas{
  width:100%;
  height:155px;
  display:block;
  margin-top:10px;
}
.panel{overflow:hidden}
.tabs{
  display:flex;
  flex-wrap:wrap;
  gap:8px;
  padding:12px;
  border-bottom:1px solid var(--line);
}
.tab{
  border:1px solid rgba(148,163,184,.18);
  background:rgba(255,255,255,.04);
  color:var(--text);
  padding:9px 13px;
  border-radius:12px;
  cursor:pointer;
  font:inherit;
  font-size:.88rem;
  font-weight:750;
}
.tab:hover{background:rgba(255,255,255,.08)}
.tab.sel{
  background:linear-gradient(135deg,var(--accent),var(--accent2));
  color:#07101f;
  border-color:transparent;
  box-shadow:0 8px 22px rgba(125,178,255,.22);
}
.panel-body{padding:14px}
.chat{
  display:flex;
  flex-direction:column;
  min-height:400px;
}
#msgs{
  flex:1;
  overflow:auto;
  display:grid;
  gap:10px;
  padding:2px 2px 2px 0;
  min-height:180px;
}
.msg{max-width:92%}
.msg b{
  display:block;
  margin-bottom:5px;
  color:var(--muted);
  font-size:.7rem;
  text-transform:uppercase;
  letter-spacing:.08em;
}
.msg div{
  padding:10px 12px;
  border-radius:14px;
  border:1px solid rgba(148,163,184,.18);
  background:rgba(255,255,255,.045);
  white-space:pre-wrap;
  word-break:break-word;
}
.msg.user div{
  background:rgba(125,178,255,.13);
  border-color:rgba(125,178,255,.32);
}
.quick{
  display:flex;
  flex-wrap:wrap;
  gap:8px;
  margin:0 0 10px;
}
.chip{
  border:1px solid rgba(148,163,184,.18);
  background:rgba(255,255,255,.04);
  color:var(--text);
  padding:7px 10px;
  border-radius:999px;
  cursor:pointer;
  font:inherit;
  font-size:.78rem;
  font-weight:650;
}
.chip:hover{background:rgba(255,255,255,.09)}
#chatForm{
  display:grid;
  grid-template-columns:1fr auto;
  gap:10px;
  margin-top:8px;
}
input[type="text"],
input[type="password"],
input[type="number"]{
  width:100%;
  min-width:0;
  background:rgba(255,255,255,.045);
  border:1px solid rgba(148,163,184,.18);
  color:var(--text);
  border-radius:12px;
  padding:11px 12px;
  font:inherit;
  outline:none;
}
input:focus-visible{
  border-color:var(--accent);
  box-shadow:0 0 0 3px rgba(125,178,255,.14);
}
.btn{
  border:0;
  border-radius:12px;
  padding:10px 14px;
  font:inherit;
  font-weight:850;
  cursor:pointer;
  transition:transform .12s ease, opacity .12s ease;
}
.btn:hover{transform:translateY(-1px)}
.btn:active{transform:translateY(0)}
.btn:disabled{
  opacity:.55;
  cursor:not-allowed;
  transform:none;
}
.btn.primary{
  background:linear-gradient(135deg,var(--accent),var(--accent2));
  color:#07101f;
  box-shadow:0 10px 24px rgba(125,178,255,.22);
}
.btn.ghost{
  background:rgba(255,255,255,.05);
  color:var(--text);
  border:1px solid rgba(148,163,184,.18);
}
.log pre{
  margin:0;
  max-height:400px;
  overflow:auto;
  font:12px/1.7 ui-monospace,SFMono-Regular,Menlo,Consolas,monospace;
  color:#dbe7ff;
  white-space:pre;
}
.cfg-grid{
  display:grid;
  grid-template-columns:repeat(3,minmax(0,1fr));
  gap:12px;
}
fieldset{
  border:1px solid rgba(148,163,184,.18);
  border-radius:16px;
  padding:12px;
  margin:0;
  min-width:0;
  display:grid;
  gap:8px;
  align-content:start;
}
legend{
  padding:0 6px;
  color:var(--muted);
  font-size:.8rem;
  font-weight:850;
}
label{
  font-size:.8rem;
  color:var(--muted);
  font-weight:650;
}
.cfg-actions{
  display:flex;
  align-items:center;
  gap:12px;
  margin-top:13px;
  flex-wrap:wrap;
}
.hint{
  margin:0;
  color:var(--muted);
  font-size:.8rem;
}
[hidden]{display:none !important}
:focus-visible{
  outline:2px solid var(--accent);
  outline-offset:2px;
}
::selection{
  background:rgba(125,178,255,.28);
}
@media (max-width:1000px){
  .metrics{grid-template-columns:repeat(3,minmax(0,1fr))}
  .cfg-grid{grid-template-columns:1fr}
}
@media (max-width:800px){
  .charts{grid-template-columns:1fr}
}
@media (max-width:700px){
  .top{
    align-items:flex-start;
    flex-direction:column;
  }
  .top-actions{
    width:100%;
    justify-content:flex-start;
  }
}
@media (max-width:620px){
  .metrics{grid-template-columns:repeat(2,minmax(0,1fr))}
  #chatForm{grid-template-columns:1fr}
  .btn.primary{width:100%}
}
@media (max-width:420px){
  .metrics{grid-template-columns:1fr}
}
@media (prefers-reduced-motion:reduce){
  *{
    animation:none !important;
    transition:none !important;
    scroll-behavior:auto !important;
  }
}
</style>
</head>
<body>
<a class="skip" href="#main">Skip to dashboard</a>

<div class="shell" id="main">
  <header class="top glass" role="banner">
    <div class="brand">
      <div class="logo" aria-hidden="true">PD</div>
      <div style="min-width:0">
        <h1>Parkinson's Screening Monitor</h1>
        <p class="sub" id="status" aria-live="polite">Connecting...</p>
      </div>
    </div>

    <div class="top-actions">
      <span id="conn" class="pill warn" role="status">connecting</span>
      <button id="download" class="btn ghost" type="button" aria-label="Download CSV">CSV</button>
    </div>
  </header>

  <section id="banner" class="banner" role="alert" hidden>
    <strong>ALERT</strong>
    <span id="bannerText"></span>
  </section>

  <main>
    <section class="metrics" aria-label="Current readings">
      <article class="card glass metric">
        <h2>Body Temp</h2>
        <p class="value"><span id="temp">--</span><small>&deg;C</small></p>
        <p class="meta" id="tempMeta">--</p>
      </article>

      <article class="card glass metric">
        <h2>Motion RMS</h2>
        <p class="value"><span id="rms">--</span><small>g</small></p>
        <p class="meta">overall acceleration energy</p>
      </article>

      <article class="card glass metric">
        <h2>Tremor Band</h2>
        <p class="value"><span id="band">--</span><small>g</small></p>
        <p class="meta">3-8 Hz tremor component</p>
      </article>

      <article class="card glass metric">
        <h2>Gait CV</h2>
        <p class="value"><span id="cv">--</span><small>%</small></p>
        <p class="meta">stride variability estimate</p>
      </article>

      <article class="card glass metric">
        <h2>Dominant Freq</h2>
        <p class="value"><span id="dom">--</span><small>Hz</small></p>
        <p class="meta" id="domMeta">--</p>
      </article>

      <article class="card glass metric">
        <h2>Flags</h2>
        <div class="flags" id="flags" aria-live="polite"></div>
        <p class="meta" id="flagMeta">--</p>
      </article>
    </section>

    <section class="charts" aria-label="Trends">
      <article class="card glass chart-card">
        <h2>Temperature / last 15 min</h2>
        <canvas id="ctemp" role="img" aria-label="Temperature history"></canvas>
      </article>

      <article class="card glass chart-card">
        <h2>Tremor Band / last 15 min</h2>
        <canvas id="cmot" role="img" aria-label="Tremor band history"></canvas>
      </article>

      <article class="card glass chart-card">
        <h2>Motion Spectrum / 0-12.5 Hz</h2>
        <canvas id="cspec" role="img" aria-label="Motion spectrum"></canvas>
      </article>

      <article class="card glass chart-card">
        <h2>Gait CV / last 15 min</h2>
        <canvas id="ccv" role="img" aria-label="Gait variability history"></canvas>
      </article>
    </section>

    <section class="glass panel" aria-label="Tools">
      <div class="tabs" role="tablist" aria-label="Dashboard panels">
        <button class="tab sel" id="tab-chat" data-tab="chat" role="tab" aria-selected="true" aria-controls="panel-chat" type="button">AI Chat</button>
        <button class="tab" id="tab-log" data-tab="log" role="tab" aria-selected="false" aria-controls="panel-log" type="button">Data Log</button>
        <button class="tab" id="tab-cfg" data-tab="cfg" role="tab" aria-selected="false" aria-controls="panel-cfg" type="button">Settings</button>
      </div>

      <div id="panel-chat" class="panel-body chat" role="tabpanel" aria-labelledby="tab-chat">
        <div id="msgs" aria-live="polite">
          <div class="msg">
            <b>AI</b>
            <div>Connected. Ask me to explain temperature, tremor band, gait CV, spectrum, or active alerts.</div>
          </div>
        </div>

        <div class="quick" id="quick">
          <button type="button" class="chip" data-q="Explain the current tremor band value.">Tremor band</button>
          <button type="button" class="chip" data-q="What does the dominant frequency suggest?">Dominant frequency</button>
          <button type="button" class="chip" data-q="Is the gait variability concerning?">Gait CV</button>
          <button type="button" class="chip" data-q="What do the active flags mean?">Active flags</button>
        </div>

        <form id="chatForm">
          <input id="q" type="text" placeholder="Ask about the readings..." autocomplete="off" aria-label="Ask the AI about the readings">
          <button id="sendBtn" class="btn primary" type="submit">Send</button>
        </form>
      </div>

      <div id="panel-log" class="panel-body log" role="tabpanel" aria-labelledby="tab-log" hidden>
        <pre id="log" tabindex="0">Loading...</pre>
      </div>

      <div id="panel-cfg" class="panel-body cfg" role="tabpanel" aria-labelledby="tab-cfg" hidden>
        <form id="cfgForm">
          <div class="cfg-grid">
            <fieldset>
              <legend>Network</legend>

              <label for="ssid">WiFi SSID</label>
              <input type="text" id="ssid" autocomplete="off">

              <label for="pass">WiFi Password</label>
              <input type="password" id="pass" autocomplete="new-password" placeholder="Leave blank to keep current">
            </fieldset>

            <fieldset>
              <legend>Alert Thresholds</legend>

              <label for="temp_low">Low Temp Alert (&deg;C)</label>
              <input type="number" id="temp_low" step="0.1" inputmode="decimal">

              <label for="temp_high">High Temp Alert (&deg;C)</label>
              <input type="number" id="temp_high" step="0.1" inputmode="decimal">

              <label for="tremor_thr">Tremor Alert Threshold (g)</label>
              <input type="number" id="tremor_thr" step="0.01" inputmode="decimal">
            </fieldset>

            <fieldset>
              <legend>AI / Notifications</legend>

              <label for="tgt">Telegram Bot Token</label>
              <input type="password" id="tgt" autocomplete="off" placeholder="Optional">

              <label for="tg_chat">Telegram Chat ID</label>
              <input type="text" id="tg_chat" autocomplete="off">

              <label for="ai_base">AI API Base URL</label>
              <input type="text" id="ai_base" placeholder="https://api.openai.com/v1">

              <label for="ai_key">AI API Key</label>
              <input type="password" id="ai_key" autocomplete="off" placeholder="Optional">

              <label for="ai_model">AI Model</label>
              <input type="text" id="ai_model" placeholder="gpt-4o-mini">
            </fieldset>
          </div>

          <div class="cfg-actions">
            <button id="saveCfg" class="btn primary" type="submit">Save Settings</button>
            <p id="cfgHint" class="hint" role="status">Secret fields are only sent if filled.</p>
          </div>
        </form>
      </div>
    </section>
  </main>
</div>

<script>
'use strict';
(() => {
  const $ = (s, r = document) => r.querySelector(s);
  const $$ = (s, r = document) => Array.from(r.querySelectorAll(s));

  const endpoints = {
    state: '/api/state',
    history: '/api/history',
    log: '/api/log',
    chat: '/api/chat',
    config: '/api/config',
    logs: '/api/logs'
  };

  const els = {
    status: $('#status'),
    conn: $('#conn'),
    banner: $('#banner'),
    bannerText: $('#bannerText'),

    temp: $('#temp'),
    tempMeta: $('#tempMeta'),
    rms: $('#rms'),
    band: $('#band'),
    cv: $('#cv'),
    dom: $('#dom'),
    domMeta: $('#domMeta'),
    flags: $('#flags'),
    flagMeta: $('#flagMeta'),

    ctemp: $('#ctemp'),
    cmot: $('#cmot'),
    cspec: $('#cspec'),
    ccv: $('#ccv'),

    tabs: $('.tabs'),
    log: $('#log'),

    msgs: $('#msgs'),
    chatForm: $('#chatForm'),
    chatInput: $('#q'),
    sendBtn: $('#sendBtn'),
    quick: $('#quick'),

    download: $('#download'),

    cfgForm: $('#cfgForm'),
    cfgHint: $('#cfgHint'),
    saveCfg: $('#saveCfg')
  };

  const FLAGS = [
    ['TEMP LOW', 0x01],
    ['TEMP HIGH', 0x02],
    ['TREMOR', 0x04],
    ['FREEZE', 0x08],
    ['FALL', 0x10],
    ['PROBE', 0x20],
    ['OFFLINE', 0x80]
  ];

  const panels = {
    chat: $('#panel-chat'),
    log: $('#panel-log'),
    cfg: $('#panel-cfg')
  };

  const tabButtons = $$('.tab[data-tab]');

  const cfgFields = {
    ssid: $('#ssid'),
    pass: $('#pass'),
    tgt: $('#tgt'),
    tgc: $('#tg_chat'),
    aib: $('#ai_base'),
    aik: $('#ai_key'),
    aim: $('#ai_model'),
    tlo: $('#temp_low'),
    thi: $('#temp_high'),
    ttr: $('#tremor_thr')
  };

  let history = null;
  let cfg = {};
  let busy = false;
  let resizeTimer = 0;

  function hexToRgba(hex, alpha) {
    const c = hex.replace('#', '');
    const full = c.length === 3
      ? c.split('').map(ch => ch + ch).join('')
      : c;
    const n = parseInt(full, 16);
    return `rgba(${(n >> 16) & 255}, ${(n >> 8) & 255}, ${n & 255}, ${alpha})`;
  }

  function fitCanvas(canvas) {
    const dpr = window.devicePixelRatio || 1;
    const rect = canvas.getBoundingClientRect();
    const w = Math.max(10, rect.width);
    const h = Math.max(10, rect.height);

    canvas.width = Math.round(w * dpr);
    canvas.height = Math.round(h * dpr);

    const ctx = canvas.getContext('2d');
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);

    return { ctx, w, h };
  }

  function drawEmpty(ctx, w, h, message = 'No data yet') {
    ctx.clearRect(0, 0, w, h);
    ctx.fillStyle = 'rgba(148,163,184,.72)';
    ctx.font = '12px system-ui,sans-serif';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText(message, w / 2, h / 2);
  }

  function niceTick(v) {
    if (!Number.isFinite(v)) return '';
    const abs = Math.abs(v);
    if (abs >= 100) return v.toFixed(0);
    if (abs >= 10) return v.toFixed(1);
    if (abs >= 1) return v.toFixed(2);
    return v.toFixed(3);
  }

  function lineChart(canvas, raw, opts = {}) {
    if (!canvas) return;

    const { ctx, w, h } = fitCanvas(canvas);
    ctx.clearRect(0, 0, w, h);

    const data = Array.isArray(raw) ? raw : [];
    const values = data.map(Number).filter(Number.isFinite);

    if (!values.length) {
      drawEmpty(ctx, w, h);
      return;
    }

    const padL = 38;
    const padR = 10;
    const padT = 10;
    const padB = 20;

    let min = Math.min(...values);
    let max = Math.max(...values);

    if (opts.softMin != null) min = Math.min(min, opts.softMin);
    if (opts.softMax != null) max = Math.max(max, opts.softMax);
    if (opts.min != null) min = opts.min;
    if (opts.max != null) max = opts.max;

    if (max === min) {
      max += 1;
      min -= 1;
    }

    const span = max - min;
    const plotW = w - padL - padR;
    const plotH = h - padT - padB;

    const x = i => data.length === 1
      ? padL + plotW / 2
      : padL + plotW * (i / (data.length - 1));

    const y = v => padT + plotH * (1 - (v - min) / span);

    ctx.save();
    ctx.font = '10px ui-monospace,SFMono-Regular,Menlo,Consolas,monospace';
    ctx.textBaseline = 'middle';

    for (let i = 0; i <= 3; i++) {
      const gy = padT + plotH * i / 3;
      const val = max - span * i / 3;

      ctx.strokeStyle = 'rgba(148,163,184,.13)';
      ctx.beginPath();
      ctx.moveTo(padL, gy);
      ctx.lineTo(w - padR, gy);
      ctx.stroke();

      ctx.fillStyle = 'rgba(148,163,184,.72)';
      ctx.textAlign = 'right';
      ctx.fillText(niceTick(val), padL - 6, gy);
    }

    ctx.textAlign = 'left';
    ctx.fillText('-15m', padL, h - 8);

    ctx.textAlign = 'right';
    ctx.fillText('now', w - padR, h - 8);
    ctx.restore();

    const segments = [];
    let seg = [];

    data.forEach((v, i) => {
      const n = Number(v);
      if (Number.isFinite(n)) {
        seg.push({ i, v: n });
      } else if (seg.length) {
        segments.push(seg);
        seg = [];
      }
    });

    if (seg.length) segments.push(seg);

    const color = opts.color || '#7db2ff';
    const grad = ctx.createLinearGradient(0, padT, 0, h - padB);
    grad.addColorStop(0, hexToRgba(color, .24));
    grad.addColorStop(1, hexToRgba(color, 0));

    segments.forEach(segment => {
      if (segment.length < 2) return;

      ctx.beginPath();
      ctx.moveTo(x(segment[0].i), h - padB);

      segment.forEach(p => ctx.lineTo(x(p.i), y(p.v)));

      ctx.lineTo(x(segment[segment.length - 1].i), h - padB);
      ctx.closePath();

      ctx.fillStyle = grad;
      ctx.fill();
    });

    ctx.beginPath();

    segments.forEach(segment => {
      segment.forEach((p, j) => {
        const px = x(p.i);
        const py = y(p.v);
        if (j === 0) ctx.moveTo(px, py);
        else ctx.lineTo(px, py);
      });
    });

    ctx.strokeStyle = color;
    ctx.lineWidth = 2;
    ctx.lineJoin = 'round';
    ctx.lineCap = 'round';
    ctx.stroke();

    const lastSeg = segments[segments.length - 1];
    if (lastSeg && lastSeg.length) {
      const p = lastSeg[lastSeg.length - 1];

      ctx.beginPath();
      ctx.arc(x(p.i), y(p.v), 3, 0, Math.PI * 2);
      ctx.fillStyle = color;
      ctx.fill();

      ctx.strokeStyle = 'rgba(7,11,20,.8)';
      ctx.lineWidth = 1;
      ctx.stroke();
    }
  }

  function spectrumChart(canvas, raw) {
    if (!canvas) return;

    const { ctx, w, h } = fitCanvas(canvas);
    ctx.clearRect(0, 0, w, h);

    const spec = Array.isArray(raw)
      ? raw.map(Number).filter(Number.isFinite)
      : [];

    if (!spec.length) {
      drawEmpty(ctx, w, h);
      return;
    }

    const padT = 8;
    const padB = 18;
    const max = 255;
    const bw = w / spec.length;
    const plotH = h - padT - padB;

    ctx.save();

    ctx.strokeStyle = 'rgba(148,163,184,.12)';
    ctx.beginPath();
    ctx.moveTo(0, h - padB);
    ctx.lineTo(w, h - padB);
    ctx.stroke();

    spec.forEach((v, i) => {
      const val = Math.max(0, Math.min(max, v));
      const barH = Math.max(2, val / max * plotH);
      const f = spec.length > 1 ? i * (12.5 / (spec.length - 1)) : 0;
      const inBand = f >= 3 && f <= 8;
      const color = inBand ? '#fbbf24' : '#7db2ff';

      const x = i * bw + 1;
      const y = h - padB - barH;

      const grad = ctx.createLinearGradient(0, y, 0, h - padB);
      grad.addColorStop(0, hexToRgba(color, .9));
      grad.addColorStop(1, hexToRgba(color, .08));

      ctx.fillStyle = grad;
      ctx.fillRect(x, y, Math.max(1, bw - 2), barH);
    });

    ctx.fillStyle = 'rgba(148,163,184,.78)';
    ctx.font = '10px ui-monospace,SFMono-Regular,Menlo,Consolas,monospace';

    ctx.textAlign = 'left';
    ctx.fillText('3 Hz', (3 / 12.5) * w, h - 5);

    ctx.textAlign = 'right';
    ctx.fillText('8 Hz', (8 / 12.5) * w, h - 5);

    ctx.textAlign = 'right';
    ctx.fillText('12.5 Hz', w - 2, h - 5);

    ctx.restore();
  }

  function formatUptime(sec) {
    if (!Number.isFinite(sec)) return '--';

    const m = Math.floor(sec / 60);
    const h = Math.floor(m / 60);
    const d = Math.floor(h / 24);

    if (d) return `${d}d ${h % 24}h`;
    if (h) return `${h}h ${m % 60}m`;
    return `${m}m`;
  }

  function setConn(ok, label) {
    els.conn.className = 'pill ' + (ok ? 'ok' : 'bad');
    els.conn.textContent = label;
  }

  function renderState(s) {
    const flags = Number(s.flags) || 0;

    const temp = Number(s.temp);
    const rms = Number(s.rms);
    const band = Number(s.band);
    const cv = Number(s.cv);
    const df = Number(s.df);
    const da = Number(s.da);

    els.temp.textContent = Number.isFinite(temp) ? temp.toFixed(2) : '--';
    els.tempMeta.textContent = (flags & 0x20)
      ? 'probe fault'
      : `probe ${s.ttype == null ? '?' : s.ttype}`;

    els.rms.textContent = Number.isFinite(rms) ? rms.toFixed(3) : '--';
    els.band.textContent = Number.isFinite(band) ? band.toFixed(3) : '--';
    els.cv.textContent = Number.isFinite(cv) ? cv.toFixed(1) : '--';

    const hasPeak = Number.isFinite(df) && Number.isFinite(da) && da > 0.02;
    els.dom.textContent = hasPeak ? df.toFixed(2) : '--';
    els.domMeta.textContent = hasPeak
      ? `amplitude ${da.toFixed(3)} g`
      : 'no clear peak';

    els.flags.textContent = '';
    const active = [];

    FLAGS.forEach(([label, bit]) => {
      const span = document.createElement('span');
      const on = (flags & bit) !== 0;
      span.className = 'flag ' + (on ? 'on' : 'off');
      span.textContent = label;
      els.flags.appendChild(span);

      if (on) active.push(label);
    });

    els.flagMeta.textContent = active.length
      ? active.join(' · ')
      : 'no active flags';

    if (s.alert) {
      els.banner.hidden = false;
      els.bannerText.textContent = String(s.alert);
    } else {
      els.banner.hidden = true;
      els.bannerText.textContent = '';
    }

    els.status.textContent =
      `fw ${s.fw == null ? '?' : s.fw}` +
      ` · probe ${s.ttype == null ? '?' : s.ttype}` +
      ` · log ${s.sd == null ? '?' : s.sd} rows` +
      ` · WiFi ${s.wifi == null ? '?' : s.wifi}` +
      ` · up ${formatUptime(Number(s.up))}`;
  }

  function renderHistory(h) {
    if (!h) return;

    history = h;

    lineChart(els.ctemp, h.temp, {
      color: '#fbbf24',
      softMin: 30,
      softMax: 42
    });

    lineChart(els.cmot, h.band, {
      color: '#7db2ff',
      softMin: 0,
      softMax: 0.25
    });

    lineChart(els.ccv, h.cv || [], {
      color: '#34d399',
      softMin: 0,
      softMax: 20
    });

    spectrumChart(els.cspec, h.spec || []);
  }

  async function fetchJson(url, opts = {}) {
    const res = await fetch(url, {
      cache: 'no-store',
      ...opts
    });

    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    return res.json();
  }

  async function fetchText(url) {
    const res = await fetch(url, {
      cache: 'no-store'
    });

    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    return res.text();
  }

  async function poll() {
    if (busy) return;
    busy = true;

    try {
      const s = await fetchJson(endpoints.state);
      renderState(s);
      setConn(true, 'online');
    } catch {
      setConn(false, 'offline');
      els.status.textContent = 'device unreachable';
    }

    try {
      const h = await fetchJson(endpoints.history);
      renderHistory(h);
    } catch {}

    busy = false;
  }

  async function loadLog() {
    try {
      els.log.textContent = await fetchText(endpoints.log);
    } catch {
      els.log.textContent = 'Failed to load log.';
    }
  }

  function addMsg(role, text, isUser = false) {
    const wrap = document.createElement('div');
    wrap.className = 'msg' + (isUser ? ' user' : '');

    const name = document.createElement('b');
    name.textContent = role;

    const body = document.createElement('div');
    body.textContent = text;

    wrap.append(name, body);
    els.msgs.appendChild(wrap);
    els.msgs.scrollTop = els.msgs.scrollHeight;

    return body;
  }

  async function sendMessage(text) {
    const q = text.trim();
    if (!q) return;

    addMsg('You', q, true);
    els.chatInput.value = '';
    els.sendBtn.disabled = true;

    const out = addMsg('AI', 'Thinking...');

    try {
      const j = await fetchJson(endpoints.chat, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ q })
      });

      out.textContent = j.a || j.error || j.e || 'No reply.';
    } catch {
      out.textContent = 'Request failed. Check device connectivity.';
    } finally {
      els.sendBtn.disabled = false;
      els.chatInput.focus();
    }
  }

  function showTab(name) {
    if (!panels[name]) name = 'chat';

    tabButtons.forEach(btn => {
      const selected = btn.dataset.tab === name;
      btn.classList.toggle('sel', selected);
      btn.setAttribute('aria-selected', String(selected));
    });

    Object.entries(panels).forEach(([key, panel]) => {
      panel.hidden = key !== name;
    });

    if (name === 'log') loadLog();
  }

  function setField(id, value) {
    if (cfgFields[id] && value != null) {
      cfgFields[id].value = value;
    }
  }

  function fieldVal(id) {
    return cfgFields[id].value.trim();
  }

  function numVal(id, cfgKey, fallback) {
    const raw = fieldVal(id);

    if (raw === '') {
      const existing = Number(cfg[cfgKey]);
      return Number.isFinite(existing) ? existing : fallback;
    }

    const n = Number(raw);
    return Number.isFinite(n) ? n : fallback;
  }

  async function loadCfg() {
    try {
      cfg = await fetchJson(endpoints.config) || {};

      setField('ssid', cfg.ssid);
      setField('tgc', cfg.tg_chat);
      setField('aib', cfg.ai_base);
      setField('aim', cfg.ai_model);
      setField('tlo', cfg.temp_low);
      setField('thi', cfg.temp_high);
      setField('ttr', cfg.tremor_thr);

      if (cfg.tg_token && !String(cfg.tg_token).includes('*')) {
        setField('tgt', cfg.tg_token);
      }

      cfgFields.pass.placeholder = 'Leave blank to keep current';

      cfgFields.tgt.placeholder = cfg.tg_token_set
        ? 'Already set. Enter new token to change.'
        : 'Optional';

      cfgFields.aik.placeholder = cfg.ai_key_set
        ? 'Already set. Enter new key to change.'
        : 'Optional';
    } catch {
      els.cfgHint.textContent = 'Could not load current settings.';
    }
  }

  function init() {
    els.download.addEventListener('click', () => {
      window.open(endpoints.logs, '_blank');
    });

    tabButtons.forEach(btn => {
      btn.addEventListener('click', () => showTab(btn.dataset.tab));
    });

    if (els.tabs) {
      els.tabs.addEventListener('keydown', e => {
        if (!['ArrowRight', 'ArrowLeft'].includes(e.key)) return;

        const idx = tabButtons.findIndex(b => b.classList.contains('sel'));
        const next = (idx + (e.key === 'ArrowRight' ? 1 : -1) + tabButtons.length) % tabButtons.length;

        tabButtons[next].focus();
        showTab(tabButtons[next].dataset.tab);
        e.preventDefault();
      });
    }

    els.chatForm.addEventListener('submit', e => {
      e.preventDefault();
      sendMessage(els.chatInput.value);
    });

    els.quick.addEventListener('click', e => {
      const chip = e.target.closest('.chip');
      if (!chip) return;
      sendMessage(chip.dataset.q || chip.textContent);
    });

    els.cfgForm.addEventListener('submit', async e => {
      e.preventDefault();

      els.saveCfg.disabled = true;
      els.cfgHint.textContent = 'Saving...';

      const payload = {
        ssid: fieldVal('ssid'),
        tg_chat: fieldVal('tgc'),
        ai_base: fieldVal('aib'),
        ai_model: fieldVal('aim'),
        tlo: numVal('tlo', 'temp_low', 35),
        thi: numVal('thi', 'temp_high', 38.5),
        ttr: numVal('ttr', 'tremor_thr', 0.12)
      };

      const pass = fieldVal('pass');
      if (pass) payload.pass = pass;

      const tgToken = fieldVal('tgt');
      if (tgToken) payload.tg_token = tgToken;

      const aiKey = fieldVal('aik');
      if (aiKey) payload.ai_key = aiKey;

      try {
        const j = await fetchJson(endpoints.config, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(payload)
        });

        if (j && j.ok) {
          els.cfgHint.textContent = 'Saved. Device rebooting...';
          setTimeout(() => location.reload(), 3500);
        } else {
          els.cfgHint.textContent = 'Save failed: ' + ((j && (j.e || j.error)) || 'unknown error');
        }
      } catch {
        els.cfgHint.textContent = 'Save failed: device unreachable.';
      } finally {
        els.saveCfg.disabled = false;
      }
    });

    window.addEventListener('resize', () => {
      clearTimeout(resizeTimer);
      resizeTimer = setTimeout(() => renderHistory(history), 150);
    });

    showTab('chat');
    poll();
    loadCfg();

    setInterval(() => {
      if (!document.hidden) poll();
    }, 2500);
  }

  init();
})();
</script>
</body>
</html>)rawliteral";