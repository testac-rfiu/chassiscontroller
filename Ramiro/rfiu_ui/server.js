// server.js — serves the front panel and forwards actions to the SCPI server
const net = require('net');
const express = require('express');
const cors = require('cors');
const path = require('path');

const app = express();
app.use(cors());
app.use(express.json());
app.use(express.static(path.join(__dirname, 'public')));

// Default target (change from the UI)
let FPGA_HOST = process.env.FPGA_HOST || '127.0.0.1';
let FPGA_PORT = Number(process.env.FPGA_PORT || 5025);

// --- Minimal SCPI sender over TCP ---
function sendScpi(line, { host = FPGA_HOST, port = FPGA_PORT, timeoutMs = 1500 } = {}) {
  return new Promise((resolve, reject) => {
    const sock = new net.Socket();
    let buf = '';
    let done = false;
    const finish = (err, data) => {
      if (done) return;
      done = true;
      try { sock.destroy(); } catch(_) {}
      err ? reject(err) : resolve(data);
    };
    const t = setTimeout(() => finish(new Error('timeout')), timeoutMs);
    sock.connect(port, host, () => { sock.write(line.endsWith('\n') ? line : line + '\n'); });
    sock.on('data', chunk => {
      buf += chunk.toString('utf8');
      if (buf.includes('\n')) { clearTimeout(t); finish(null, buf.trim()); }
    });
    sock.on('error', e => { clearTimeout(t); finish(e); });
    sock.on('close', () => { if (!done) { clearTimeout(t); finish(null, buf.trim()); } });
  });
}

// API: set board target
app.post('/api/target', (req, res) => {
  const { host, port } = req.body || {};
  if (host) FPGA_HOST = host;
  if (port) FPGA_PORT = Number(port);
  res.json({ ok: true, host: FPGA_HOST, port: FPGA_PORT });
});

// API: select a path (1..4) — original
app.post('/api/path/:id', async (req, res) => {
  const id = Number(req.params.id);
  if (!(id >= 1 && id <= 8)) return res.status(400).json({ ok:false, error:'Invalid path' });
  try {
    const setResp = await sendScpi(`PATH:SELECT ${id}`);
    let stateResp = '';
    try { stateResp = await sendScpi('STATE?'); } catch(_) {}
    let state = null; try { state = JSON.parse(stateResp); } catch(_) {}
    res.json({ ok:true, setResp, stateResp, state });
  } catch (e) {
    res.status(502).json({ ok:false, error: e.message });
  }
});

// NEW API: select a route (switch 1–2, path 1–4)
app.post('/api/route/:sw/:p', async (req, res) => {
  const sw = Number(req.params.sw);
  const p  = Number(req.params.p);
  if (![1,2].includes(sw) || !(p >= 1 && p <= 4)) {
    return res.status(400).json({ ok:false, error:'Invalid switch/path' });
  }
  try {
    const setResp = await sendScpi(`SWITCH:SELECT ${sw} ${p}`);
    const stateResp = await sendScpi('STATE?');
    let state = null; try { state = JSON.parse(stateResp); } catch(_) {}
    res.json({ ok:true, setResp, stateResp, state });
  } catch (e) {
    res.status(502).json({ ok:false, error: e.message });
  }
});

// API: read state
app.get('/api/state', async (_req, res) => {
  try {
    const resp = await sendScpi('STATE?');
    let state = null; try { state = JSON.parse(resp); } catch(_) {}
    res.json({ ok:true, raw: resp, state });
  } catch (e) {
    res.status(502).json({ ok:false, error: e.message });
  }
});

const PORT = process.env.PORT || 8080;
app.listen(PORT, () => console.log(`Front panel available at http://localhost:${PORT} (target ${FPGA_HOST}:${FPGA_PORT})`));
