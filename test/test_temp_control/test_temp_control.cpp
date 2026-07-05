#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

constexpr uint8_t  PIN_PWM_OUT      = 9;
constexpr uint8_t  PIN_DS18B20      = 2;
constexpr uint32_t PWM_FREQ_HZ      = 3921;
constexpr uint16_t PWM_DUTY_MAX     = 1023;
constexpr uint32_t SERIAL_BAUD      = 9600;
constexpr uint16_t TIMER_PRESCALER  = 1;

constexpr float    TEMP_START_C     = 40.0;
constexpr float    TEMP_FULL_C      = 80.0;
constexpr float    TEMP_HYSTER_C    = 2.0;
constexpr uint32_t SENSOR_PERIOD_MS = 1000;

static const uint16_t TIMER_ICR1 =
    (uint16_t)((F_CPU / (TIMER_PRESCALER * PWM_FREQ_HZ)) - 1UL);

OneWire             oneWire(PIN_DS18B20);
DallasTemperature   sensors(&oneWire);

static uint16_t fanDuty        = 0;
static float    lastTempC      = -127.0;
static bool     sensorOk       = false;

static inline void setFanDuty(uint16_t userDuty) {
  if (userDuty > PWM_DUTY_MAX) userDuty = PWM_DUTY_MAX;
  uint16_t arduinoDuty = (uint16_t)(PWM_DUTY_MAX - userDuty);
  OCR1A = (uint16_t)(((uint32_t)arduinoDuty * (TIMER_ICR1 + 1UL)) /
                     (PWM_DUTY_MAX + 1UL));
  fanDuty = userDuty;
}

static inline void setupTimer1Pwm() {
  digitalWrite(PIN_PWM_OUT, HIGH);
  pinMode(PIN_PWM_OUT, OUTPUT);
  TCCR1A = _BV(COM1A1) | _BV(WGM11);
  TCCR1B = _BV(WGM13)  | _BV(WGM12) | _BV(CS10);
  ICR1   = TIMER_ICR1;
  OCR1A  = TIMER_ICR1;
}

static uint16_t tempToDuty(float tC) {
  if (tC <= TEMP_START_C) return 0;
  if (tC >= TEMP_FULL_C)  return PWM_DUTY_MAX;
  float frac = (tC - TEMP_START_C) / (TEMP_FULL_C - TEMP_START_C);
  return (uint16_t)(frac * (PWM_DUTY_MAX + 1));
}

static void printHelp() {
  Serial.println();
  Serial.println(F("Denso 89257-30080 fan driver + DS18B20 (3.9 kHz)"));
  Serial.println(F("-------------------------------------------------"));
  Serial.println(F("Auto:  fan follows temperature"));
  Serial.println(F("       0%   <= TEMP_START  (default 40 C)"));
  Serial.println(F("       100% >= TEMP_FULL   (default 80 C)"));
  Serial.println(F("Manual commands:"));
  Serial.println(F("  0..1023   set raw fan duty"));
  Serial.println(F("  0..100%   set fan duty as percent"));
  Serial.println(F("  off       fan off (0%)"));
  Serial.println(F("  on        fan full (100%)"));
  Serial.println(F("  auto      resume temperature control"));
  Serial.println(F("  read      show current duty + temperature"));
  Serial.println(F("  help      show this message"));
  Serial.println();
  Serial.println(F("Wiring: D9 -> 1k -> 2N2222 base;"));
  Serial.println(F("        collector -> Denso Pin 2; emitter -> GND"));
  Serial.println(F("        D2 <- DS18B20 DQ (with 4.7k pull-up to 5V)"));
  Serial.println();
}

