
#include <Arduino.h>
#include <DHT.h>
#include <ArduinoJson.h>


// 0 = Chattering (Single Threshold)
// 1 = Histeresis
#define GUNAKAN_HISTERESIS 1

// 0 = Dry-Run OFF
// 1 = Dry-Run ON
#define ENABLE_DRY_RUN 1

// ======================================
// PIN ESP32
// ======================================

#define PIN_DHT 4
#define PIN_SOIL 34
#define PIN_TOMBOL 15
#define PIN_RELAY 26
#define PIN_PWM 18

// ======================================
// PARAMETER SISTEM
// ======================================

const int MV_KERING = 2650;
const int MV_BASAH = 1100;

const float POMPA_ON = 35.0f;
const float POMPA_OFF = 60.0f;
const float AMBANG_TUNGGAL = 45.0f;

const int N_FILTER = 5;

const int PWM_FREQ = 5000;
const int PWM_RES = 8;
const int PWM_CH = 0;

const unsigned long INTERVAL_MS = 1000;
const unsigned long MAX_RUN_MS = 15000;

const float KENAIKAN_MIN = 1.0f;

// ======================================
// VARIABEL GLOBAL
// ======================================

DHT dht(PIN_DHT, DHT22);

uint32_t sampel[N_FILTER] = {0};

int idx = 0;
bool penuh = false;

unsigned long lastRead = 0;
unsigned long lastDebounce = 0;
unsigned long seq = 0;
unsigned long relaySwitches = 0;

unsigned long pumpStart = 0;

float soilAtStart = 0;

bool pumpOn = false;
bool manualMode = false;
bool buttonPrev = HIGH;
bool dryRunFault = false;

// ======================================
// PWM SETUP
// ======================================

void pwmSetup() {

#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3,0,0))

    ledcAttach(PIN_PWM, PWM_FREQ, PWM_RES);

#else

    ledcSetup(PWM_CH, PWM_FREQ, PWM_RES);
    ledcAttachPin(PIN_PWM, PWM_CH);

#endif
}

void pwmWrite(uint8_t duty) {

#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3,0,0))

    ledcWrite(PIN_PWM, duty);

#else

    ledcWrite(PWM_CH, duty);

#endif
}

// ======================================
// MOVING AVERAGE
// ======================================

uint32_t soilMvFiltered() {

    sampel[idx] = analogReadMilliVolts(PIN_SOIL);

    idx = (idx + 1) % N_FILTER;

    if (idx == 0) {
        penuh = true;
    }

    int count = penuh ? N_FILTER : idx;

    uint64_t total = 0;

    for (int i = 0; i < count; i++) {
        total += sampel[i];
    }

    return total / count;
}

// ======================================
// KONVERSI SOIL MOISTURE
// ======================================

float soilPercent(uint32_t mv) {

    float p = (float)(MV_KERING - (int)mv)
              * 100.0f / (MV_KERING - MV_BASAH);

    return constrain(p, 0.0f, 100.0f);
}

// ======================================
// KONTROL POMPA
// ======================================

void setPump(bool state) {

    if (pumpOn != state) {

        pumpOn = state;

        relaySwitches++;

        digitalWrite(PIN_RELAY, pumpOn ? HIGH : LOW);

    }
}

// ======================================
// SETUP
// ======================================

void setup() {

    Serial.begin(115200);

    pinMode(PIN_SOIL, INPUT);
    pinMode(PIN_TOMBOL, INPUT_PULLUP);

    pinMode(PIN_RELAY, OUTPUT);
    pinMode(PIN_PWM, OUTPUT);

    digitalWrite(PIN_RELAY, LOW);

    analogReadResolution(12);
    analogSetPinAttenuation(PIN_SOIL, ADC_11db);

    pwmSetup();

    dht.begin();

    Serial.println("{\"event\":\"boot\",\"lab\":\"Greenhouse Kopi\"}");

    Serial.print("MODE KONTROL: ");

#if GUNAKAN_HISTERESIS == 1
    Serial.println("HYSTERESIS");
#else
    Serial.println("SINGLE_THRESHOLD");
#endif

#if ENABLE_DRY_RUN == 1
    Serial.println("DRY_RUN: ON");
#else
    Serial.println("DRY_RUN: OFF");
#endif

}

