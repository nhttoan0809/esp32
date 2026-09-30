#include "web_portal.h"
#include "config.h"
#include "auth_manager.h"
#include "lock_actuator.h"
#include "rtc_manager.h"
#include "feedback.h"
#include <ArduinoJson.h>

WebPortal webPortal;

// Embedded HTML Dashboard
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>SMH-02 Tri-Factor Smart Lock</title>
  <style>
    :root {
      --bg: #0f172a;
      --card-bg: #1e293b;
      --text: #f8fafc;
      --text-muted: #94a3b8;
      --primary: #3b82f6;
      --primary-hover: #2563eb;
      --danger: #ef4444;
      --success: #22c55e;
      --warning: #f59e0b;
      --border: #334155;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
    body { background-color: var(--bg); color: var(--text); padding: 1.5rem 1rem; }
    .container { max-width: 800px; margin: 0 auto; display: flex; flex-direction: column; gap: 1.25rem; }
    header { text-align: center; margin-bottom: 0.5rem; }
    header h1 { font-size: 1.6rem; color: #fff; margin-bottom: 0.25rem; }
    header p { font-size: 0.9rem; color: var(--text-muted); }
    .status-card {
      background: var(--card-bg);
      border-radius: 12px;
      padding: 1.5rem;
      border: 1px solid var(--border);
      text-align: center;
      box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.2);
    }
    .status-badge {
      display: inline-block;
      font-size: 1.25rem;
      font-weight: 700;
      padding: 0.5rem 1.5rem;
      border-radius: 9999px;
      margin: 0.75rem 0;
      letter-spacing: 0.05em;
    }
    .status-locked { background: rgba(239, 68, 68, 0.2); color: var(--danger); border: 1px solid var(--danger); }
    .status-unlocked { background: rgba(34, 197, 94, 0.2); color: var(--success); border: 1px solid var(--success); }
    .status-lockout { background: rgba(245, 158, 11, 0.2); color: var(--warning); border: 1px solid var(--warning); }
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(320px, 1fr)); gap: 1.25rem; }
    .card { background: var(--card-bg); border-radius: 12px; padding: 1.25rem; border: 1px solid var(--border); }
    .card h2 { font-size: 1.1rem; margin-bottom: 0.75rem; border-bottom: 1px solid var(--border); padding-bottom: 0.5rem; display: flex; align-items: center; gap: 0.5rem; }
    .form-group { margin-bottom: 0.85rem; }
    label { display: block; font-size: 0.85rem; color: var(--text-muted); margin-bottom: 0.35rem; }
    input[type="text"], input[type="password"] {
      width: 100%;
      padding: 0.6rem 0.75rem;
      background: #0f172a;
      border: 1px solid var(--border);
      border-radius: 6px;
      color: #fff;
      font-size: 0.95rem;
    }
    button {
      cursor: pointer;
      font-weight: 600;
      padding: 0.65rem 1.2rem;
      border-radius: 6px;
      border: none;
      transition: background 0.15s ease;
      font-size: 0.95rem;
      width: 100%;
    }
    .btn-primary { background: var(--primary); color: #fff; }
    .btn-primary:hover { background: var(--primary-hover); }
    .btn-success { background: var(--success); color: #fff; }
    .btn-danger { background: var(--danger); color: #fff; }
    table { width: 100%; border-collapse: collapse; margin-top: 0.5rem; font-size: 0.85rem; }
    th, td { text-align: left; padding: 0.5rem; border-bottom: 1px solid var(--border); }
    th { color: var(--text-muted); }
    .badge-ok { color: var(--success); font-weight: bold; }
    .badge-fail { color: var(--danger); font-weight: bold; }
    .msg-box { margin-top: 0.5rem; font-size: 0.85rem; min-height: 1.2rem; text-align: center; }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <h1>🛡️ SMH-02 Smart Access Lock</h1>
      <p>Tri-Factor Security Portal (RFID • PIN • Web)</p>
    </header>

    <div class="status-card">
      <div id="rtc-time" style="font-size: 1.1rem; color: var(--text-muted);">Syncing time...</div>
      <div id="status-badge" class="status-badge status-locked">🔒 LOCKED</div>
      <div id="relock-timer" style="font-size: 0.95rem; color: var(--text-muted);">Chốt khóa cơ khí đang gài an toàn (0°)</div>
    </div>

    <div class="grid">
      <!-- Remote Unlock Card -->
      <div class="card">
        <h2>🌐 Mở Khóa Từ Xa (Factor 3)</h2>
        <div class="form-group">
          <label>Mật Khẩu Quản Trị (Admin Password)</label>
          <input type="password" id="unlock-pass" placeholder="Nhập mật khẩu web..." value="admin123">
        </div>
        <button class="btn-success" onclick="remoteUnlock()">🔓 Mở Khóa Cửa (5 Giây)</button>
        <div id="unlock-msg" class="msg-box"></div>
      </div>

      <!-- Security Settings -->
      <div class="card">
        <h2>🔑 Đổi Mã PIN Bàn Phím (Factor 2)</h2>
        <div class="form-group">
          <label>Mật Khẩu Quản Trị Hiện Tại</label>
          <input type="password" id="pin-admin-pass" placeholder="Mật khẩu web hiện tại...">
        </div>
        <div class="form-group">
          <label>Mã PIN Mới (4 - 8 chữ số)</label>
          <input type="text" id="new-pin" placeholder="Ví dụ: 5678" maxlength="8">
        </div>
        <button class="btn-primary" onclick="changePin()">Cập Nhật Mã PIN</button>
        <div id="pin-msg" class="msg-box"></div>
      </div>
    </div>

    <!-- RFID Cards Card -->
    <div class="card">
      <h2>💳 Quản Lý Thẻ Từ RFID (Factor 1)</h2>
      <div style="display: flex; gap: 0.5rem; margin-bottom: 0.75rem;">
        <input type="text" id="new-card-uid" placeholder="Nhập UID thẻ HEX (ví dụ: DEADBEEF)" style="flex: 2;">
        <input type="password" id="card-admin-pass" placeholder="Admin Pass" style="flex: 1;" value="admin123">
        <button class="btn-primary" onclick="addCard()" style="flex: 1;">+ Thêm Thẻ</button>
      </div>
      <div id="card-msg" class="msg-box"></div>
      <div style="overflow-x: auto;">
        <table>
          <thead>
            <tr><th>UID Thẻ</th><th>Loại Thẻ</th><th>Thao Tác</th></tr>
          </thead>
          <tbody id="cards-tbody">
            <tr><td colspan="3">Đang tải danh sách thẻ...</td></tr>
          </tbody>
        </table>
      </div>
    </div>

    <!-- Access Logs Card -->
    <div class="card">
      <h2>📜 Nhật Ký Ra Vào (Access Event Logs)</h2>
      <div style="overflow-x: auto;">
        <table>
          <thead>
            <tr><th>Thời Gian (RTC)</th><th>Phương Thức</th><th>Chi Tiết</th><th>Kết Quả</th></tr>
          </thead>
          <tbody id="logs-tbody">
            <tr><td colspan="4">Đang tải nhật ký...</td></tr>
          </tbody>
        </table>
      </div>
    </div>
  </div>

  <script>
    async function updateStatus() {
      try {
        const res = await fetch('/api/status');
        const data = await res.json();
        
        document.getElementById('rtc-time').innerText = `🕒 RTC: ${data.datetime}`;
        
        const badge = document.getElementById('status-badge');
        const timer = document.getElementById('relock-timer');

        if (data.lockout) {
          badge.className = 'status-badge status-lockout';
          badge.innerText = `⚠️ LOCKOUT (${data.remaining_sec}s)`;
          timer.innerText = 'Cảnh báo an ninh: Bàn phím & RFID đang bị tạm khóa!';
        } else if (data.status === 'UNLOCKED') {
          badge.className = 'status-badge status-unlocked';
          badge.innerText = '🔓 UNLOCKED';
          timer.innerText = `Chốt cửa đang mở! Tự động khóa lại sau ${data.remaining_sec}s...`;
        } else {
          badge.className = 'status-badge status-locked';
          badge.innerText = '🔒 LOCKED';
          timer.innerText = 'Chốt khóa cơ khí đang gài an toàn (0°)';
        }
      } catch (e) {
        console.error("Status fetch error", e);
      }
    }

    async function loadLogs() {
      try {
        const res = await fetch('/api/logs');
        const logs = await res.json();
        const tbody = document.getElementById('logs-tbody');
        if (logs.length === 0) {
          tbody.innerHTML = '<tr><td colspan="4" style="text-align:center;">Chưa có sự kiện nào</td></tr>';
          return;
        }
        tbody.innerHTML = logs.map(l => `
          <tr>
            <td>${l.timestamp}</td>
            <td><strong>${l.method}</strong></td>
            <td>${l.detail}</td>
            <td class="${l.status === 'GRANTED' ? 'badge-ok' : 'badge-fail'}">${l.status}</td>
          </tr>
        `).join('');
      } catch (e) {
        console.error("Logs load error", e);
      }
    }

    async function loadCards() {
      try {
        const res = await fetch('/api/cards');
        const cards = await res.json();
        const tbody = document.getElementById('cards-tbody');
        if (cards.length === 0) {
          tbody.innerHTML = '<tr><td colspan="3" style="text-align:center;">Chưa có thẻ nào</td></tr>';
          return;
        }
        tbody.innerHTML = cards.map(c => `
          <tr>
            <td><code>${c.uid}</code></td>
            <td>${c.is_master ? '⭐ Master Card' : 'Thẻ Người Dùng'}</td>
            <td>
              ${c.is_master ? '-' : `<button class="btn-danger" style="padding:0.25rem 0.5rem; font-size:0.75rem; width:auto;" onclick="removeCard('${c.uid}')">Xóa</button>`}
            </td>
          </tr>
        `).join('');
      } catch (e) {
        console.error("Cards load error", e);
      }
    }

    async function remoteUnlock() {
      const pass = document.getElementById('unlock-pass').value;
      const msg = document.getElementById('unlock-msg');
      msg.innerText = 'Đang xử lý...';
      try {
        const res = await fetch('/api/unlock', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ password: pass })
        });
        const data = await res.json();
        if (data.success) {
          msg.style.color = 'var(--success)';
          msg.innerText = '✅ ' + data.message;
          updateStatus();
          loadLogs();
        } else {
          msg.style.color = 'var(--danger)';
          msg.innerText = '❌ ' + data.message;
          updateStatus();
        }
      } catch (e) {
        msg.style.color = 'var(--danger)';
        msg.innerText = 'Lỗi kết nối máy chủ!';
      }
    }

    async function changePin() {
      const pass = document.getElementById('pin-admin-pass').value;
      const newPin = document.getElementById('new-pin').value;
      const msg = document.getElementById('pin-msg');
      try {
        const res = await fetch('/api/pin', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ password: pass, new_pin: newPin })
        });
        const data = await res.json();
        if (data.success) {
          msg.style.color = 'var(--success)';
          msg.innerText = '✅ ' + data.message;
          document.getElementById('new-pin').value = '';
        } else {
          msg.style.color = 'var(--danger)';
          msg.innerText = '❌ ' + data.message;
        }
      } catch (e) {
        msg.style.color = 'var(--danger)';
        msg.innerText = 'Lỗi kết nối!';
      }
    }

    async function addCard() {
      const uid = document.getElementById('new-card-uid').value.trim();
      const pass = document.getElementById('card-admin-pass').value;
      const msg = document.getElementById('card-msg');
      if (!uid) return;
      try {
        const res = await fetch('/api/cards/add', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ password: pass, uid: uid })
        });
        const data = await res.json();
        if (data.success) {
          msg.style.color = 'var(--success)';
          msg.innerText = '✅ ' + data.message;
          document.getElementById('new-card-uid').value = '';
          loadCards();
        } else {
          msg.style.color = 'var(--danger)';
          msg.innerText = '❌ ' + data.message;
        }
      } catch (e) {
        msg.style.color = 'var(--danger)';
        msg.innerText = 'Lỗi kết nối!';
      }
    }

    async function removeCard(uid) {
      const pass = prompt('Nhập mật khẩu Admin để xác nhận xóa thẻ ' + uid + ':');
      if (!pass) return;
      try {
        const res = await fetch('/api/cards/remove', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ password: pass, uid: uid })
        });
        const data = await res.json();
        alert(data.message);
        loadCards();
      } catch (e) {
        alert('Lỗi kết nối!');
      }
    }

    // Auto refresh status every 2 seconds and logs every 5 seconds
    setInterval(updateStatus, 2000);
    setInterval(loadLogs, 5000);

    // Initial load
    updateStatus();
    loadLogs();
    loadCards();
  </script>
