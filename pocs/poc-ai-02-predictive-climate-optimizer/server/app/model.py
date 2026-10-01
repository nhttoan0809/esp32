import time
import math
from typing import List, Dict, Any, Optional, Tuple
from pydantic import BaseModel, Field

class TelemetrySample(BaseModel):
    timestamp: float = Field(default_factory=time.time)
    temperature: float
    humidity: float
    light_level: int = 1  # 0: Dark, 1: Bright
    light_analog: Optional[int] = None  # 0-4095
    rtc_timestamp: Optional[int] = None
    rtc_time_str: Optional[str] = None
    relay1_fan: bool = False
    relay2_heat: bool = False

class PredictionResult(BaseModel):
    current_temp: float
    current_hum: float
    light_level: int
    heat_index: float
    dew_point: float
    trend: str  # RISING_FAST, RISING, STABLE, FALLING, FALLING_FAST
    rate_of_change_temp: float  # °C/min
    rate_of_change_hum: float   # %RH/min
    predicted_temp_15m: float
    predicted_temp_30m: float
    comfort_status: str
    optimization_mode: str  # PRE_COOLING, NORMAL, EARLY_CUTOFF, HEATING, MOLD_VENTILATION
    recommended_relay1_fan: bool
    recommended_relay2_heat: bool
    reason: str
    timestamp: float = Field(default_factory=time.time)

