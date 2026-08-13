#pragma once

#include <Arduino.h>

const char PROVISIONING_PAGE[] PROGMEM = R"HTML(
<!doctype html>
<html lang="vi">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>ESP32 Wi-Fi Setup</title>
  <style>
    :root { color-scheme: light dark; font-family: system-ui, sans-serif; }
    body { margin: 0; padding: 24px; background: #0f172a; color: #e2e8f0; }
    main { max-width: 520px; margin: auto; padding: 24px; border-radius: 16px;
      background: #1e293b; box-shadow: 0 12px 40px #0008; }
    h1 { margin-top: 0; font-size: 1.5rem; }
    label { display: block; margin: 16px 0 6px; }
    input, button { box-sizing: border-box; width: 100%; padding: 12px;
      border: 1px solid #64748b; border-radius: 8px; font: inherit; }
    button { margin-top: 16px; cursor: pointer; border: 0;
      background: #38bdf8; color: #082f49; font-weight: 700; }
    button.danger { background: #fda4af; color: #4c0519; }
    #status { margin: 18px 0; padding: 12px; border-radius: 8px;
      background: #0f172a; white-space: pre-wrap; }
    .notice { color: #fbbf24; }
  </style>
</head>
<body>
<main>
  <h1>ESP32 Wi-Fi Setup</h1>
  <p id="device">Đang đọc thông tin thiết bị…</p>
  <p class="notice">SoftAP này chỉ dùng để cấu hình và không chia sẻ Internet.</p>
  <div id="status">Chưa cấu hình</div>
  <form id="wifi-form">
    <label for="ssid">SSID Wi-Fi</label>
    <input id="ssid" name="ssid" maxlength="32" required autocomplete="off">
    <label for="password">Password (để trống cho mạng open)</label>
    <input id="password" name="password" type="password" maxlength="63"
      autocomplete="new-password">
    <button type="submit">Kết nối</button>
  </form>
  <button id="reset" class="danger" type="button">Xóa cấu hình</button>
</main>
<script>
  const statusBox = document.querySelector('#status');
  const deviceBox = document.querySelector('#device');
  const form = document.querySelector('#wifi-form');
  const passwordField = document.querySelector('#password');

  async function readStatus() {
    try {
      const response = await fetch('/api/status', { cache: 'no-store' });
      const data = await response.json();
      deviceBox.textContent = `${data.ap.ssid} — ${data.ap.ip}`;
      const labels = {
        provisioning: 'Chưa cấu hình', connecting: 'Đang kết nối',
        verifying: 'Đang kiểm tra Internet', connected: 'Đã kết nối',
        failed: 'Thất bại'
      };
      let text = labels[data.state] || data.state;
      if (data.sta.ip) text += `\nSTA IP: ${data.sta.ip}`;
      if (data.error) text += `\nLỗi: ${data.error}`;
      statusBox.textContent = text;
    } catch (_) {
      statusBox.textContent = 'Mất kết nối tạm thời; hãy nối lại SoftAP và tải lại trang.';
    }
  }

  form.addEventListener('submit', async (event) => {
    event.preventDefault();
    const body = new URLSearchParams(new FormData(form));
    passwordField.value = '';
    try {
      const response = await fetch('/api/wifi/configure', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body
      });
      const data = await response.json();
      statusBox.textContent = response.ok ? 'Đang kết nối' : `Lỗi: ${data.error}`;
    } catch (_) {
      statusBox.textContent = 'SoftAP đổi channel hoặc mất kết nối; hãy nối lại và thử lại.';
    }
  });

  document.querySelector('#reset').addEventListener('click', async () => {
    try {
      const response = await fetch('/api/wifi/reset', { method: 'POST' });
      const data = await response.json();
      statusBox.textContent = response.ok ? 'Chưa cấu hình' : `Lỗi: ${data.error}`;
      await readStatus();
    } catch (_) {
      statusBox.textContent = 'Không gửi được yêu cầu reset; hãy nối lại SoftAP.';
    }
  });

  readStatus();
  setInterval(readStatus, 1000);
</script>
</body>
</html>
)HTML";
