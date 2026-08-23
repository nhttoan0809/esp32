#pragma once

#include <Arduino.h>

constexpr char PROVISIONING_PAGE[] PROGMEM = R"HTML(
<!doctype html>
<html lang="vi">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>ESP32 Setup</title>
  <style>
    :root{font-family:system-ui,sans-serif;color-scheme:light dark}body{margin:0;min-height:100vh;display:grid;place-items:center;background:#0f172a;color:#f8fafc}main{width:min(92vw,460px);box-sizing:border-box;padding:24px;border-radius:16px;background:#1e293b;box-shadow:0 16px 40px #0008}h1{margin-top:0;font-size:1.4rem}label{display:block;margin:12px 0 5px;color:#cbd5e1}input,button{width:100%;box-sizing:border-box;min-height:42px;border-radius:8px;padding:9px 11px;font:inherit}input{border:1px solid #64748b;background:#0f172a;color:#f8fafc}.row{display:grid;grid-template-columns:2fr 1fr;gap:10px}button{margin-top:16px;border:0;background:#38bdf8;color:#082f49;font-weight:700;cursor:pointer}button.secondary{background:#475569;color:#f8fafc}button:disabled{opacity:.55;cursor:wait}pre{white-space:pre-wrap;background:#0f172a;padding:12px;border-radius:8px;min-height:72px}.note{font-size:.9rem;color:#cbd5e1}</style>
</head>
<body>
<main>
  <h1>ESP32 Wi-Fi &amp; Cloud Setup</h1>
  <p class="note">Kết nối cấu hình dùng SoftAP. Cloud chỉ chấp nhận WSS port 443.</p>
  <form id="form">
    <label for="ssid">Wi-Fi SSID</label>
    <input id="ssid" name="ssid" maxlength="32" required autocomplete="off">
    <label for="password">Wi-Fi password</label>
    <input id="password" name="password" type="password" maxlength="63" autocomplete="off">
    <label for="server_host">Server host</label>
    <input id="server_host" name="server_host" placeholder="name.ngrok-free.app" maxlength="253" required autocomplete="off">
    <div class="row">
      <div><label for="server_path">WebSocket base path</label><input id="server_path" name="server_path" value="/ws/devices" maxlength="96" required></div>
      <div><label for="server_port">Port</label><input id="server_port" name="server_port" value="443" inputmode="numeric" required></div>
    </div>
    <button id="submit" type="submit">Kết nối</button>
  </form>
  <button id="reset" class="secondary" type="button">Xóa cấu hình đã lưu</button>
  <pre id="status">Đang tải trạng thái...</pre>
</main>
<script>
const form=document.querySelector('#form'),statusNode=document.querySelector('#status'),submit=document.querySelector('#submit');
async function body(response){const value=await response.json().catch(()=>({}));if(!response.ok)throw new Error(value.error||`HTTP ${response.status}`);return value}
function render(value){statusNode.textContent=[`Device: ${value.device_id}`,`Provisioning: ${value.provisioning}`,`Wi-Fi: ${value.wifi.state}${value.wifi.ip?' / '+value.wifi.ip:''}`,`Cloud: ${value.cloud.state}`,`Real_Device: ${value.real_device.on?'ON':'OFF'}`,value.error?`Error: ${value.error}`:''].filter(Boolean).join('\n')}
async function refresh(){try{render(await body(await fetch('/api/status',{cache:'no-store'})))}catch(error){statusNode.textContent=`Mất kết nối portal: ${error.message}`}}
form.addEventListener('submit',async event=>{event.preventDefault();submit.disabled=true;const password=document.querySelector('#password');try{const encoded=new URLSearchParams(new FormData(form));password.value='';await body(await fetch('/api/configure',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:encoded}));await refresh()}catch(error){statusNode.textContent=`Lỗi: ${error.message}`}finally{submit.disabled=false}});
document.querySelector('#reset').addEventListener('click',async()=>{if(!confirm('Xóa Wi-Fi và server config đã lưu?'))return;try{await body(await fetch('/api/reset',{method:'POST'}));await refresh()}catch(error){statusNode.textContent=`Lỗi: ${error.message}`}});
refresh();setInterval(refresh,1000);
</script>
</body>
</html>
)HTML";
