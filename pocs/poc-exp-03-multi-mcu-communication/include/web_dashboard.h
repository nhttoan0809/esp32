#ifndef WEB_DASHBOARD_H
#define WEB_DASHBOARD_H

#include <Arduino.h>

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>EXP-03: Multi-MCU Distributed Network</title>
  <style>
    :root {
      --bg: #0f172a;
      --card-bg: #1e293b;
      --card-border: #334155;
      --text: #f8fafc;
      --text-muted: #94a3b8;
      --primary: #38bdf8;
      --success: #22c55e;
      --warning: #f59e0b;
      --danger: #ef4444;
      --accent: #818cf8;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
    body { background: var(--bg); color: var(--text); padding: 16px; min-height: 100vh; }
    header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 20px; padding-bottom: 12px; border-bottom: 1px solid var(--card-border); flex-wrap: wrap; gap: 10px; }
    h1 { font-size: 1.25rem; font-weight: 700; color: var(--primary); display: flex; align-items: center; gap: 8px; }
    .badge { padding: 4px 10px; border-radius: 9999px; font-size: 0.75rem; font-weight: 600; text-transform: uppercase; }
    .badge-ok { background: rgba(34, 197, 94, 0.2); color: var(--success); border: 1px solid var(--success); }
    .badge-warn { background: rgba(245, 158, 11, 0.2); color: var(--warning); border: 1px solid var(--warning); }
    .badge-err { background: rgba(239, 68, 68, 0.2); color: var(--danger); border: 1px solid var(--danger); }
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(320px, 1fr)); gap: 16px; }
    .card { background: var(--card-bg); border: 1px solid var(--card-border); border-radius: 12px; padding: 18px; box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.3); }
    .card-title { font-size: 1rem; font-weight: 600; margin-bottom: 14px; display: flex; justify-content: space-between; align-items: center; }
    .card-title span.mcu { font-size: 0.75rem; padding: 2px 8px; border-radius: 6px; background: rgba(56, 189, 248, 0.15); color: var(--primary); }
    .row { display: flex; justify-content: space-between; padding: 6px 0; border-bottom: 1px solid rgba(255,255,255,0.05); font-size: 0.85rem; }
    .row span:first-child { color: var(--text-muted); }
    .row span:last-child { font-weight: 600; font-family: ui-monospace, monospace; }
    .btn-group { display: flex; gap: 8px; margin-top: 14px; flex-wrap: wrap; }
    button { flex: 1; padding: 8px 12px; border: none; border-radius: 6px; font-weight: 600; cursor: pointer; transition: all 0.15s; font-size: 0.85rem; min-width: 100px; }
    button:active { transform: scale(0.97); }
    .btn-primary { background: var(--primary); color: #000; }
    .btn-danger { background: var(--danger); color: #fff; }
    .btn-success { background: var(--success); color: #000; }
    .btn-accent { background: var(--accent); color: #fff; }
    .btn-secondary { background: #475569; color: #fff; }
    .progress-bar-bg { width: 100%; height: 8px; background: #334155; border-radius: 4px; overflow: hidden; margin-top: 6px; }
    .progress-bar { height: 100%; background: var(--primary); width: 0%; transition: width 0.2s; }
    .val-highlight { font-size: 1.4rem; font-weight: 700; color: var(--primary); font-family: ui-monospace, monospace; margin: 4px 0; }
    .status-dot { display: inline-block; width: 8px; height: 8px; border-radius: 50%; margin-right: 6px; }
    .dot-green { background: var(--success); box-shadow: 0 0 6px var(--success); }
    .dot-red { background: var(--danger); box-shadow: 0 0 6px var(--danger); }
    .dot-orange { background: var(--warning); box-shadow: 0 0 6px var(--warning); }
    footer { margin-top: 24px; text-align: center; font-size: 0.75rem; color: var(--text-muted); }
  </style>
</head>
<body>
  <header>
    <h1>
      <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="2" y="2" width="20" height="8" rx="2"/><rect x="2" y="14" width="20" height="8" rx="2"/><line x1="6" y1="6" x2="6.01" y2="6"/><line x1="6" y1="18" x2="6.01" y2="18"/></svg>
      EXP-03: Multi-MCU Distributed Network
    </h1>
    <div style="display:flex; align-items:center; gap:8px;">
      <span id="ws-badge" class="badge badge-warn">Đang kết nối...</span>
      <span id="ntp-time" style="font-size:0.8rem; color:var(--text-muted); font-family:monospace;">--:--:--</span>
    </div>
  </header>

  <main class="grid">
    <!-- NODE 1: ESP32 MASTER GATEWAY -->
    <div class="card">
      <div class="card-title">
        <span>Node 1: Master Gateway</span>
        <span class="mcu">ESP32 240MHz</span>
      </div>
      <div class="row"><span>Trạng Thái Hệ Thống:</span><span id="master-status" style="color:var(--success);">RUNNING</span></div>
      <div class="row"><span>Uptime:</span><span id="master-uptime">0s</span></div>
      <div class="row"><span>Free Heap:</span><span id="master-heap">-- KB</span></div>
      <div class="row"><span>IP Trạm Wi-Fi:</span><span id="master-ip">--</span></div>
      <div class="row"><span>I2C Bus Clock:</span><span>100 kHz (Std)</span></div>
      <div class="row"><span>UART2 Bus Speed:</span><span>115200 bps</span></div>
      <div class="row"><span>Tỷ lệ lỗi gói UART:</span><span id="uart-err-count">0</span></div>
      <div style="margin-top:14px; font-size:0.8rem; color:var(--text-muted);">
        <span>Trạng thái LED Master: </span>
        <span id="led-i2c-status" class="status-dot dot-green"></span>I2C
        <span id="led-uart-status" class="status-dot dot-orange" style="margin-left:8px;"></span>UART
        <span id="led-sys-status" class="status-dot dot-green" style="margin-left:8px;"></span>SYS
      </div>
    </div>

    <!-- NODE 2: STM32F4 COPROCESSOR -->
    <div class="card">
      <div class="card-title">
        <span>Node 2: Coprocessor (I2C 0x42)</span>
        <span class="mcu">STM32F4 Black Pill</span>
      </div>
      <div class="row"><span>Trạng Thái Nút:</span><span id="stm32-online"><span class="status-dot dot-red"></span>OFFLINE</span></div>
      <div class="row"><span>Động Cơ Bước 28BYJ-48:</span><span id="stm32-step-state">IDLE</span></div>
      <div class="row"><span>Vị Trí Hiện Tại:</span><span id="stm32-step-pos" style="color:var(--primary);">0 bước</span></div>
      
      <div style="margin-top:10px; font-size:0.8rem; color:var(--text-muted);">Điều khiển góc quay Động cơ bước:</div>
      <div class="btn-group">
        <button class="btn-primary" onclick="sendCmd('stepper', {dir:0, rpm:15, steps:512})">Quay Thuận (+45°)</button>
        <button class="btn-primary" onclick="sendCmd('stepper', {dir:1, rpm:15, steps:512})">Quay Nghịch (-45°)</button>
        <button class="btn-danger" onclick="sendCmd('stepper_stop', {})">Dừng Khẩn</button>
      </div>

      <div style="margin-top:16px; border-top:1px solid rgba(255,255,255,0.05); padding-top:10px;">
        <div style="font-size:0.8rem; color:var(--text-muted); display:flex; justify-content:space-between;">
          <span>ARM Cortex-M4 FPU Benchmark:</span>
          <span id="stm32-dsp-time" style="color:var(--accent); font-weight:bold;">-- us</span>
        </div>
        <div class="btn-group">
          <button class="btn-accent" onclick="sendCmd('dsp_bench', {algo:1, iters:20})">Chạy FPU DSP Benchmark</button>
        </div>
      </div>
    </div>

    <!-- NODE 3: ARDUINO NANO SUB-CONTROLLER -->
    <div class="card">
      <div class="card-title">
        <span>Node 3: Sub-Controller (UART2)</span>
        <span class="mcu">Arduino Nano (5V)</span>
      </div>
      <div class="row"><span>Trạng Thái Nút:</span><span id="nano-online"><span class="status-dot dot-red"></span>OFFLINE</span></div>
      <div class="row"><span>Điện Áp Cấp VCC:</span><span id="nano-vcc">-- V</span></div>
      
      <div style="margin-top:8px;">
        <div class="row"><span>Cảm biến dòng ACS712 (A0):</span><span id="nano-acs712">-- A (Raw: --)</span></div>
        <div class="progress-bar-bg"><div id="acs712-bar" class="progress-bar"></div></div>
      </div>

      <div style="margin-top:8px;">
        <div class="row"><span>Biến Trở Xoay 10k (A1):</span><span id="nano-pot">-- V (Raw: --)</span></div>
        <div class="progress-bar-bg"><div id="pot-bar" class="progress-bar"></div></div>
      </div>

      <div class="row" style="margin-top:8px;"><span>Nút Bấm Số (D2 / D3):</span><span id="nano-btn">BTN1: UP | BTN2: UP</span></div>

      <div style="margin-top:14px; font-size:0.8rem; color:var(--text-muted);">Điều khiển Ngoại vi 5V (Relay & Buzzer):</div>
      <div class="btn-group">
        <button id="btn-relay" class="btn-secondary" onclick="toggleRelay()">Relay: Đang Tắt</button>
        <button class="btn-warning" onclick="sendCmd('buzzer', {freq:2400, duration:80})">Còi Bíp (80ms)</button>
      </div>
    </div>
  </main>

  <footer>
    Hệ Thống Phân Tán Đa Vi Điều Khiển POC EXP-03 • ESP32 + STM32F4 + Arduino Nano • Dual-Target Architecture
  </footer>

  <script>
    let ws;
    let currentRelayState = 0;

    function initWebSocket() {
      const wsUrl = `ws://${window.location.hostname}/ws`;
      ws = new WebSocket(wsUrl);

      ws.onopen = () => {
        const badge = document.getElementById('ws-badge');
        badge.textContent = 'ONLINE (WS)';
        badge.className = 'badge badge-ok';
      };

      ws.onclose = () => {
        const badge = document.getElementById('ws-badge');
        badge.textContent = 'MẤT KẾT NỐI';
        badge.className = 'badge badge-err';
        setTimeout(initWebSocket, 2000);
      };

      ws.onmessage = (event) => {
        try {
          const data = JSON.parse(event.data);
          updateUI(data);
        } catch (e) {
          console.error(e);
        }
      };
    }

    function updateUI(data) {
      // 1. Cập nhật ESP32 Master
      if (data.master) {
        document.getElementById('master-uptime').textContent = `${data.master.uptime}s`;
        document.getElementById('master-heap').textContent = `${Math.round(data.master.heap / 1024)} KB`;
        document.getElementById('master-ip').textContent = data.master.ip || '192.168.4.1';
        document.getElementById('uart-err-count').textContent = data.master.uart_errs || 0;
        if (data.master.time) document.getElementById('ntp-time').textContent = data.master.time;
      }

      // 2. Cập nhật STM32F4
      if (data.stm32) {
        const onlineEl = document.getElementById('stm32-online');
        if (data.stm32.online) {
          onlineEl.innerHTML = '<span class="status-dot dot-green"></span>ONLINE';
          onlineEl.style.color = 'var(--success)';
        } else {
          onlineEl.innerHTML = '<span class="status-dot dot-red"></span>OFFLINE';
          onlineEl.style.color = 'var(--danger)';
        }
        document.getElementById('stm32-step-state').textContent = data.stm32.moving ? 'DANG QUAY ⚙️' : 'IDLE';
        document.getElementById('stm32-step-pos').textContent = `${data.stm32.pos} bước`;
        if (data.stm32.dsp_time > 0) {
          document.getElementById('stm32-dsp-time').textContent = `${data.stm32.dsp_time} µs`;
        }
      }

      // 3. Cập nhật Arduino Nano
      if (data.nano) {
        const onlineEl = document.getElementById('nano-online');
        if (data.nano.online) {
          onlineEl.innerHTML = '<span class="status-dot dot-green"></span>ONLINE';
          onlineEl.style.color = 'var(--success)';
        } else {
          onlineEl.innerHTML = '<span class="status-dot dot-red"></span>OFFLINE';
          onlineEl.style.color = 'var(--danger)';
        }

        document.getElementById('nano-vcc').textContent = `${(data.nano.vcc / 1000).toFixed(2)} V`;
        
        // ACS712 current reading
        const acsRaw = data.nano.acs712 || 0;
        const acsVolts = (acsRaw / 1023.0) * 5.0;
        const acsAmps = Math.abs((acsVolts - 2.5) / 0.185).toFixed(2);
        document.getElementById('nano-acs712').textContent = `${acsAmps} A (Raw: ${acsRaw})`;
        document.getElementById('acs712-bar').style.width = `${Math.min(100, (acsRaw / 1023) * 100)}%`;

        // 10k Potentiometer
        const potRaw = data.nano.pot || 0;
        const potVolts = ((potRaw / 1023.0) * 5.0).toFixed(2);
        document.getElementById('nano-pot').textContent = `${potVolts} V (Raw: ${potRaw})`;
        document.getElementById('pot-bar').style.width = `${Math.min(100, (potRaw / 1023) * 100)}%`;

        // Buttons
        const b1 = (data.nano.btn & 0x01) ? 'PRESS' : 'UP';
        const b2 = (data.nano.btn & 0x02) ? 'PRESS' : 'UP';
        document.getElementById('nano-btn').textContent = `BTN1: ${b1} | BTN2: ${b2}`;

        // Relay button update
        currentRelayState = data.nano.relay || 0;
        const rBtn = document.getElementById('btn-relay');
        if (currentRelayState === 1) {
          rBtn.textContent = 'Relay: ĐANG BẬT [ON]';
          rBtn.className = 'btn-success';
        } else {
          rBtn.textContent = 'Relay: ĐANG TẮT [OFF]';
          rBtn.className = 'btn-secondary';
        }
      }
    }

    function sendCmd(cmd, payload) {
      if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ cmd, ...payload }));
      } else {
        alert('Chưa kết nối WebSocket tới ESP32 Master!');
      }
    }

    function toggleRelay() {
      const nextState = (currentRelayState === 1) ? 0 : 1;
      sendCmd('relay', { idx: 0, state: nextState });
    }

    window.onload = initWebSocket;
  </script>
</body>
</html>
)rawliteral";

#endif // WEB_DASHBOARD_H
