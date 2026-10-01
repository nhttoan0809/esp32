#include "sensors_manager.h"
#include <DHT.h>
#include <Wire.h>
#include <RTClib.h>

static DHT dht(PIN_DHT11, DHT_TYPE);

#if defined(WOKWI_SIMULATION)
static RTC_DS1307 rtc;
#else
static RTC_DS3231 rtc;
#endif

SensorsManager::SensorsManager() {}

bool SensorsManager::begin() {
  Serial.println("[AI-02] Dang khoi tao cac cam bien...");
  
  // 1. Khoi tao DHT11 / DHT22
  dht.begin();
  
  // 2. Khoi tao chan LDR
  pinMode(PIN_LDR_DO, INPUT);
  pinMode(PIN_LDR_AO, INPUT);
  
  // 3. Khoi tao RTC qua bus I2C
  // Luu y: Wire.begin() da duoc goi o setup() hoac goi tai day voi chan PIN_I2C_SDA, PIN_I2C_SCL
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  
  if (rtc.begin()) {
    _data.rtc_valid = true;
    Serial.println("[AI-02] RTC_INIT_OK: Da ket noi module RTC");
    
    #if defined(WOKWI_SIMULATION)
    if (!rtc.isrunning()) {
      Serial.println("[AI-02] RTC chua chay, dang dong bo gio he thong...");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    #else
    if (rtc.lostPower()) {
      Serial.println("[AI-02] RTC bi mat nguon, dang dong bo gio he thong...");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    #endif
  } else {
    _data.rtc_valid = false;
    Serial.println("[AI-02] CANH BAO: Khong tim thay RTC tai dia chi 0x68. Su dung bo dem millis() du phong.");
  }
  
  // Doc thu lan dau
  readDHT();
  readLDR();
  readRTC();
  
  Serial.println("[AI-02] SENSORS_INIT_OK: Khoi tao cam bien thanh cong!");
  return true;
}

bool SensorsManager::update() {
  unsigned long now = millis();
  if (now - _lastReadMs >= SENSOR_READ_INTERVAL_MS || _lastReadMs == 0) {
    _lastReadMs = now;
    readDHT();
    readLDR();
    readRTC();
    return true;
  }
  return false;
}

void SensorsManager::readDHT() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  
  if (!isnan(t) && !isnan(h) && t > -40.0f && t < 85.0f && h >= 0.0f && h <= 100.0f) {
    _data.temperature = t;
    _data.humidity = h;
    _data.dht_valid = true;
  } else {
    _data.dht_valid = false;
    // Giu nguyen gia tri hop le truoc do de tranh gay soc du lieu
  }
}

void SensorsManager::readLDR() {
  // Logic module quang tro LM393:
  // DO = LOW khi du sang (LED tin hieu tren module sang)
  // DO = HIGH khi troi toi (LED tin hieu tat)
  int doVal = digitalRead(PIN_LDR_DO);
  _data.light_level = (doVal == LOW) ? 1 : 0;
  
  // Doc kenh analog ADC1 de co gia tri do sang lien tuc
  _data.light_analog = analogRead(PIN_LDR_AO);
}

void SensorsManager::readRTC() {
  if (_data.rtc_valid) {
    DateTime now = rtc.now();
    _data.rtc_timestamp = now.unixtime();
    snprintf(_data.rtc_time_str, sizeof(_data.rtc_time_str), "%02d:%02d:%02d",
             now.hour(), now.minute(), now.second());
  } else {
    // Gia lap gio dua tren millis()
    unsigned long sec = millis() / 1000;
    _data.rtc_timestamp = 1727700000 + sec;
    snprintf(_data.rtc_time_str, sizeof(_data.rtc_time_str), "%02lu:%02lu:%02lu",
             (sec / 3600) % 24, (sec / 60) % 60, sec % 60);
  }
}
