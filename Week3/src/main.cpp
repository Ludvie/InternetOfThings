#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include "esp_sleep.h"

#define OLED_W 128
#define OLED_H 64
#define OLED_ADDR 0x3C
#define PIN_LED 2
#define PIN_WAKE GPIO_NUM_15
#define SLEEP_SECONDS 10

Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);
Adafruit_BMP280 bmp;

RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR float previousTemp = 0.0f;
RTC_DATA_ATTR float previousPressure = 0.0f;

struct SensorData { float temp, pressure, altitude; bool valid; };
SensorData weather = {0,0,0,false};
SemaphoreHandle_t dataMutex;

void showWakeReason() {
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  Serial.printf("\\n[RTC] Boot #%d | ", bootCount);
  if (cause == ESP_SLEEP_WAKEUP_TIMER) Serial.println("Wakeup: TIMER");
  else if (cause == ESP_SLEEP_WAKEUP_EXT0) Serial.println("Wakeup: EXT0 GPIO15");
  else Serial.println("Wakeup: POWER-ON");
  Serial.printf("[RTC MEM] Previous: %.2f C, %.2f hPa\n", previousTemp, previousPressure);
}
void drawOLED(const SensorData &d, const char *source) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0); display.println("UNIB IoT WEATHER STATION");
  display.drawLine(0,10,127,10,SSD1306_WHITE);
  display.setCursor(0,14); display.printf("Temp: %.1f C", d.temp);
  display.setCursor(0,26); display.printf("Press: %.1f hPa", d.pressure);
  display.setCursor(0,38); display.printf("Alt: %.1f m", d.altitude);
  display.drawLine(0,50,127,50,SSD1306_WHITE);
  display.setCursor(0,54); display.printf("#%d %s", bootCount, source);
  display.display();
}
void TaskUI(void *arg) {
  SensorData local;
  if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
    local = weather;
    xSemaphoreGive(dataMutex);
  }
  if (local.valid) {
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    const char *src = cause == ESP_SLEEP_WAKEUP_EXT0 ? "EXT0" :
                      cause == ESP_SLEEP_WAKEUP_TIMER ? "TIMER" : "BOOT";
    drawOLED(local, src);
    JsonDocument doc;
    doc["device_id"] = "esp32-weather-st01";
    doc["boot_count"] = bootCount;
    doc["wakeup_cause"] = src;
    doc["temp_c"] = local.temp;
    doc["pressure_hpa"] = local.pressure;
    doc["altitude_m"] = local.altitude;
    doc["prev_temp_c"] = previousTemp;
    serializeJson(doc, Serial); Serial.println();
    previousTemp = local.temp;
    previousPressure = local.pressure;
  } else Serial.println("[SENSOR] BMP280 tidak valid");
  vTaskDelete(NULL);
}
void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_WAKE, INPUT_PULLUP);
  digitalWrite(PIN_LED, HIGH);
  bootCount++;
  showWakeReason();

  dataMutex = xSemaphoreCreateMutex();
  Wire.begin(21,22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR))
    Serial.println("[OLED] Tidak ditemukan di alamat 0x3C");

  if (!bmp.begin(0x76)) {
    Serial.println("[BMP280] Tidak ditemukan di alamat 0x76");
    weather.valid = false;
  } else {
    bmp.setSampling(Adafruit_BMP280::MODE_FORCED,
                    Adafruit_BMP280::SAMPLING_X2,
                    Adafruit_BMP280::SAMPLING_X16,
                    Adafruit_BMP280::FILTER_X16,
                    Adafruit_BMP280::STANDBY_MS_500);
    bmp.takeForcedMeasurement();
    if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
      weather.temp = bmp.readTemperature();
      weather.pressure = bmp.readPressure()/100.0F;
      weather.altitude = bmp.readAltitude(1013.25F);
      weather.valid = true;
      xSemaphoreGive(dataMutex);
    }
  }

  // setup() berjalan pada Core 1; task UI/JSON dipin ke Core 0.
  xTaskCreatePinnedToCore(TaskUI, "TaskUI", 4096, NULL, 1, NULL, 0);
  delay(500); // beri task UI waktu menyelesaikan OLED dan JSON

  esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_SECONDS*1000000ULL);
  esp_sleep_enable_ext0_wakeup(PIN_WAKE, 0); // tombol LOW
  digitalWrite(PIN_LED, LOW);
  Serial.printf("[POWER] Deep Sleep %d detik\n", SLEEP_SECONDS);
  Serial.flush();
  esp_deep_sleep_start();
}
void loop() {}
