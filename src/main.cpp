#include <Arduino.h>

constexpr uint8_t  PIN_PWM_OUT     = 9;
constexpr uint32_t PWM_FREQ_HZ     = 3921;
constexpr uint16_t PWM_DUTY_MAX    = 1023;
constexpr uint32_t SERIAL_BAUD     = 9600;
constexpr uint16_t TIMER_PRESCALER = 1;

static const uint16_t TIMER_ICR1 =
    (uint16_t)((F_CPU / (TIMER_PRESCALER * PWM_FREQ_HZ)) - 1UL);

static uint16_t fanDuty = 0;

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

static void printHelp() {
  Serial.println();
  Serial.println(F("Toyota 89257-30080 fan PWM driver (3.9 kHz)"));
  Serial.println(F("---------------------------------------------"));
  Serial.println(F("Type 0..1023   -> raw 10-bit fan duty"));
  Serial.println(F("Type 0..100%   -> fan duty as percent"));
  Serial.println(F("Type 'off'     -> fan off (0% speed)"));
  Serial.println(F("Type 'on'      -> fan full (100% speed)"));
  Serial.println(F("Type 'read'    -> show current duty"));
  Serial.println(F("Type 'help'    -> show this message"));
  Serial.println();
  Serial.println(F("Wiring: Uno D9 -> 1k -> 2N2222 base;"));
  Serial.println(F("        2N2222 collector -> Denso Pin 2"));
  Serial.println(F("        2N2222 emitter   -> GND"));
  Serial.println();
}

static void handleSerial() {
  if (!Serial.available()) return;

  String s = Serial.readStringUntil('\n');
  s.trim();
  if (s.length() == 0) return;

  if (s.equalsIgnoreCase("off")) {
    setFanDuty(0);
    Serial.println(F("fan OFF (0%)"));
    return;
  }
  if (s.equalsIgnoreCase("on")) {
    setFanDuty(PWM_DUTY_MAX);
    Serial.println(F("fan FULL (100%)"));
    return;
  }
  if (s.equalsIgnoreCase("read")) {
    Serial.print(F("duty = "));
    Serial.print(fanDuty);
    Serial.print(F(" / "));
    Serial.print(PWM_DUTY_MAX);
    Serial.print(F(" ("));
    Serial.print((100UL * fanDuty) / (PWM_DUTY_MAX + 1));
    Serial.println(F("%)"));
    return;
  }
  if (s.equalsIgnoreCase("help") || s == "?") {
    printHelp();
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
  Serial.print(F("duty = "));
  Serial.print(duty);
  Serial.print(F(" ("));
  Serial.print((100UL * duty) / (PWM_DUTY_MAX + 1));
  Serial.println(F("%)"));
}

void setup() {
  setupTimer1Pwm();
  setFanDuty(0);
  Serial.begin(SERIAL_BAUD);
  printHelp();
}

void loop() {
  handleSerial();
}
