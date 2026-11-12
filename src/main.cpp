#include <Arduino.h>
#include <DHT.h>
#include <ArduinoJson.h>

// Ubah menjadi 0 untuk eksperimen BLOCKING (delay).
// Ubah menjadi 1 untuk eksperimen NON-BLOCKING (millis).
#define MODE_NON_BLOCKING 0

#define PIN_DHT 4
#define TIPE_DHT DHT22
#define PIN_LED_ALERT 2
#define PIN_TOMBOL_DARURAT 15

const unsigned long INTERVAL_TELEMETRI_MS = 2000UL;
const float AMBANG_SUHU_C = 32.0f;

DHT dht(PIN_DHT, TIPE_DHT);

unsigned long waktuBacaTerakhir = 0;
unsigned long nomorPaket = 0;
bool statusTombolSebelumnya = HIGH;

void kirimTelemetriJson(float suhu, float kelembapan, bool alertSuhu) {
  JsonDocument doc;

  // Buffer menjaga angka tetap menjadi nilai numerik JSON dengan 2 desimal.
  char suhuJson[16];
  char kelembapanJson[16];
  snprintf(suhuJson, sizeof(suhuJson), "%.2f", suhu);
  snprintf(kelembapanJson, sizeof(kelembapanJson), "%.2f", kelembapan);

  doc["device_id"] = "esp32-unib-node01";
  doc["seq"] = ++nomorPaket;
  doc["uptime_ms"] = millis();
  doc["mode"] = MODE_NON_BLOCKING ? "non-blocking" : "blocking";
  doc["temp_c"] = serialized(suhuJson);
  doc["hum_pct"] = serialized(kelembapanJson);
  doc["alert"] = alertSuhu;

  serializeJson(doc, Serial);
  Serial.println();
}

void bacaDanKirimSensor() {
  float kelembapan = dht.readHumidity();
  float suhu = dht.readTemperature();

  if (isnan(kelembapan) || isnan(suhu)) {
    Serial.println("{\"error\":\"DHT22_read_failed\"}");
    return;
  }

  bool alertSuhu = (suhu >= AMBANG_SUHU_C);
  digitalWrite(PIN_LED_ALERT, alertSuhu ? HIGH : LOW);

  kirimTelemetriJson(suhu, kelembapan, alertSuhu);
}

void cekTombolDarurat() {
  bool statusTombol = digitalRead(PIN_TOMBOL_DARURAT);

  // Deteksi transisi HIGH ke LOW (tombol ditekan).
  if (statusTombol == LOW && statusTombolSebelumnya == HIGH) {
    Serial.printf(
      "{\"event\":\"emergency_button\",\"pressed_at_ms\":%lu}\n",
      millis()
    );

    // Indikator LED juga dapat diuji langsung melalui tombol.
    digitalWrite(PIN_LED_ALERT, !digitalRead(PIN_LED_ALERT));

    // Debounce sederhana sesuai eksperimen pada modul.
    delay(50);
  }

  statusTombolSebelumnya = statusTombol;
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_LED_ALERT, OUTPUT);
  pinMode(PIN_TOMBOL_DARURAT, INPUT_PULLUP);

  digitalWrite(PIN_LED_ALERT, LOW);
  dht.begin();

  Serial.println("{\"event\":\"boot\",\"msg\":\"Simpul IoT UNIB Aktif\"}");
}

void loop() {
  // Input tombol diperiksa pada setiap iterasi loop.
  cekTombolDarurat();

#if MODE_NON_BLOCKING == 0
  // MODE BLOCKING:
  // Mikrokontroler menunggu 2 detik dan tidak memeriksa tombol selama jeda.
  delay(INTERVAL_TELEMETRI_MS);
  bacaDanKirimSensor();
#else
  // MODE NON-BLOCKING:
  // millis() mengecek interval tanpa menghentikan loop.
  unsigned long sekarang = millis();

  if (sekarang - waktuBacaTerakhir >= INTERVAL_TELEMETRI_MS) {
    waktuBacaTerakhir = sekarang;
    bacaDanKirimSensor();
  }
#endif
}
