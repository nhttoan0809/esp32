import json
import logging
import asyncio
from typing import Dict, List, Set
from fastapi import FastAPI, WebSocket, WebSocketDisconnect, Request
from fastapi.responses import HTMLResponse, JSONResponse
from fastapi.staticfiles import StaticFiles
from fastapi.templating import Jinja2Templates
from pydantic import BaseModel

from .model import TelemetrySample, PredictionResult, PredictiveClimateEngine

logging.basicConfig(level=logging.INFO, format="%(asctime)s [%(levelname)s] %(message)s")
logger = logging.getLogger("climate_server")

app = FastAPI(title="AI-02 Predictive Climate Optimizer Server")

# Khởi tạo mô hình AI cho từng thiết bị
engines: Dict[str, PredictiveClimateEngine] = {}
latest_predictions: Dict[str, PredictionResult] = {}
device_history: Dict[str, List[Dict]] = {}
active_device_ws: Dict[str, WebSocket] = {}
active_ui_clients: Set[WebSocket] = set()

# Biến cờ ghi đè thủ công (Manual override)
manual_override: Dict[str, Dict] = {}

def get_engine(device_id: str) -> PredictiveClimateEngine:
    if device_id not in engines:
        engines[device_id] = PredictiveClimateEngine(window_size=30, min_cycle_seconds=20.0)
        device_history[device_id] = []
    return engines[device_id]

@app.get("/", response_class=HTMLResponse)
async def get_dashboard():
    # Giao diện web trực quan, tự chứa toàn bộ CSS và JS
    return HTMLResponse(content=DASHBOARD_HTML)

@app.get("/api/status/{device_id}")
async def get_device_status(device_id: str):
    is_online = device_id in active_device_ws
    prediction = latest_predictions.get(device_id)
    history = device_history.get(device_id, [])[-20:]
    return JSONResponse({
        "device_id": device_id,
        "online": is_online,
        "prediction": prediction.model_dump() if prediction else None,
        "history": history,
        "manual_override": manual_override.get(device_id)
    })

@app.post("/api/override/{device_id}")
async def set_override(device_id: str, request: Request):
    data = await request.json()
    enabled = data.get("enabled", False)
    fan = data.get("relay1_fan", False)
    heat = data.get("relay2_heat", False)
    
    manual_override[device_id] = {
        "enabled": enabled,
        "relay1_fan": fan,
        "relay2_heat": heat
    }
    
    # Nếu thiết bị đang kết nối, gửi lệnh ngay lập tức
    if device_id in active_device_ws and enabled:
        cmd = {
            "type": "climate_prediction",
            "predicted_temp_15m": 0.0,
            "predicted_temp_30m": 0.0,
            "trend": "MANUAL",
            "comfort_status": "OVERRIDE",
            "heat_index": 0.0,
            "dew_point": 0.0,
            "relay1_fan": fan,
            "relay2_heat": heat,
            "optimization_mode": "MANUAL_OVERRIDE",
            "reason": "Điều khiển thủ công từ Web Dashboard"
        }
        await active_device_ws[device_id].send_text(json.dumps(cmd))
        
    return JSONResponse({"status": "ok", "override": manual_override[device_id]})

