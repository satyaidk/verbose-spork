/* =============================================================================
   web_ui.h  -  the entire web controller, as one page in flash
   -----------------------------------------------------------------------------
   No CDN, no external stylesheet, no framework. That is a requirement rather
   than a preference: when the Mochi cannot find your Wi-Fi it serves this page
   from its own hotspot, where there is no internet, and anything loaded from a
   CDN would silently fail exactly when you most need the page to work.

   It is stored with PROGMEM and served with server.send_P(), so the ~14 KB
   never occupies RAM.

   The page talks to the REST API in web_api.h. Everything is form-encoded on
   the way in and JSON on the way out, which avoids pulling a JSON parsing
   library onto the device.
   ============================================================================= */
#pragma once

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"HTMLPAGE(<!doctype html>
<html lang="en"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Mochi Controller</title>
<style>
:root{--bg:#14161a;--panel:#1c1f26;--line:#2b303a;--ink:#e9e5da;
--dim:#8b8778;--glow:#ffb000;--ok:#5fd08a;--bad:#e8705a}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--ink);
 font:15px/1.5 -apple-system,"Segoe UI",Roboto,Helvetica,Arial,sans-serif}
header{position:sticky;top:0;z-index:5;background:rgba(20,22,26,.96);
 border-bottom:1px solid var(--line);padding:14px 16px}
.wrap{max-width:44rem;margin:0 auto}
h1{margin:0;font-size:1.15rem;font-weight:600;letter-spacing:.01em}
h1 span{color:var(--glow)}
.meta{color:var(--dim);font-size:.76rem;margin-top:3px;font-family:ui-monospace,monospace}
.dot{display:inline-block;width:7px;height:7px;border-radius:50%;
 background:var(--bad);margin-right:6px;vertical-align:middle}
.dot.on{background:var(--ok)}
.acts{display:flex;gap:8px;margin-top:12px;flex-wrap:wrap}
button{font:inherit;font-size:.86rem;border:1px solid var(--line);border-radius:8px;
 background:var(--panel);color:var(--ink);padding:8px 14px;cursor:pointer}
