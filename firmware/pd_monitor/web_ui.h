#pragma once
#include <Arduino.h>

static const char INDEX_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>PD Monitor</title>
<style>
:root{--tx:#f4f7fb;--mut:rgba(230,240,255,.55);--glass:rgba(255,255,255,.06);--glass2:rgba(255,255,255,.1);--bd:rgba(255,255,255,.14);--acc:#6ea8ff;--warn:#ffb454;--bad:#ff6b6b;--ok:#4ade80}
*{box-sizing:border-box;margin:0}
html,body{min-height:100%}
body{
 color:var(--tx);font:15px/1.55 -apple-system,BlinkMacSystemFont,"Segoe UI",Inter,sans-serif;
 padding:20px;max-width:1140px;margin:auto;
 background:
  radial-gradient(1200px 600px at 10% -10%,rgba(110,168,255,.25),transparent 60%),
  radial-gradient(1000px 500px at 110% 10%,rgba(167,139,250,.22),transparent 55%),
  radial-gradient(800px 700px at 50% 120%,rgba(56,189,248,.16),transparent 60%),
  linear-gradient(160deg,#0b1020 0%,#0d1428 50%,#0a0f1f 100%);
 background-attachment:fixed;
}
header{display:flex;align-items:center;gap:12px;margin-bottom:4px}
.logo{width:40px;height:40px;border-radius:12px;background:linear-gradient(135deg,var(--acc),#a78bfa);display:flex;align-items:center;justify-content:center;font-size:20px;box-shadow:0 8px 24px rgba(110,168,255,.35)}
h1{font-size:21px;font-weight:700;letter-spacing:.3px}
.sub{color:var(--mut);font-size:12px;margin-bottom:16px}
.glass{background:var(--glass);border:1px solid var(--bd);border-radius:16px;backdrop-filter:blur(18px) saturate(140%);-webkit-backdrop-filter:blur(18px) saturate(140%);box-shadow:0 10px 30px rgba(0,0,0,.35),inset 0 1px 0 rgba(255,255,255,.08)}
#banner{display:none;background:rgba(255,80,80,.16);border:1px solid rgba(255,107,107,.5);border-radius:14px;padding:12px 16px;margin-bottom:14px;font-weight:600;backdrop-filter:blur(14px)}
#banner.pop{animation:pulse 1s infinite alternate}
@keyframes pulse{from{box-shadow:0 0 0 rgba(255,107,107,0)}to{box-shadow:0 0 26px rgba(255,107,107,.5)}}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(160px,1fr));gap:12px;margin-bottom:14px}
.card{padding:16px}
.card .v{font-size:27px;font-weight:700;background:linear-gradient(120deg,#fff,#cfe0ff);-webkit-background-clip:text;background-clip:text;-webkit-text-fill-color:transparent}
.card .l{color:var(--mut);font-size:11px;text-transform:uppercase;letter-spacing:1.2px;margin-bottom:6px}
.flag{display:inline-block;padding:3px 10px;border-radius:99px;font-size:11px;margin:2px;border:1px solid transparent}
.flag.on{background:rgba(255,107,107,.18);color:#ffb3b3;border-color:rgba(255,107,107,.4)}
.flag.off{background:rgba(255,255,255,.05);color:var(--mut)}
canvas.chart{width:100%;height:130px;display:block;border-radius:12px}
.row2{display:grid;grid-template-columns:1fr 1fr;gap:12px}
@media(max-width:760px){.row2{grid-template-columns:1fr}}
.tabs{margin:14px 0;display:flex;flex-wrap:wrap;gap:8px}
.tab{background:var(--glass);color:var(--tx);border:1px solid var(--bd);border-radius:10px;padding:8px 14px;font-size:13px;cursor:pointer;backdrop-filter:blur(10px)}
.tab.sel{background:linear-gradient(135deg,var(--acc),#a78bfa);color:#0b1020;font-weight:700;border-color:transparent}
.chat{display:flex;flex-direction:column;height:360px;overflow:hidden}
#msgs{flex:1;overflow-y:auto;padding:14px;font-size:13px}
.msg{margin-bottom:10px}.msg b{color:var(--acc);font-size:12px}
.msg div{background:var(--glass2);border:1px solid var(--bd);border-radius:12px;padding:8px 12px;margin-top:4px;white-space:pre-wrap;word-break:break-word}
#cin{display:flex;border-top:1px solid var(--bd)}
#cin input{flex:1;background:none;border:none;color:var(--tx);padding:12px;font-size:14px;outline:none}
#cin button{background:linear-gradient(135deg,var(--acc),#a78bfa);color:#0b1020;border:none;padding:9px 16px;font-weight:700;cursor:pointer;margin:8px;border-radius:10px}
input[type=text],input[type=password],input[type=number]{background:rgba(255,255,255,.05);border:1px solid var(--bd);color:var(--tx);padding:10px;border-radius:10px;width:100%;margin-bottom:10px}
input:focus{outline:none;border-color:var(--acc)}
label{font-size:12px;color:var(--mut)}
.save{background:linear-gradient(135deg,var(--acc),#a78bfa);color:#0b1020;border:none;padding:11px 18px;font-weight:700;cursor:pointer;border-radius:10px;margin-bottom:10px}
.hint{color:var(--mut);font-size:11px;margin-top:8px}
</style></head><body>
<header><div class="logo">PD</div><h1>Parkinson's Screening Monitor</h1></header>
<div class="sub" id="status">connecting...</div>
<div id="banner" class="glass"></div>

<div class="grid">
 <div class="card glass"><div class="l">Body Temp</div><div class="v" id="temp">--</div></div>
 <div class="card glass"><div class="l">Motion RMS (g)</div><div class="v" id="rms">--</div></div>
 <div class="card glass"><div class="l">Tremor Band (g)</div><div class="v" id="band">--</div></div>
 <div class="card glass"><div class="l">Gait Variability CV%</div><div class="v" id="cv">--</div></div>
 <div class="card glass"><div class="l">Dominant Freq</div><div class="v" id="dom">--</div></div>
 <div class="card glass"><div class="l">Flags</div><div id="flags"></div></div>
</div>

<div class="row2">
 <div class="card glass"><div class="l">Temp (last 15 min)</div><canvas class="chart" id="ctemp" height="130"></canvas></div>
 <div class="card glass"><div class="l">Tremor Band (last 15 min)</div><canvas class="chart" id="cmot" height="130"></canvas></div>
</div>
<div class="row2" style="margin-top:12px">
 <div class="card glass"><div class="l">Motion Spectrum (0-12.5 Hz)</div><canvas class="chart" id="cspec" height="130"></canvas></div>
 <div class="card glass"><div class="l">Gait Variability CV% (last 15 min)</div><canvas class="chart" id="ccv" height="130"></canvas></div>
</div>

<div class="tabs">
 <button class="tab sel" id="tChat" onclick="show('chat')">AI Chat</button>
 <button class="tab" id="tLog" onclick="show('log')">Data Log</button>
 <button class="tab" id="tCfg" onclick="show('cfg')">Settings</button>
 <button class="tab" onclick="dl()">Download CSV</button>
</div>

<div id="chat" class="glass chat"><div id="msgs"></div>
 <form id="cin" onsubmit="return ask()"><input id="q" placeholder="Ask about the readings, e.g. 'what do these tremor values mean?'" autocomplete="off"><button>Send</button></form>
</div>

<div id="log" class="glass card" style="display:none;font:12px/1.7 monospace;max-height:360px;overflow:auto;white-space:pre">loading...</div>

<div id="cfg" class="glass card" style="display:none">
 <label>WiFi SSID</label><input type="text" id="ssid">
 <label>WiFi Password</label><input type="password" id="pass">
 <label>Telegram Bot Token</label><input type="text" id="tgt">
 <label>Telegram Chat ID</label><input type="text" id="tgc">
 <label>AI API Base URL (OpenAI-compatible)</label><input type="text" id="aib">
 <label>AI API Key</label><input type="password" id="aik">
 <label>AI Model</label><input type="text" id="aim">
 <label>Low Temp Alert (C)</label><input type="number" step="0.1" id="tlo">
 <label>High Temp Alert (C)</label><input type="number" step="0.1" id="thi">
 <label>Tremor Alert Threshold (g)</label><input type="number" step="0.05" id="ttr">
 <button class="save" onclick="saveCfg()">Save Settings</button>
 <div class="hint">Saved to flash. Changing WiFi reboots the device.</div>
</div>

<script>
let S={};
const FLAGS=[['TEMP LOW',0x01],['TEMP HIGH',0x02],['TREMOR',0x04],['FREEZE',0x08],['FALL',0x10],['PROBE',0x20],['SD',0x40],['OFFLINE',0x80]];
const flagsHTML=f=>FLAGS.map(([n,b])=>`<span class="flag ${f&b?'on':'off'}">${n}</span>`).join('');

function chart(id,data,max,color){
 const c=document.getElementById(id),x=c.getContext('2d');
 const W=c.width=c.offsetWidth;
 x.clearRect(0,0,W,130);
 if(!data.length)return;
 const grad=x.createLinearGradient(0,0,0,130);
 grad.addColorStop(0,color+'55');grad.addColorStop(1,color+'00');
 x.beginPath();
 data.forEach((v,i)=>{const px=i/(data.length-1||1)*W,py=125-Math.min(Math.max(v,0)/max,1)*115;i?x.lineTo(px,py):x.moveTo(px,py)});
 x.strokeStyle=color;x.lineWidth=2;x.stroke();
 x.lineTo(W,130);x.lineTo(0,130);x.closePath();x.fillStyle=grad;x.fill();
}

async function poll(){
 try{
  const r=await fetch('/api/state');S=await r.json();
  document.getElementById('status').textContent=
   `fw ${S.fw} | probe ${S.ttype} | sd ${S.sd} | wifi ${S.wifi} | uptime ${Math.floor(S.up/60)}min`;
  document.getElementById('temp').textContent=isFinite(S.temp)?S.temp.toFixed(2)+'\u00b0C':'n/a';
  document.getElementById('rms').textContent=S.rms.toFixed(3);
  document.getElementById('band').textContent=S.band.toFixed(3);
  document.getElementById('cv').textContent=(S.cv??0).toFixed(1)+'%';
  document.getElementById('dom').textContent=isFinite(S.df)&&S.da>0.02?S.df.toFixed(2)+' Hz':'--';
  document.getElementById('flags').innerHTML=flagsHTML(S.flags);
  const b=document.getElementById('banner');
  if(S.alert){b.textContent='ALERT: '+S.alert;b.style.display='block';b.classList.add('pop');}
  else{b.style.display='none';b.classList.remove('pop');}
 }catch(e){document.getElementById('status').textContent='device unreachable'}
 try{
  const h=await (await fetch('/api/history')).json();
  chart('ctemp',h.temp,42,'#ffb454');
  chart('cmot',h.band,Math.max(1,...h.band),'#6ea8ff');
  chart('ccv',h.cv||[],Math.max(50,...(h.cv||[])),'#4ade80');
  bars(h.spec||[]);
 }catch(e){}
}

function bars(spec){
 const c=document.getElementById('cspec'),x=c.getContext('2d');
 const W=c.width=c.offsetWidth;
 x.clearRect(0,0,W,130);
 if(!spec.length)return;
 const bw=W/spec.length;
 spec.forEach((v,i)=>{
  const h=Math.min(v/255,1)*118;
  const f=(i*bw/2)>=3&&(i*bw/2)<=8;
  const g=x.createLinearGradient(0,128-h,0,128);
  g.addColorStop(0,f?'#ffb454':'#6ea8ff');g.addColorStop(1,f?'#ffb45400':'#6ea8ff00');
  x.fillStyle=g;
  x.fillRect(i*bw+1,128-h,bw-2,h);
 });
 x.fillStyle='rgba(230,240,255,.55)';x.font='10px monospace';
 x.fillText('3Hz',3/12.5*W,10);x.fillText('8Hz',8/12.5*W,10);
}

function show(id){
 for(const t of ['chat','log','cfg'])document.getElementById(t).style.display=t===id?(t==='chat'?'flex':'block'):'none';
 for(const t of ['Chat','Log','Cfg'])document.getElementById('t'+t).classList.toggle('sel',t.toLowerCase()===id);
 if(id==='log')loadLog();
}

async function loadLog(){
 try{document.getElementById('log').textContent=await (await fetch('/api/log')).text();}
 catch(e){document.getElementById('log').textContent='failed to load';}
}

async function ask(){
 const q=document.getElementById('q').value.trim();if(!q)return false;
 document.getElementById('q').value='';
 const m=document.getElementById('msgs');
 const esc=s=>s.replace(/&/g,'&amp;').replace(/</g,'&lt;');
 m.insertAdjacentHTML('beforeend',`<div class="msg"><b>You</b><div>${esc(q)}</div></div>`);
 m.insertAdjacentHTML('beforeend',`<div class="msg" id="pw"><b>AI</b><div>thinking...</div></div>`);
 m.scrollTop=m.scrollHeight;
 try{
  const j=await (await fetch('/api/chat',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({q})})).json();
  const pw=document.getElementById('pw');pw.lastChild.textContent=j.a||('error: '+(j.e||'no reply'));
 }catch(e){const pw=document.getElementById('pw');if(pw)pw.lastChild.textContent='request failed';}
 m.scrollTop=m.scrollHeight;return false;
}

async function saveCfg(){
 const body={ssid:v('ssid'),pass:v('pass'),tg_token:v('tgt'),tg_chat:v('tgc'),
  ai_base:v('aib'),ai_key:v('aik'),ai_model:v('aim'),
  tlo:parseFloat(v('tlo')),thi:parseFloat(v('thi')),ttr:parseFloat(v('ttr'))};
 const j=await (await fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)})).json();
 alert(j.ok?'Saved. Device rebooting...':'Save failed');
 if(j.ok)setTimeout(()=>location.reload(),3000);
}
const v=id=>document.getElementById(id).value;
const dl=()=>window.open('/api/logs');

async function loadCfg(){
 try{
  const j=await (await fetch('/api/config')).json();
  for(const [k,id] of [['ssid','ssid'],['tg_chat','tgc'],['ai_base','aib'],['ai_model','aim'],
     ['temp_low','tlo'],['temp_high','thi'],['tremor_thr','ttr'],['tg_token','tgt']])
   document.getElementById(id).value=j[k]??'';
  document.getElementById('aik').placeholder=j.ai_key_set?'(already set, enter new to change)':'';
 }catch(e){}
}

poll();loadCfg();setInterval(poll,2500);
</script></body></html>)rawliteral";