@app.websocket("/ws/climate/{device_id}")
async def websocket_device_endpoint(websocket: WebSocket, device_id: str):
    await websocket.accept()
    active_device_ws[device_id] = websocket
    logger.info(f"ESP32 Connected: {device_id}")
    
    # Gửi gói tin chào mừng
    await websocket.send_text(json.dumps({
        "type": "welcome",
        "device_id": device_id,
        "message": "Connected to AI Predictive Climate Optimizer"
    }))
    
    engine = get_engine(device_id)
    
    try:
        while True:
            text_data = await websocket.receive_text()
            try:
                raw_json = json.loads(text_data)
                sample = TelemetrySample(**raw_json)
            except Exception as e:
                logger.warning(f"Invalid telemetry from {device_id}: {e}")
                continue
            
            # Đánh giá qua mô hình AI
            prediction = engine.evaluate(sample)
            latest_predictions[device_id] = prediction
            
            # Lưu lịch sử để vẽ biểu đồ
            hist_entry = {
                "timestamp": sample.timestamp,
                "rtc_time": sample.rtc_time_str or "",
                "temp": sample.temperature,
                "hum": sample.humidity,
                "light": sample.light_level,
                "pred_15m": prediction.predicted_temp_15m,
                "pred_30m": prediction.predicted_temp_30m,
                "fan": sample.relay1_fan,
                "heat": sample.relay2_heat,
                "trend": prediction.trend,
                "mode": prediction.optimization_mode
            }
            device_history[device_id].append(hist_entry)
            if len(device_history[device_id]) > 60:
                device_history[device_id].pop(0)
                
            # Kiểm tra xem có đang bị ghi đè thủ công không
            override = manual_override.get(device_id, {})
            if override.get("enabled", False):
                fan_target = override.get("relay1_fan", False)
                heat_target = override.get("relay2_heat", False)
                reason_msg = "Ghi đè thủ công từ Dashboard."
                opt_mode = "MANUAL_OVERRIDE"
            else:
                fan_target = prediction.recommended_relay1_fan
                heat_target = prediction.recommended_relay2_heat
                reason_msg = prediction.reason
                opt_mode = prediction.optimization_mode
                
            # Đóng gói lệnh phản hồi cho ESP32
            response_payload = {
                "type": "climate_prediction",
                "predicted_temp_15m": prediction.predicted_temp_15m,
                "predicted_temp_30m": prediction.predicted_temp_30m,
                "trend": prediction.trend,
                "comfort_status": prediction.comfort_status,
                "heat_index": prediction.heat_index,
                "dew_point": prediction.dew_point,
                "relay1_fan": fan_target,
                "relay2_heat": heat_target,
                "optimization_mode": opt_mode,
                "reason": reason_msg
            }
            
            await websocket.send_text(json.dumps(response_payload))
            logger.info(f"[{device_id}] T={sample.temperature}°C, Pred(+30m)={prediction.predicted_temp_30m}°C, Fan={fan_target}, Mode={opt_mode}")
            
            # Broadcast dữ liệu tới Web UI
            ui_payload = json.dumps({
                "type": "update",
                "device_id": device_id,
                "telemetry": hist_entry,
                "prediction": prediction.model_dump(),
                "override": override
            })
            for client in list(active_ui_clients):
                try:
                    await client.send_text(ui_payload)
                except Exception:
                    active_ui_clients.discard(client)
                    
    except WebSocketDisconnect:
        logger.info(f"ESP32 Disconnected: {device_id}")
    finally:
        active_device_ws.pop(device_id, None)

@app.websocket("/ws/ui")
async def websocket_ui_endpoint(websocket: WebSocket):
    await websocket.accept()
    active_ui_clients.add(websocket)
    try:
        while True:
            await websocket.receive_text()
    except WebSocketDisconnect:
        pass
    finally:
        active_ui_clients.discard(websocket)

