### server.js — Soft Front Panel bridge & SCPI proxy

Serves the front panel from `/public` and forwards UI actions to a target controller over TCP/IP using SCPI. Keeps a configurable target IP/port, translates REST calls into SCPI, and returns parsed state to the UI.

**Endpoints**
- `POST /api/target { host, port }` – set SCPI target (defaults to 127.0.0.1:5025).
- `POST /api/route/:sw/:p` – 2×4 control; sends `SWITCH:SELECT <sw> <p>` then `STATE?`.
- `POST /api/path/:id` – flat mode 1..8; sends `PATH:SELECT <n>` then `STATE?`.
- `GET  /api/state` – readback; sends `STATE?` and returns `{ ok, state }`.

**Transport**
- Raw TCP socket, newline-terminated commands/replies, 1500 ms timeout.

**Run**
```bash
npm install
npm start
# open http://localhost:8080
