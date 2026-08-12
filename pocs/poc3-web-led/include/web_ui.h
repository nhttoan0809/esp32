#pragma once

#include <Arduino.h>

const char WEB_UI[] PROGMEM = R"HTML(
<!doctype html>
<html lang="vi">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 LED Control</title>
  <style>
    :root { color-scheme: dark; font-family: system-ui, sans-serif; }
    * { box-sizing: border-box; }
    body {
      min-height: 100vh;
      margin: 0;
      display: grid;
      place-items: center;
      background: #101827;
      color: #e8eef8;
    }
    main {
      width: min(92vw, 28rem);
      padding: 2rem;
      border: 1px solid #344258;
      border-radius: 1rem;
      background: #182235;
      box-shadow: 0 1rem 3rem #0005;
    }
    h1 { margin: 0 0 .4rem; font-size: 1.6rem; }
    .device { margin: 0 0 2rem; color: #aebbd0; }
    .badge {
      display: inline-block;
      margin-left: .4rem;
      padding: .15rem .55rem;
      border-radius: 999px;
      background: #713838;
      color: #ffd5d5;
      font-size: .8rem;
      font-weight: 700;
    }
    .badge.online { background: #174d38; color: #9ff5ce; }
    .control {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 1rem;
    }
    #ledToggle {
      width: 4.25rem;
      height: 2.35rem;
      padding: .2rem;
      border: 0;
      border-radius: 999px;
      background: #617086;
      cursor: pointer;
      transition: background .2s;
    }
    #ledToggle::before {
      content: "";
      display: block;
      width: 1.95rem;
      height: 1.95rem;
      border-radius: 50%;
      background: white;
      transition: transform .2s;
    }
    #ledToggle[aria-pressed="true"] { background: #22a06b; }
    #ledToggle[aria-pressed="true"]::before { transform: translateX(1.9rem); }
    #ledToggle:focus-visible { outline: .2rem solid #73b7ff; outline-offset: .2rem; }
    #ledToggle:disabled { cursor: wait; opacity: .55; }
    #ledState { margin: 1.5rem 0 .5rem; font-size: 1.15rem; font-weight: 700; }
    #error { min-height: 1.4rem; margin: 0; color: #ff9c9c; }
  </style>
</head>
<body>
  <main>
    <h1>ESP32 Dev Module</h1>
    <p class="device">GPIO2 LED <span id="connection" class="badge">Offline</span></p>
    <div class="control">
      <span id="toggleLabel">Công tắc đèn</span>
      <button id="ledToggle" type="button" role="switch"
              aria-labelledby="toggleLabel" aria-pressed="false" disabled></button>
    </div>
    <p id="ledState" aria-live="polite">Đang đọc trạng thái…</p>
    <p id="error" role="alert"></p>
  </main>
  <script>
    const toggle = document.querySelector('#ledToggle');
    const stateText = document.querySelector('#ledState');
    const connection = document.querySelector('#connection');
    const errorText = document.querySelector('#error');

    function renderState(payload) {
      const isOn = payload.led.on === true;
      toggle.setAttribute('aria-pressed', String(isOn));
      stateText.textContent = isOn ? 'Đèn đang bật' : 'Đèn đang tắt';
    }

    function setOnline(online) {
      connection.textContent = online ? 'Online' : 'Offline';
      connection.classList.toggle('online', online);
    }

    async function request(path, options = {}) {
      const response = await fetch(path, options);
      if (!response.ok) throw new Error(`HTTP ${response.status}`);
      return response.json();
    }

    async function loadState(keepError = false) {
      toggle.disabled = true;
      if (!keepError) errorText.textContent = '';
      try {
        const payload = await request('/api/led');
        renderState(payload);
        setOnline(true);
      } catch (error) {
        setOnline(false);
        if (!keepError) errorText.textContent = `Không đọc được trạng thái: ${error.message}`;
      } finally {
        toggle.disabled = false;
      }
    }

    async function setLed(nextState) {
      toggle.disabled = true;
      errorText.textContent = '';
      try {
        const path = nextState ? '/api/led/on' : '/api/led/off';
        const payload = await request(path, { method: 'POST' });
        renderState(payload);
        setOnline(true);
      } catch (error) {
        errorText.textContent = `Không đổi được trạng thái: ${error.message}`;
        await loadState(true);
      } finally {
        toggle.disabled = false;
      }
    }

    toggle.addEventListener('click', () => {
      const currentState = toggle.getAttribute('aria-pressed') === 'true';
      setLed(!currentState);
    });

    loadState();
  </script>
</body>
</html>
)HTML";
