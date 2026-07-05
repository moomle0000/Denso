// Drive a Toyota Denso 8925-30080 radiator cooling fan controller from an
// Arduino Uno. The Denso module has a 3-pin connector on its harness;
// the MIDDLE wire is the PWM input. We drive it directly from a 5 V
// PWM-capable Uno pin (no level shifter needed).
//
// Wiring (see diagram.png / plan.md):
//   Uno D9  -> Denso Pin 2 (middle, PWM IN)
//   Uno GND -> Denso Pin 1 (left,   GND)
//   Denso Pin 3 (right, +12 V) is fed by the vehicle battery via a fuse.
//   The Uno itself is powered separately (USB or barrel jack).
//
// Fan speed = PWM duty on D9. ~490 Hz is the Arduino default for D9
// (Timer1 Fast PWM, prescaler 64) and works on most Denso revisions.
// If the fan buzzes audibly, switch Timer1 to a slower prescaler in
// setup() to land near 100 Hz.

#include <Arduino.h>

constexpr int PIN_PWM_OUT = 9;     // OC1A — Timer1 PWM
constexpr int RAMP_DELAY_MS = 20;  // sweep speed

void setup() {
  pinMode(PIN_PWM_OUT, OUTPUT);
  analogWrite(PIN_PWM_OUT, 0);     // fan off at boot
  Serial.begin(9600);
  Serial.println(F("Denso fan PWM driver ready."));
}

void loop() {
  static int duty = 0;
  static int dir  = 1;

  duty += dir;
  if (duty >= 255) { duty = 255; dir = -1; }
  if (duty <=   0) { duty =   0; dir =  1; }

  analogWrite(PIN_PWM_OUT, duty);
  Serial.print(F("fan duty = "));
  Serial.println(duty);
  delay(RAMP_DELAY_MS);
}