</body>
</html>
)rawliteral";

WebPortal::WebPortal()
    : server(WEB_SERVER_PORT),
      wifiConnected(false),
      lastWifiCheck(0) {}

void WebPortal::begin() {
    // 1. Configure WiFi Station & SoftAP
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASS);

    Serial.print(F("[WIFI] Access Point started: "));
    Serial.println(AP_SSID);
    Serial.print(F("[WIFI] AP IP: "));
    Serial.println(WiFi.softAPIP());

    WiFi.begin(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASS);
    Serial.print(F("[WIFI] Connecting to STA SSID: "));
    Serial.println(DEFAULT_WIFI_SSID);

    setupRoutes();
    server.begin();
    Serial.println(F("[HTTP] Web Server started on port 80"));
}

void WebPortal::setupRoutes() {
    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on("/api/status", HTTP_GET, [this]() { handleApiStatus(); });
    server.on("/api/unlock", HTTP_POST, [this]() { handleApiUnlock(); });
    server.on("/api/logs", HTTP_GET, [this]() { handleApiLogs(); });
    server.on("/api/cards", HTTP_GET, [this]() { handleApiCards(); });
    server.on("/api/cards/add", HTTP_POST, [this]() { handleApiAddCard(); });
    server.on("/api/cards/remove", HTTP_POST, [this]() { handleApiRemoveCard(); });
    server.on("/api/pin", HTTP_POST, [this]() { handleApiChangePin(); });
}