// ======================================
// LOOP UTAMA
// ======================================

void loop() {

    unsigned long now = millis();

    // ------------------------------
    // PEMERIKSAAN TOMBOL
    // ------------------------------

    bool button = digitalRead(PIN_TOMBOL);

    if (button == LOW &&
        buttonPrev == HIGH &&
        now - lastDebounce > 60) {

        lastDebounce = now;

        manualMode = !manualMode;

        Serial.printf(
            "{\"event\":\"mode_change\",\"manual_override\":%s}\n",
            manualMode ? "true" : "false"
        );

    }

    buttonPrev = button;

    // ------------------------------
    // INDIKATOR FAULT
    // ------------------------------

    if (dryRunFault) {

        digitalWrite(
            PIN_PWM,
            ((now / 250) % 2) ? HIGH : LOW
        );

    }

    // ------------------------------
    // INTERVAL PEMBACAAN SENSOR
    // ------------------------------

    if (now - lastRead < INTERVAL_MS) {
        return;
    }

    lastRead = now;

    // ------------------------------
    // BACA SENSOR
    // ------------------------------

    int adcRaw = analogRead(PIN_SOIL);

    uint32_t mv = soilMvFiltered();

    float soil = soilPercent(mv);

    float temp = dht.readTemperature();

    float hum = dht.readHumidity();

    if (isnan(temp) || isnan(hum)) {

        Serial.println(
            "{\"error\":\"Pembacaan DHT22 gagal\"}"
        );

        return;
    }

    // ------------------------------
    // KONTROL POMPA
    // ------------------------------

    bool wasOn = pumpOn;

    if (!dryRunFault) {

        if (manualMode) {

            setPump(true);

        } else {

#if GUNAKAN_HISTERESIS == 1

            if (soil < POMPA_ON) {
                setPump(true);
            }

            else if (soil >= POMPA_OFF) {
                setPump(false);
            }

#else

            setPump(soil < AMBANG_TUNGGAL);

#endif

        }

    }

    // ------------------------------
    // CATAT WAKTU POMPA
    // ------------------------------

    if (!wasOn && pumpOn) {

        pumpStart = now;

        soilAtStart = soil;

    }

    if (wasOn && !pumpOn) {

        pumpStart = 0;

    }

    // ------------------------------
    // DRY-RUN SAFETY CUT-OFF
    // ------------------------------

#if ENABLE_DRY_RUN == 1

    if (pumpOn &&
        !dryRunFault &&
        now - pumpStart >= MAX_RUN_MS &&
        soil < soilAtStart + KENAIKAN_MIN) {

        dryRunFault = true;

        manualMode = false;

        setPump(false);

        Serial.println(
            "{\"fault\":\"DRY_RUN_PROTECTION\"}"
        );

    }

#endif

    // ------------------------------
    // KONTROL KIPAS PWM
    // ------------------------------

    int duty = map(
        (long)(temp * 10.0f),
        280,
        380,
        0,
        255
    );

    duty = constrain(duty, 0, 255);

    float fanPct = duty * 100.0f / 255.0f;

    if (!dryRunFault) {

        pwmWrite((uint8_t)duty);

    }

    // ------------------------------
    // OUTPUT JSON
    // ------------------------------

    JsonDocument doc;

    doc["device_id"] = "esp32-kopi-gh01";

    doc["seq"] = ++seq;

    doc["adc_raw"] = adcRaw;

    doc["soil_mv_avg"] = mv;

    doc["soil_pct"] = soil;

    doc["temp_c"] = temp;

    doc["air_hum_pct"] = hum;

    doc["pump_on"] = pumpOn;

    doc["relay_switches"] = relaySwitches;

    doc["fan_pwm_pct"] = fanPct;

    doc["ctrl_mode"] =
        manualMode ? "MANUAL" :
        (GUNAKAN_HISTERESIS ? "HYSTERESIS" : "SINGLE_THR");

    doc["fault"] =
        dryRunFault ? "DRY_RUN_PROTECTION" : "NONE";

    serializeJson(doc, Serial);

    Serial.println();

}