static void handleSerial() {
  if (!Serial.available()) return;

  String s = Serial.readStringUntil('\n');
  s.trim();
  if (s.length() == 0) return;

  if (s.equalsIgnoreCase("off"))   { setFanDuty(0);              Serial.println(F("manual OFF"));  return; }
  if (s.equalsIgnoreCase("on"))    { setFanDuty(PWM_DUTY_MAX);   Serial.println(F("manual FULL")); return; }
  if (s.equalsIgnoreCase("auto"))  { Serial.println(F("auto mode on")); return; }
  if (s.equalsIgnoreCase("help") || s == "?") { printHelp(); return; }

  if (s.equalsIgnoreCase("read")) {
    Serial.print(F("duty = "));
    Serial.print(fanDuty);
    Serial.print(F(" / "));
    Serial.print(PWM_DUTY_MAX);
    Serial.print(F(" ("));
    Serial.print((100UL * fanDuty) / (PWM_DUTY_MAX + 1));
    Serial.print(F("%)  temp = "));
    Serial.print(lastTempC, 1);
    Serial.print(F(" C  sensor = "));
    Serial.println(sensorOk ? F("ok") : F("ERROR"));
    return;
  }

  bool isPercent = s.endsWith("%");
  if (isPercent) s.remove(s.length() - 1);

  int v = s.toInt();
  if (v < 0) v = 0;

  uint16_t duty;
  if (isPercent) {
    if (v > 100) v = 100;
    duty = (uint16_t)((uint32_t)v * (PWM_DUTY_MAX + 1) / 100);
  } else {
    if (v > (int)PWM_DUTY_MAX) v = PWM_DUTY_MAX;
    duty = (uint16_t)v;
  }

  setFanDuty(duty);
  Serial.print(F("manual duty = "));
  Serial.print(duty);
  Serial.print(F(" ("));
  Serial.print((100UL * duty) / (PWM_DUTY_MAX + 1));
  Serial.println(F("%)"));
}

static void readTemperature() {
  sensors.requestTemperatures();
  float t = sensors.getTempCByIndex(0);

  if (t == -127.0 || t == 85.0) {
    sensorOk = false;
    return;
  }
  lastTempC = t;
  sensorOk  = true;
}

static void applyAutoControl() {
  if (!sensorOk) {
    setFanDuty(PWM_DUTY_MAX);
    return;
  }
  static float    smoothedC    = lastTempC;
  static bool     warmed       = false;
  const float     alpha        = 0.2;

  smoothedC = (alpha * lastTempC) + ((1.0 - alpha) * smoothedC);

  if (!warmed && smoothedC < (TEMP_START_C - TEMP_HYSTER_C)) {
    warmed = false;
  } else if (!warmed && smoothedC > (TEMP_START_C + TEMP_HYSTER_C)) {
    warmed = true;
  } else if (warmed && smoothedC < (TEMP_START_C - TEMP_HYSTER_C)) {
    warmed = false;
  }

  if (!warmed) {
    setFanDuty(0);
    return;
  }
  setFanDuty(tempToDuty(smoothedC));
}

void setup() {
  setupTimer1Pwm();
  setFanDuty(0);

  sensors.begin();
  Serial.begin(SERIAL_BAUD);
  printHelp();
}

void loop() {
  static uint32_t lastSensorMs = 0;
  static bool     autoMode     = true;
  static uint32_t lastReportMs = 0;

  uint32_t now = millis();

  if (autoMode) {
    if (now - lastSensorMs >= SENSOR_PERIOD_MS) {
      lastSensorMs = now;
      readTemperature();
      applyAutoControl();
    }
  }

  if (Serial.available()) {
    String s = Serial.readStringUntil('\n');
    s.trim();
    if (s.equalsIgnoreCase("auto")) {
      autoMode = true;
      Serial.println(F("auto mode on"));
    } else {
      autoMode = false;
      handleSerial();
    }
  }

  if (autoMode && (now - lastReportMs >= 2000)) {
    lastReportMs = now;
    Serial.print(F("auto  temp = "));
    Serial.print(lastTempC, 1);
    Serial.print(F(" C  duty = "));
    Serial.print((100UL * fanDuty) / (PWM_DUTY_MAX + 1));
    Serial.println(F("%"));
  }
}