button:hover{border-color:#3d4453}
button.pri{background:var(--glow);border-color:var(--glow);color:#14161a;font-weight:600}
button.warn{border-color:#5a3230;color:var(--bad)}
button:focus-visible{outline:2px solid var(--glow);outline-offset:2px}
nav{display:flex;gap:2px;border-bottom:1px solid var(--line);margin:16px 0 0;
 overflow-x:auto}
nav b{padding:10px 14px;font-weight:500;font-size:.9rem;color:var(--dim);
 cursor:pointer;white-space:nowrap;border-bottom:2px solid transparent}
nav b.sel{color:var(--ink);border-bottom-color:var(--glow)}
main{max-width:44rem;margin:0 auto;padding:18px 16px 70px}
section{display:none}
section.sel{display:block}
.card{background:var(--panel);border:1px solid var(--line);border-radius:12px;
 padding:16px;margin-bottom:14px}
.card h2{margin:0 0 4px;font-size:.95rem;font-weight:600}
.card p.hint{margin:0 0 12px;color:var(--dim);font-size:.8rem}
label{display:block;color:var(--dim);font-size:.78rem;margin:12px 0 5px}
label:first-of-type{margin-top:0}
input,select,textarea{width:100%;background:#12141896;border:1px solid var(--line);
 border-radius:8px;color:var(--ink);padding:9px 10px;font:inherit;font-size:.88rem}
textarea{font-family:ui-monospace,Menlo,Consolas,monospace;font-size:.8rem;
 min-height:62px;resize:vertical}
input:focus,select:focus,textarea:focus{outline:2px solid var(--glow);
 outline-offset:1px;border-color:transparent}
.row{display:flex;gap:10px;flex-wrap:wrap}
.row>div{flex:1 1 130px}
.sw{display:flex;align-items:center;gap:9px;margin:14px 0 0;color:var(--ink);
 font-size:.88rem;cursor:pointer}
.sw input{width:auto;accent-color:var(--glow)}
.chips{display:flex;flex-wrap:wrap;gap:6px;margin-top:8px}
.chips button{padding:5px 10px;font-size:.76rem;border-radius:999px;color:var(--dim)}
.chips button:hover{color:var(--ink)}
.gifs{display:grid;grid-template-columns:repeat(auto-fill,minmax(148px,1fr));gap:8px;
 margin-top:10px}
.gif{display:flex;align-items:center;gap:8px;background:#1214188c;
 border:1px solid var(--line);border-radius:9px;padding:8px 10px}
.gif input{width:auto;accent-color:var(--glow);flex:none}
.gif span{flex:1;font-size:.82rem;overflow:hidden;text-overflow:ellipsis;
 white-space:nowrap}
.gif small{color:var(--dim);font-size:.68rem;font-family:ui-monospace,monospace}
.gif b{cursor:pointer;color:var(--dim);font-weight:400;padding:0 3px}
.gif b:hover{color:var(--glow)}
.mel{display:flex;gap:8px;align-items:flex-start}
.mel textarea{flex:1}
.mel button{flex:none;padding:9px 12px}
table{width:100%;border-collapse:collapse;font-size:.83rem}
td{padding:6px 0;border-bottom:1px solid var(--line);color:var(--dim)}
td:last-child{text-align:right;color:var(--ink);font-family:ui-monospace,monospace}
tr:last-child td{border-bottom:0}
.seg{display:inline-flex;border:1px solid var(--line);border-radius:9px;overflow:hidden;
 margin-top:12px}
.seg button{border:0;border-radius:0;background:var(--panel);padding:8px 18px;
 font-size:.85rem;color:var(--dim)}
.seg button.on{background:var(--glow);color:#14161a;font-weight:600}
.live{display:grid;grid-template-columns:repeat(auto-fit,minmax(96px,1fr));gap:8px;
 margin-top:10px}
.live div{background:#1214188c;border:1px solid var(--line);border-radius:9px;
 padding:9px 10px}
.live b{display:block;font-size:1rem;font-weight:600}
.live span{color:var(--dim);font-size:.72rem}
#toast{position:fixed;left:50%;transform:translateX(-50%);bottom:22px;
 background:#232830;border:1px solid var(--line);border-radius:9px;
 padding:10px 18px;font-size:.85rem;opacity:0;transition:opacity .2s;
 pointer-events:none;z-index:9}
#toast.on{opacity:1}
</style></head><body>

<header><div class="wrap">
  <h1><span>&#9673;</span> MOCHI CONTROLLER</h1>
  <div class="meta"><i class="dot" id="dot"></i><span id="stat">connecting</span></div>
  <div class="seg">
    <button id="m0" onclick="setMode(0)">Mochi</button>
    <button id="m1" onclick="setMode(1)">Clock</button>
  </div>
  <div class="acts">
    <button class="pri" onclick="save()">Save settings</button>
    <button onclick="api('/api/notify')">Test sound</button>
    <button class="warn" onclick="reboot()">Reset Mochi</button>
  </div>
  <nav>
    <b class="sel" data-t="gif">Adjust GIF</b>
    <b data-t="snd">Adjust the sound</b>
    <b data-t="clk">Clock</b>
    <b data-t="hw">Hardware</b>
    <b data-t="sys">System</b>
  </nav>
</div></header>

<main>
<section id="gif" class="sel">
  <div class="card">
    <h2>Speed</h2>
    <p class="hint">Frame time in milliseconds. 70 is the rate the clips were recorded at; higher is slower.</p>
    <div class="row">
      <div><label>GIF speed (ms/frame)</label><input type="number" id="speed" min="20" max="500"></div>
      <div><label>Delay between GIFs (s)</label><input type="number" id="delay" min="0" max="120"></div>
    </div>
    <label class="sw"><input type="checkbox" id="neg"> Negative (white background)</label>
  </div>

  <div class="card">
    <h2>Which clip plays when</h2>
    <p class="hint">The resting face loops continuously. Reactions play once, then return to it.</p>
    <div class="row">
      <div><label>Default (resting face)</label><select id="def"></select></div>
      <div><label>Intro (at boot)</label><select id="intro"></select></div>
    </div>
    <div class="row">
      <div><label>Tap once</label><select id="tap"></select></div>
      <div><label>Tap twice</label><select id="dbl"></select></div>
    </div>
    <div class="row">
      <div><label>Tap three times</label><select id="trip"></select></div>
      <div><label>Hold 5 s</label><select id="long"></select></div>
    </div>
    <p class="hint">Holding 15 s always switches apps instead, in either mode.</p>
  </div>

  <div class="card">
    <h2>GIF list</h2>
    <p class="hint">Ticked clips join the random rotation. Untick everything and only the resting face plays. The arrow shows a clip on the device now.</p>
    <div class="acts">
      <button onclick="allGifs(1)">Select all</button>
      <button onclick="allGifs(0)">Deselect all</button>
    </div>
    <div class="gifs" id="gifs"></div>
  </div>
</section>

<section id="snd">
  <div class="card">
    <h2>Melody format</h2>
    <p class="hint">Triplets of note, duration, rest: <code>C4 100 20 D4S 100 20</code>.
      Notes are A to G with optional S for sharp and an octave digit 0-8. R is a rest.
      Tap a sample name to load it, then the arrow to hear it on the device.</p>
    <label class="sw"><input type="checkbox" id="sndon"> Sound enabled</label>
  </div>
  <div class="card"><h2>Opening music</h2><div class="mel">
    <textarea id="mIntro"></textarea><button onclick="prev('mIntro')">&#9654;</button></div>
    <div class="chips" data-for="mIntro"></div></div>
  <div class="card"><h2>Sound on single tap</h2><div class="mel">
    <textarea id="mTap"></textarea><button onclick="prev('mTap')">&#9654;</button></div>
    <div class="chips" data-for="mTap"></div></div>
  <div class="card"><h2>Sound on double tap</h2><div class="mel">
    <textarea id="mDbl"></textarea><button onclick="prev('mDbl')">&#9654;</button></div>
    <div class="chips" data-for="mDbl"></div></div>
  <div class="card"><h2>Sound on triple tap</h2><div class="mel">
    <textarea id="mTrip"></textarea><button onclick="prev('mTrip')">&#9654;</button></div>
    <div class="chips" data-for="mTrip"></div></div>
  <div class="card"><h2>Sound when held 5 s</h2><div class="mel">
    <textarea id="mLong"></textarea><button onclick="prev('mLong')">&#9654;</button></div>
    <div class="chips" data-for="mLong"></div></div>
  <div class="card"><h2>Notification</h2>
    <p class="hint">Not triggered by anything on the device yet. It is the hook for the
      Bluetooth notification feature, and fires from the Test sound button.</p>
    <div class="mel"><textarea id="mNotify"></textarea>
      <button onclick="prev('mNotify')">&#9654;</button></div>
    <div class="chips" data-for="mNotify"></div></div>
</section>

<section id="clk">
  <div class="card">
    <h2>Live now</h2>
    <p class="hint">Fetched by the device. Weather refreshes every 15 minutes, prices every minute.</p>
    <div class="live" id="liveBox"></div>
    <div class="acts"><button onclick="api('/api/refresh').then(function(){toast('fetching');setTimeout(load,3000)})">Refresh now</button></div>
  </div>
  <div class="card">
    <h2>Time and units</h2>
    <p class="hint">POSIX timezone string. The sign is inverted: India is
      <code>IST-5:30</code>, London <code>GMT0BST,M3.5.0/1,M10.5.0</code>,
      New York <code>EST5EDT,M3.2.0,M11.1.0</code>.</p>
    <label>Timezone</label><input id="tz" maxlength="48">
    <label class="sw"><input type="checkbox" id="metric"> Metric (Celsius and km/h)</label>
  </div>
  <div class="card">
    <h2>Notes screen</h2>
    <p class="hint">About 21 characters per line, 6 lines per page. Longer text pages
      automatically every 5 seconds.</p>
    <div class="screen"><pre id="notePrev"></pre></div>
    <label>Title</label><input id="ntitle" maxlength="18">
    <label>Text</label><textarea id="ntext" maxlength="1400" style="min-height:150px"></textarea>
  </div>
</section>

<section id="hw">
  <div class="card">
    <h2>Pins and bus</h2>
    <p class="hint">Changing anything here reboots the Mochi, because the display and
      touch pad are only bound to their pins at startup. Use 22 to mean "not fitted".</p>
    <div class="row">
      <div><label>SDA</label><input type="number" id="sda" min="0" max="22"></div>
      <div><label>SCL</label><input type="number" id="scl" min="0" max="22"></div>
    </div>
    <div class="row">
      <div><label>Touch pin</label><input type="number" id="touch" min="0" max="22"></div>
      <div><label>Buzzer pin</label><input type="number" id="buzz" min="0" max="22"></div>
    </div>
    <label>I2C speed</label>
    <select id="i2c">
      <option value="400000">400 kHz (safe)</option>
      <option value="700000">700 kHz (default)</option>
      <option value="1000000">1 MHz (fastest)</option>
    </select>
    <label class="sw"><input type="checkbox" id="flip"> Flip the screen 180&deg;</label>
    <div class="acts"><button class="pri" onclick="saveHw()">Save and reboot</button></div>
  </div>
</section>

<section id="sys">
  <div class="card">
    <h2>Status</h2>
    <table id="statT"></table>
  </div>
  <div class="card">
    <h2>Wi-Fi</h2>
    <p class="hint">2.4 GHz only. Saving reboots and joins the new network. If it cannot
      connect it comes back as its own hotspot.</p>
    <label>Network name</label><input id="ssid">
    <label>Password</label><input id="pass" type="password" placeholder="unchanged">
    <div class="acts"><button class="pri" onclick="saveWifi()">Save and reboot</button></div>
  </div>
  <div class="card">
    <h2>Factory reset</h2>
    <p class="hint">Clears every stored setting including Wi-Fi, then reboots.
      The animations themselves are compiled in and are not affected.</p>
    <div class="acts"><button class="warn" onclick="factory()">Restore defaults</button></div>
  </div>
</section>
</main>
<div id="toast"></div>

<script>
var st = null;

function $(i){ return document.getElementById(i); }

function toast(m){
  var t = $('toast'); t.textContent = m; t.className = 'on';
  clearTimeout(t._h); t._h = setTimeout(function(){ t.className = ''; }, 1800);
}

function api(p, d){
  var o = { method: 'POST' };
  if (d) { o.body = new URLSearchParams(d); }
  return fetch(p, o).then(function(r){ return r.text(); });
}

/* ---- pull state and paint everything ---- */
function load(){
  fetch('/api/state').then(function(r){ return r.json(); }).then(function(j){
    st = j;
    $('dot').className = 'dot on';
    $('stat').textContent = (j.ap ? 'hotspot ' : '') + j.ip + '  |  ' + j.mac +
                            '  |  fw ' + j.fw;
    fillSelects(); fillGifs(); fillChips(); paint(); paintStats(); paintLive();
  }).catch(function(){
    $('dot').className = 'dot';
    $('stat').textContent = 'no connection to the device';
  });
}

function fillSelects(){
  ['def','intro','tap','dbl','trip','long'].forEach(function(k){
    var s = $(k), h = (k === 'def') ? '' : '<option value="255">None</option>';
    st.anims.forEach(function(a, i){ h += '<option value="' + i + '">' + a.n + '</option>'; });
    s.innerHTML = h;
  });
}

function fillGifs(){
  var h = '';
  st.anims.forEach(function(a, i){
    h += '<div class="gif"><input type="checkbox" data-i="' + i + '" onchange="pushGif()">' +
         '<span>' + a.n + '<br><small>' + a.f + 'f &middot; ' +
         (a.b / 1024).toFixed(1) + 'K</small></span>' +
         '<b onclick="api(\'/api/play?i=' + i + '\')">&#9654;</b></div>';
  });
  $('gifs').innerHTML = h;
}

function fillChips(){
  document.querySelectorAll('.chips').forEach(function(c){
    var f = c.dataset.for, h = '';
    st.presets.forEach(function(p){
      h += '<button onclick="setMel(\'' + f + '\',\'' + p.s + '\')">' + p.n + '</button>';
    });
    c.innerHTML = h;
  });
}

function paint(){
  var g = st.gif, s = st.snd, w = st.hw;
  $('speed').value = g.speed; $('delay').value = g.delay;
  $('neg').checked = g.neg;
  $('def').value = g.def; $('intro').value = g.intro;
  $('tap').value = g.tap; $('dbl').value = g.dbl;
  $('trip').value = g.trip; $('long').value = g.lng;
  document.querySelectorAll('#gifs input').forEach(function(c){
    c.checked = !!(g.mask & (1 << (+c.dataset.i)));
  });
  $('sndon').checked = s.on;
  ['mIntro','mTap','mDbl','mTrip','mLong','mNotify'].forEach(function(k){ $(k).value = s[k]; });
  $('tz').value = st.clk.tz; $('metric').checked = st.clk.metric;
  $('ntitle').value = st.clk.title; $('ntext').value = st.clk.text;
  notePreview();
  $('m0').className = st.mode === 0 ? 'on' : '';
  $('m1').className = st.mode === 1 ? 'on' : '';
  $('sda').value = w.sda; $('scl').value = w.scl;
  $('touch').value = w.touch; $('buzz').value = w.buzz;
  $('i2c').value = w.i2c; $('flip').checked = w.flip;
  $('ssid').value = st.ssid;
}

function paintStats(){
  var free = 1310 - (st.stats.bytes / 1024) - 300;
  $('statT').innerHTML =
    row('Firmware', st.fw) +
    row('MAC', st.mac) +
    row('IP address', st.ip + (st.ap ? ' (hotspot)' : '')) +
    row('Wi-Fi', st.ap ? 'not joined' : st.ssid + '  ' + st.rssi + ' dBm') +
    row('Running', st.mode ? 'Clock app' : 'Mochi app') +
    row('Display', st.display ? 'OK at ' + st.addr : 'not detected') +
    row('Touch pin', st.hw.touch === 22 ? 'not fitted' : 'GPIO' + st.hw.touch) +
    row('Animations', st.anims.length + ' clips, ' + st.stats.frames + ' frames') +
    row('Frame data', (st.stats.bytes / 1024).toFixed(0) + ' KB of flash') +
    row('Free heap', (st.heap / 1024).toFixed(0) + ' KB') +
    row('Uptime', Math.floor(st.up / 60) + 'm ' + (st.up % 60) + 's');
}

function row(a, b){ return '<tr><td>' + a + '</td><td>' + b + '</td></tr>'; }

function paintLive(){
  var L = st.live, u = st.clk.metric ? 'C' : 'F';
  var scr = ['Clock','Weather','Crypto','Notes'][st.screen] || '-';
  $('liveBox').innerHTML =
    cell(L.wx ? L.temp + u : '--', L.city || 'location') +
    cell(L.wx ? L.cond : '--', 'condition') +
    cell(L.btc ? '$' + L.btc : '--', 'BTC') +
    cell(L.eth ? '$' + L.eth : '--', 'ETH') +
    cell(L.sol ? '$' + L.sol : '--', 'SOL') +
    cell(scr, 'screen showing');
}
function cell(v, l){ return '<div><b>' + v + '</b><span>' + l + '</span></div>'; }

function notePreview(){
  var w = 21, out = [], n = 0;
  $('ntext').value.split('\n').forEach(function(p){
    if (!p.length) out.push('');
    while (p.length > w) {
      var c = p.lastIndexOf(' ', w); if (c <= 0) c = w;
      out.push(p.slice(0, c)); p = p.slice(c).trim();
    }
    if (p.length) out.push(p);
  });
  $('notePrev').textContent = out.slice(0, 6).join('\n') + (out.length > 6 ? '\n...' : '');
}

function pushClock(){
  notePreview();
  api('/api/clock', { tz: $('tz').value, metric: $('metric').checked ? 1 : 0,
                      title: $('ntitle').value, text: $('ntext').value })
    .then(function(){ toast('applied'); });
}

function setMode(m){
  api('/api/mode', { m: m }).then(function(){
    st.mode = m;
    $('m0').className = m === 0 ? 'on' : '';
    $('m1').className = m === 1 ? 'on' : '';
    toast(m ? 'clock app' : 'mochi app');
  });
}

/* ---- push changes live ---- */
function mask(){
  var m = 0;
  document.querySelectorAll('#gifs input').forEach(function(c){
    if (c.checked) m |= (1 << (+c.dataset.i));
  });
  return m;
}

function pushGif(){
  api('/api/gif', {
    speed: $('speed').value, delay: $('delay').value,
    def: $('def').value, intro: $('intro').value, tap: $('tap').value,
    dbl: $('dbl').value, trip: $('trip').value, lng: $('long').value,
    mask: mask(), neg: $('neg').checked ? 1 : 0
  }).then(function(){ toast('applied'); });
}

function pushSnd(){
  api('/api/snd', {
    on: $('sndon').checked ? 1 : 0,
    mIntro: $('mIntro').value, mTap: $('mTap').value, mDbl: $('mDbl').value,
    mTrip: $('mTrip').value, mLong: $('mLong').value, mNotify: $('mNotify').value
  }).then(function(){ toast('applied'); });
}

function setMel(f, s){ $(f).value = s; pushSnd(); prev(f); }
function prev(f){ api('/api/preview', { m: $(f).value }); }
function allGifs(v){
  document.querySelectorAll('#gifs input').forEach(function(c){ c.checked = !!v; });
  pushGif();
}

function save(){ api('/api/save').then(function(){ toast('saved to flash'); }); }

function saveHw(){
  api('/api/hw', {
    sda: $('sda').value, scl: $('scl').value, touch: $('touch').value,
    buzz: $('buzz').value, i2c: $('i2c').value, flip: $('flip').checked ? 1 : 0
  }).then(function(){ toast('rebooting'); setTimeout(load, 6000); });
}

function saveWifi(){
  api('/api/wifi', { ssid: $('ssid').value, pass: $('pass').value })
    .then(function(){ toast('rebooting, reconnect to the new network'); });
}

function reboot(){
  if (confirm('Reset the Mochi? Unsaved changes are lost.')) {
    api('/api/reboot').then(function(){ toast('rebooting'); setTimeout(load, 6000); });
  }
}

function factory(){
  if (confirm('Erase every stored setting and reboot?')) {
    api('/api/defaults').then(function(){ toast('restoring defaults'); });
  }
}

/* ---- wiring ---- */
document.querySelectorAll('nav b').forEach(function(b){
  b.onclick = function(){
    document.querySelectorAll('nav b').forEach(function(x){ x.className = ''; });
    document.querySelectorAll('section').forEach(function(x){ x.className = ''; });
    b.className = 'sel';
    $(b.dataset.t).className = 'sel';
  };
});

['speed','delay','def','intro','tap','dbl','trip','long'].forEach(function(k){
  $(k).addEventListener('change', pushGif);
});
['tz','metric','ntitle','ntext'].forEach(function(k){
  $(k).addEventListener('change', pushClock);
});
$('ntext').addEventListener('input', notePreview);
$('neg').addEventListener('change', pushGif);
['sndon','mIntro','mTap','mDbl','mTrip','mLong','mNotify'].forEach(function(k){
  $(k).addEventListener('change', pushSnd);
});

load();
setInterval(function(){
  if (!st) return;
  fetch('/api/state').then(function(r){ return r.json(); }).then(function(j){
    st = j; paintStats(); paintLive();
    $('m0').className = j.mode === 0 ? 'on' : '';
    $('m1').className = j.mode === 1 ? 'on' : '';
  }).catch(function(){});
}, 10000);
</script></body></html>
)HTMLPAGE";