class PredictiveClimateEngine:
    """
    Mô hình học máy / phân tích chuỗi thời gian dự báo vi khí hậu và tối ưu đón đầu (Pre-emptive Optimization).
    Duy trì bộ đệm trượt N điểm đo và tính toán các đạo hàm thời gian thực.
    """
    def __init__(self, window_size: int = 30, min_cycle_seconds: float = 30.0):
        self.window_size = window_size
        self.min_cycle_seconds = min_cycle_seconds
        self.buffer: List[TelemetrySample] = []
        self.last_relay_change_time: float = 0.0
        self.last_relay1_state: bool = False
        self.last_relay2_state: bool = False

    def add_sample(self, sample: TelemetrySample):
        self.buffer.append(sample)
        if len(self.buffer) > self.window_size:
            self.buffer.pop(0)

    @staticmethod
    def compute_dew_point(temp_c: float, hum_rh: float) -> float:
        """Tính điểm sương theo công thức thực nghiệm Magnus-Tetens"""
        hum_rh = max(1.0, min(100.0, hum_rh))
        a = 17.27
        b = 237.7
        alpha = ((a * temp_c) / (b + temp_c)) + math.log(hum_rh / 100.0)
        dew_point = (b * alpha) / (a - alpha)
        return round(dew_point, 1)

    @staticmethod
    def compute_heat_index(temp_c: float, hum_rh: float) -> float:
        """Tính chỉ số nhiệt cảm nhận (Heat Index) theo công thức Rothfusz"""
        if temp_c < 20.0:
            return round(temp_c, 1)
        
        # Chuyển đổi sang Fahrenheit để áp dụng công thức chuẩn NOAA
        t_f = temp_c * 9.0 / 5.0 + 32.0
        r = hum_rh
        hi_f = 0.5 * (t_f + 61.0 + ((t_f - 68.0) * 1.2) + (r * 0.094))
        
        if hi_f >= 80.0:
            hi_f = (-42.379 + 2.04901523 * t_f + 10.14333127 * r
                    - 0.22475541 * t_f * r - 0.00683783 * t_f * t_f
                    - 0.05481717 * r * r + 0.00122874 * t_f * t_f * r
                    + 0.00085282 * t_f * r * r - 0.00000199 * t_f * t_f * r * r)
        
        hi_c = (hi_f - 32.0) * 5.0 / 9.0
        return round(hi_c, 1)

    def compute_derivatives(self) -> Tuple[float, float]:
        """
        Tính tốc độ biến thiên nhiệt độ (dT/dt tính bằng °C/phút) và độ ẩm (dH/dt tính bằng %/phút)
        sử dụng hồi quy tuyến tính (Ordinary Least Squares) trên bộ đệm trượt.
        """
        if len(self.buffer) < 2:
            return 0.0, 0.0
        
        n = len(self.buffer)
        t_ref = self.buffer[0].timestamp
        times = [(s.timestamp - t_ref) / 60.0 for s in self.buffer]  # Đơn vị phút
        temps = [s.temperature for s in self.buffer]
        hums = [s.humidity for s in self.buffer]
        
        mean_t = sum(times) / n
        mean_temp = sum(temps) / n
        mean_hum = sum(hums) / n
        
        denom = sum((t - mean_t) ** 2 for t in times)
        if denom < 1e-6:
            return 0.0, 0.0
        
        slope_temp = sum((times[i] - mean_t) * (temps[i] - mean_temp) for i in range(n)) / denom
        slope_hum = sum((times[i] - mean_t) * (hums[i] - mean_hum) for i in range(n)) / denom
        
        return round(slope_temp, 3), round(slope_hum, 3)

    def evaluate(self, current_sample: TelemetrySample) -> PredictionResult:
        """
        Thực hiện toàn bộ quy trình: phân tích chuỗi thời gian, dự báo 15m & 30m,
        và đưa ra quyết định điều khiển đón đầu (Pre-emptive Actuation).
        """
        self.add_sample(current_sample)
        
        t_now = current_sample.temperature
        h_now = current_sample.humidity
        light_now = current_sample.light_level
        fan_now = current_sample.relay1_fan
        heat_now = current_sample.relay2_heat
        
        # 1. Tính toán các chỉ số vật lý vi khí hậu
        heat_idx = self.compute_heat_index(t_now, h_now)
        dew_pt = self.compute_dew_point(t_now, h_now)
        
        # 2. Đạo hàm biến thiên thời gian thực (°C/phút & %/phút)
        dt_dt, dh_dt = self.compute_derivatives()
        
        # 3. Phân loại xu hướng (Trend classification)
        if dt_dt > 0.15:
            trend = "RISING_FAST"
        elif dt_dt >= 0.05:
            trend = "RISING"
        elif dt_dt <= -0.15:
            trend = "FALLING_FAST"
        elif dt_dt <= -0.05:
            trend = "FALLING"
        else:
            trend = "STABLE"
            
        # 4. Mô hình dự báo nhiệt độ tương lai có xét quán tính nhiệt & yếu tố ngoại cảnh
        # Quán tính nhiệt làm giảm đà biến thiên theo hàm mũ bão hòa:
        damping_15 = 0.75
        damping_30 = 0.55
        
        # Yếu tố bức xạ mặt trời (nếu trời sáng rực qua LDR thì nhiệt độ có xu hướng tăng thêm):
        sun_contrib = 0.4 if light_now == 1 else -0.1
        
        # Yếu tố làm mát từ quạt nếu đang chạy:
        fan_contrib = -0.6 if fan_now else 0.0
        
        pred_15 = t_now + (dt_dt * 15.0 * damping_15) + (sun_contrib * 0.5) + (fan_contrib * 0.5)
        pred_30 = t_now + (dt_dt * 30.0 * damping_30) + (sun_contrib * 1.0) + (fan_contrib * 1.0)
        
        pred_15 = round(max(10.0, min(50.0, pred_15)), 1)
        pred_30 = round(max(10.0, min(50.0, pred_30)), 1)
        
        # 5. Phân loại trạng thái tiện nghi (Comfort Status)
        is_mold_risk = (h_now >= 75.0 and (t_now - dew_pt) <= 2.5)
        if is_mold_risk:
            comfort_status = "MOLD_ALERT"
        elif t_now >= 34.0 or heat_idx >= 38.0:
            comfort_status = "HEAT_DANGER"
        elif t_now > 28.5:
            comfort_status = "HOT"
        elif t_now < 19.0:
            comfort_status = "COLD"
        elif 21.0 <= t_now <= 28.0 and 40.0 <= h_now <= 70.0:
            comfort_status = "COMFORT"
        else:
            comfort_status = "MODERATE"
            
        # 6. Thuật toán Ra quyết định Tối ưu Đón đầu (Pre-emptive Optimization Engine)
        now_sec = time.time()
        time_since_change = now_sec - self.last_relay_change_time
        can_change_relay = (time_since_change >= self.min_cycle_seconds)
        
        target_fan = fan_now
        target_heat = heat_now
        mode = "NORMAL"
        reason = "Môi trường ổn định, duy trì trạng thái hiện tại."
        
        if is_mold_risk:
            mode = "MOLD_VENTILATION"
            target_fan = True
            target_heat = False
            reason = f"Cảnh báo nồm ẩm! RH={h_now}%, điểm sương {dew_pt}°C sát nhiệt độ phòng. Bật quạt thông gió đẩy ẩm."
            
        elif t_now >= 33.0 or heat_idx >= 36.0:
            mode = "EMERGENCY_COOLING"
            target_fan = True
            target_heat = False
            reason = f"Quá nhiệt khẩn cấp (T={t_now}°C, HI={heat_idx}°C)! Bật quạt công suất tối đa."
            
        # QUY TẮC PRE-COOLING: Dự báo nhiệt độ sẽ vượt 29.5°C trong 30p tới và đà tăng > 0
        elif (pred_30 >= 29.5 or (pred_15 >= 29.0 and dt_dt > 0.05)) and not fan_now:
            mode = "PRE_COOLING"
            target_fan = True
            target_heat = False
            reason = f"Pre-cooling: AI dự báo nhiệt độ tăng lên {pred_30}°C trong 30p tới (dT/dt=+{dt_dt}°C/min). Bật quạt đón đầu triệt tiêu đỉnh nhiệt!"
            
        # QUY TẮC EARLY-CUTOFF: Quạt đang chạy nhưng dự báo hạ nhiệt tốt < 25.5°C
        elif fan_now and pred_15 <= 25.5 and dt_dt <= 0.0:
            mode = "EARLY_CUTOFF"
            target_fan = False
            reason = f"Tắt quạt sớm: Nhiệt độ dự báo hạ xuống {pred_15}°C trong 15p tới. Tận dụng luồng khí còn lại để tiết kiệm điện."
            
        # QUY TẮC HEATING: Quá lạnh < 18°C
        elif pred_30 <= 18.0 and dt_dt <= 0.0:
            mode = "HEATING"
            target_fan = False
            target_heat = True
            reason = f"Thời tiết trở rét: Dự báo nhiệt độ hạ còn {pred_30}°C. Kích hoạt sưởi ấm."
            
        elif heat_now and t_now >= 22.0:
            target_heat = False
            reason = "Nhiệt độ đã ấm lên đạt ngưỡng lý tưởng (>=22°C). Tắt đèn sưởi."
            
        # Kiểm tra bảo vệ chống đảo trạng thái rơ-le quá nhanh
        if not can_change_relay and (target_fan != fan_now or target_heat != heat_now):
            reason += f" [Anti-rapid cycle: Chờ thêm {int(self.min_cycle_seconds - time_since_change)}s]"
            target_fan = fan_now
            target_heat = heat_now
        else:
            if target_fan != fan_now or target_heat != heat_now:
                self.last_relay_change_time = now_sec
                self.last_relay1_state = target_fan
                self.last_relay2_state = target_heat
                
        return PredictionResult(
            current_temp=t_now,
            current_hum=h_now,
            light_level=light_now,
            heat_index=heat_idx,
            dew_point=dew_pt,
            trend=trend,
            rate_of_change_temp=dt_dt,
            rate_of_change_hum=dh_dt,
            predicted_temp_15m=pred_15,
            predicted_temp_30m=pred_30,
            comfort_status=comfort_status,
            optimization_mode=mode,
            recommended_relay1_fan=target_fan,
            recommended_relay2_heat=target_heat,
            reason=reason
        )