DASHBOARD_HTML = """<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>AI-02: Predictive Climate Optimizer Dashboard</title>
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style>
    :root {
      --bg: #f8fafc;
      --card-bg: #ffffff;
      --text: #0f172a;
      --muted: #64748b;
      --primary: #0284c7;
      --accent: #10b981;
      --warn: #f59e0b;
      --danger: #ef4444;
      --border: #e2e8f0;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
    body { background: var(--bg); color: var(--text); padding: 24px; }
    .header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 24px; padding-bottom: 16px; border-bottom: 1px solid var(--border); }
    .title-box h1 { font-size: 1.5rem; font-weight: 700; color: #0369a1; }
    .title-box p { font-size: 0.875rem; color: var(--muted); }
    .badge { padding: 6px 12px; border-radius: 9999px; font-size: 0.75rem; font-weight: 600; text-transform: uppercase; }
    .badge-online { background: #dcfce7; color: #15803d; }
    .badge-offline { background: #fee2e2; color: #b91c1c; }
    
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); gap: 16px; margin-bottom: 24px; }
    .card { background: var(--card-bg); border: 1px solid var(--border); border-radius: 12px; padding: 18px; box-shadow: 0 1px 3px rgba(0,0,0,0.05); }
    .card-label { font-size: 0.8rem; text-transform: uppercase; color: var(--muted); font-weight: 600; margin-bottom: 8px; }
    .card-val { font-size: 1.8rem; font-weight: 700; color: var(--text); }
    .card-sub { font-size: 0.8rem; color: var(--muted); margin-top: 4px; }
    
    .highlight-pred { color: #0284c7; }
    .highlight-comfort { color: #10b981; }
    .highlight-warn { color: #f59e0b; }
    
    .chart-container { background: var(--card-bg); border: 1px solid var(--border); border-radius: 12px; padding: 20px; margin-bottom: 24px; }
    .decision-box { background: #f0f9ff; border: 1px solid #bae6fd; border-radius: 12px; padding: 16px; margin-bottom: 24px; }
    .decision-title { font-weight: 600; color: #0369a1; margin-bottom: 4px; display: flex; align-items: center; gap: 8px; }
    .decision-desc { font-size: 0.95rem; color: #0c4a6e; }

    .control-row { display: flex; gap: 16px; align-items: center; margin-top: 12px; }
    .btn { padding: 8px 16px; border-radius: 6px; font-weight: 600; cursor: pointer; border: none; font-size: 0.875rem; }
    .btn-primary { background: #0284c7; color: white; }
    .btn-secondary { background: #e2e8f0; color: var(--text); }
  </style>
</head>
<body>
  <div class="header">
    <div class="title-box">
      <h1>AI-02: Predictive Climate Optimizer</h1>
      <p>Hệ thống giám sát vi khí hậu & Điều khiển làm mát đón đầu (Pre-cooling) bằng AI</p>
    </div>
    <div>
      <span id="conn-badge" class="badge badge-offline">ESP32 Offline</span>
    </div>
  </div>

  <div class="decision-box">
    <div class="decision-title">
      <span>🌿</span>
      <span>Quyết Định Tối Ưu Đón Đầu Của AI (Predictive Actuation):</span>
      <span id="ai-mode-tag" class="badge badge-online">NORMAL</span>
    </div>
    <div id="ai-reason" class="decision-desc">Đang chờ kết nối từ thiết bị ESP32...</div>
  </div>

  <div class="grid">
    <div class="card">
      <div class="card-label">Nhiệt độ Hiện tại</div>
      <div id="val-temp" class="card-val">--.- °C</div>
      <div id="val-trend" class="card-sub">Xu hướng: Đang đồng bộ...</div>
    </div>
    <div class="card">
      <div class="card-label">Dự Báo AI (+30 Phút)</div>
      <div id="val-pred30" class="card-val highlight-pred">--.- °C</div>
      <div id="val-pred15" class="card-sub">Dự báo 15p: --.- °C</div>
    </div>
    <div class="card">
      <div class="card-label">Độ Ẩm & Điểm Sương</div>
      <div id="val-hum" class="card-val">-- %</div>
      <div id="val-dew" class="card-sub">Điểm sương: --.- °C</div>
    </div>
    <div class="card">
      <div class="card-label">Quang Trở & Giờ RTC</div>
      <div id="val-light" class="card-val">--</div>
      <div id="val-rtc" class="card-sub">RTC: --:--:--</div>
    </div>
    <div class="card">
      <div class="card-label">Relay 1 (Quạt / Cooling)</div>
      <div id="val-relay1" class="card-val">TẮT</div>
      <div class="card-sub">Điều khiển bởi: AI Model</div>
    </div>
    <div class="card">
      <div class="card-label">Relay 2 (Sưởi / Heat)</div>
      <div id="val-relay2" class="card-val">TẮT</div>
      <div class="card-sub">Điều khiển bởi: AI Model</div>
    </div>
  </div>

  <div class="chart-container">
    <h3 style="font-size: 1rem; margin-bottom: 12px; color: #334155;">Đồ Thị Nhiệt Độ Thực Tế vs Dự Báo Tương Lai Của AI</h3>
    <canvas id="climateChart" height="90"></canvas>
  </div>

  <script>
    const ctx = document.getElementById('climateChart').getContext('2d');
    const maxDataPoints = 25;
    const chartData = {
      labels: [],
      datasets: [
        { label: 'Nhiệt độ đo (°C)', data: [], borderColor: '#0284c7', backgroundColor: 'rgba(2, 132, 199, 0.1)', tension: 0.3, fill: true },
        { label: 'AI Dự báo +30m (°C)', data: [], borderColor: '#f59e0b', borderDash: [5, 5], tension: 0.3, fill: false },
        { label: 'Độ ẩm (%RH)', data: [], borderColor: '#10b981', borderDash: [2, 2], tension: 0.3, fill: false, yAxisID: 'y1' }
      ]
    };
    const climateChart = new Chart(ctx, {
      type: 'line',
      data: chartData,
      options: {
        responsive: true,
        scales: {
          y: { title: { display: true, text: 'Nhiệt độ (°C)' }, min: 15, max: 42 },
          y1: { position: 'right', title: { display: true, text: 'Độ ẩm (%)' }, min: 20, max: 100, grid: { drawOnChartArea: false } }
        }
      }
    });

    const wsProtocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const uiWs = new WebSocket(`${wsProtocol}//${window.location.host}/ws/ui`);

    uiWs.onmessage = (event) => {
      const msg = JSON.parse(event.data);
      if (msg.type === 'update') {
        const t = msg.telemetry;
        const p = msg.prediction;
        
        document.getElementById('conn-badge').textContent = 'ESP32 Online';
        document.getElementById('conn-badge').className = 'badge badge-online';
        
        document.getElementById('val-temp').textContent = `${t.temp.toFixed(1)} °C`;
        document.getElementById('val-trend').textContent = `Xu hướng: ${p.trend} (${p.rate_of_change_temp > 0 ? '+' : ''}${p.rate_of_change_temp}°C/min)`;
        document.getElementById('val-pred30').textContent = `${p.predicted_temp_30m.toFixed(1)} °C`;
        document.getElementById('val-pred15').textContent = `Dự báo 15p: ${p.predicted_temp_15m.toFixed(1)} °C`;
        document.getElementById('val-hum').textContent = `${t.hum.toFixed(0)} %`;
        document.getElementById('val-dew').textContent = `Điểm sương: ${p.dew_point.toFixed(1)} °C`;
        document.getElementById('val-light').textContent = t.light === 1 ? 'SÁNG ☀️' : 'TỐI 🌙';
        document.getElementById('val-rtc').textContent = `RTC: ${t.rtc_time || '--:--:--'}`;
        
        const r1 = p.recommended_relay1_fan;
        const r2 = p.recommended_relay2_heat;
        document.getElementById('val-relay1').textContent = r1 ? 'BẬT (ON)' : 'TẮT (OFF)';
        document.getElementById('val-relay1').style.color = r1 ? '#0284c7' : '#0f172a';
        document.getElementById('val-relay2').textContent = r2 ? 'BẬT (ON)' : 'TẮT (OFF)';
        document.getElementById('val-relay2').style.color = r2 ? '#f59e0b' : '#0f172a';
        
        document.getElementById('ai-mode-tag').textContent = p.optimization_mode;
        document.getElementById('ai-reason').textContent = p.reason;
        
        const timeLabel = t.rtc_time || new Date().toLocaleTimeString();
        chartData.labels.push(timeLabel);
        chartData.datasets[0].data.push(t.temp);
        chartData.datasets[1].data.push(p.predicted_temp_30m);
        chartData.datasets[2].data.push(t.hum);
        
        if (chartData.labels.length > maxDataPoints) {
          chartData.labels.shift();
          chartData.datasets[0].data.shift();
          chartData.datasets[1].data.shift();
          chartData.datasets[2].data.shift();
        }
        climateChart.update();
      }
    };

    uiWs.onclose = () => {
      document.getElementById('conn-badge').textContent = 'Web Hub Offline';
      document.getElementById('conn-badge').className = 'badge badge-offline';
    };
  </script>
</body>
</html>
"""
