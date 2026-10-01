#include "display_manager.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
static bool oled_available = false;

DisplayManager::DisplayManager() {}

bool DisplayManager::begin() {
  Serial.println("[AI-02] Dang khoi tao man hinh OLED SSD1306...");
  if (display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
    oled_available = true;
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(15, 20);
    display.println("AI-02 CLIMATE");
    display.setCursor(10, 36);
    display.println("OPTIMIZER BOOTING");
    display.display();
    Serial.println("[AI-02] OLED_INIT_OK: Khoi tao OLED thanh cong!");
    return true;
  } else {
    oled_available = false;
    Serial.println("[AI-02] LOI: Khong tim thay man hinh OLED SSD1306 tai 0x3C!");
    return false;
  }
}

void DisplayManager::togglePage() {
  _currentPage = (_currentPage == 0) ? 1 : 0;
  Serial.printf("[AI-02] OLED: Chuyen sang trang %d\n", _currentPage);
}

void DisplayManager::update(const SensorData& sensor, const AIDecisionData& ai, bool r1, bool r2) {
  if (!oled_available) return;
  
  unsigned long now = millis();
  if (now - _lastRefreshMs < OLED_REFRESH_INTERVAL_MS && _lastRefreshMs != 0) {
    return;
  }
  _lastRefreshMs = now;
  
  display.clearDisplay();
  
  if (_currentPage == 0) {
    renderPageOverview(sensor, ai, r1, r2);
  } else {
    renderPagePrediction(sensor, ai, r1, r2);
  }
  
  display.display();
}

void DisplayManager::renderPageOverview(const SensorData& s, const AIDecisionData& ai, bool r1, bool r2) {
  // Dong 1: Thanh tieu de (Header)
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(s.rtc_time_str);
  
  display.setCursor(75, 0);
  if (ai.is_ai_online) {
    display.print("[AI-ON]");
  } else {
    display.print("[LOCAL]");
  }
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  
  // Dong 2: Nhiet do & Do am (Text lon)
  display.setTextSize(2);
  display.setCursor(0, 15);
  display.printf("%.1fC", s.temperature);
  
  display.setCursor(70, 15);
  display.printf("%.0f%%", s.humidity);
  
  // Dong 3: Anh sang LDR & Do am tuong doi
  display.setTextSize(1);
  display.setCursor(0, 36);
  display.printf("Light: %s", (s.light_level == 1) ? "SUN 1" : "DARK 0");
  
  display.setCursor(75, 36);
  display.printf("AO:%d", s.light_analog);
  
  // Dong 4: Trang thai 2 kenh Relay
  display.drawLine(0, 48, 127, 48, SSD1306_WHITE);
  display.setCursor(0, 52);
  display.printf("FAN:%s", r1 ? "[ON]" : "OFF");
  
  display.setCursor(68, 52);
  display.printf("HEAT:%s", r2 ? "[ON]" : "OFF");
}

void DisplayManager::renderPagePrediction(const SensorData& s, const AIDecisionData& ai, bool r1, bool r2) {
  // Dong 1: Header trang du bao
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("AI PREDICTION (P2)");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  
  // Dong 2: Du bao nhiet do 30 phut toi
  display.setCursor(0, 14);
  display.printf("Pred +30m: %.1fC", ai.predicted_temp_30m);
  
  // Dong 3: Xu huong
  display.setCursor(0, 26);
  display.printf("Trend: %s", ai.trend);
  
  // Dong 4: Che do toi uu
  display.setCursor(0, 38);
  display.printf("Mode: %s", ai.optimization_mode);
  
  // Dong 5: Khuyen nghi Relay
  display.drawLine(0, 49, 127, 49, SSD1306_WHITE);
  display.setCursor(0, 53);
  display.printf("Fan:%s | Heat:%s", r1 ? "ACT" : "IDLE", r2 ? "ACT" : "IDLE");
}