void WebPortal::handleRoot() {
    server.send_P(200, "text/html", INDEX_HTML);
}

void WebPortal::handleApiStatus() {
    JsonDocument doc;
    doc["status"] = lockActuator.isUnlocked() ? "UNLOCKED" : "LOCKED";
    doc["remaining_sec"] = authManager.isLockoutActive() ? 
                           authManager.getRemainingLockoutSeconds() : 
                           lockActuator.getRemainingRelockSeconds();
    doc["lockout"] = authManager.isLockoutActive();
    doc["datetime"] = rtcManager.getFormattedDateTime();

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void WebPortal::handleApiUnlock() {
    if (server.hasArg("plain") == false) {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"Missing body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err) {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
        return;
    }

    String password = doc["password"] | "";
    if (authManager.verifyWebPassword(password)) {
        lockActuator.unlock();
        feedback.beepSuccess();
        server.send(200, "application/json", "{\"success\":true,\"message\":\"Cửa đã mở khóa thành công!\"}");
    } else {
        feedback.beepDenied();
        if (authManager.isLockoutActive()) {
            server.send(403, "application/json", "{\"success\":false,\"message\":\"Sai mật khẩu! Hệ thống đang bị LOCKOUT!\"}");
        } else {
            server.send(401, "application/json", "{\"success\":false,\"message\":\"Sai mật khẩu quản trị!\"}");
        }
    }
}

void WebPortal::handleApiLogs() {
    server.send(200, "application/json", authManager.getLogsJson());
}

void WebPortal::handleApiCards() {
    server.send(200, "application/json", authManager.getCardsJson());
}

void WebPortal::handleApiAddCard() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"Missing body\"}");
        return;
    }
    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));
    String password = doc["password"] | "";
    String uid = doc["uid"] | "";

    if (!authManager.verifyWebPassword(password)) {
        server.send(401, "application/json", "{\"success\":false,\"message\":\"Sai mật khẩu quản trị!\"}");
        return;
    }

    if (authManager.addAuthorizedCard(uid)) {
        feedback.beepSuccess();
        server.send(200, "application/json", "{\"success\":true,\"message\":\"Thêm thẻ thành công!\"}");
    } else {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"Thẻ đã tồn tại hoặc danh bạ đầy!\"}");
    }
}

void WebPortal::handleApiRemoveCard() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"Missing body\"}");
        return;
    }
    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));
    String password = doc["password"] | "";
    String uid = doc["uid"] | "";

    if (!authManager.verifyWebPassword(password)) {
        server.send(401, "application/json", "{\"success\":false,\"message\":\"Sai mật khẩu quản trị!\"}");
        return;
    }

    if (authManager.removeAuthorizedCard(uid)) {
        feedback.beepSuccess();
        server.send(200, "application/json", "{\"success\":true,\"message\":\"Đã xóa thẻ thành công!\"}");
    } else {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"Không tìm thấy thẻ cần xóa!\"}");
    }
}

void WebPortal::handleApiChangePin() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"Missing body\"}");
        return;
    }
    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));
    String password = doc["password"] | "";
    String newPin = doc["new_pin"] | "";

    if (!authManager.verifyWebPassword(password)) {
        server.send(401, "application/json", "{\"success\":false,\"message\":\"Sai mật khẩu quản trị!\"}");
        return;
    }

    if (authManager.setUserPin(newPin)) {
        feedback.beepSuccess();
        server.send(200, "application/json", "{\"success\":true,\"message\":\"Đổi mã PIN thành công!\"}");
    } else {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"Mã PIN không hợp lệ (cần 4-8 số)!\"}");
    }
}

bool WebPortal::isConnected() const {
    return (WiFi.status() == WL_CONNECTED);
}

String WebPortal::getIpAddress() const {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.localIP().toString();
    }
    return WiFi.softAPIP().toString();
}

void WebPortal::update() {
    server.handleClient();

    // Check Wi-Fi state periodically
    if (millis() - lastWifiCheck >= 5000) {
        lastWifiCheck = millis();
        wifiConnected = (WiFi.status() == WL_CONNECTED);
    }
}
