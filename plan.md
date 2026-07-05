# Plan: Arduino Uno → Toyota Denso Radiator Cooling Fan Control Module

**Target module:** Toyota OEM Denso Radiator Cooling Fan Control Module
**Part number:** `8925-30080`
**Controller:** Arduino Uno (ATmega328P, 5 V logic)
**Goal:** Drive the fan speed from the Uno by injecting a PWM signal into the
module's 3-wire control connector (middle wire = PWM input).

---

## 1. Background

The Denso 8925-30080 is an OEM brushless-fan controller used on several Toyota
models (e.g. 4Runner, Tacoma, Tundra, GX470 with the 1GR / 2TR / 2UZ fan
shrouds). It has a small 3-pin PWM input connector. Per the user's measurement
on the actual harness:

| Pin (connector) | Wire        | Function                   |
| --------------- | ----------- | -------------------------- |
| 1 (left)        | —           | GND (0 V, chassis ground)  |
| **2 (middle)**  | **PWM in**  | **PWM control signal**     |
| 3 (right)       | —           | +12 V supply (VBAT)        |

The middle wire is the **PWM input**. The module expects a 5 V-tolerant PWM
(~100–250 Hz, duty = requested fan %), with GND common to chassis.

The **Arduino Uno** runs at 5 V logic, so its GPIO can drive the Denso PWM
input **directly** — **no level shifter required**.

---

## 2. Components

| Qty | Part                                     | Notes                                       |
| --- | ---------------------------------------- | ------------------------------------------- |
| 1   | Arduino Uno                              | ATmega328P, 5 V logic                       |
| 1   | Denso 8925-30080 fan controller          | Already in the vehicle                      |
| —   | Inline fuse (5–10 A) on 12 V to module   | Protects harness                            |
| 1   | TVS diode on 12 V input line             | SMBJ16A or similar, automotive protection   |
| —   | 22 AWG automotive wire, ring terminals   | For chassis/12 V connections                |

> ⚠️ The fan motor itself can pull 20–40 A. The Denso module switches the
> high-current path; **never** route the motor wires through the Uno.

---

## 3. Wiring

**Just three wires between the Uno and the Denso module:**

| Uno pin   | Wire     | Denso 3-pin connector | Notes                     |
| --------- | -------- | ---------------------- | ------------------------- |
| **D9**    | signal   | Pin 2 (middle)         | 5 V PWM (~490 Hz default) |
| **GND**   | ground   | Pin 1 (left)           | Common chassis ground     |
| (none)    | +12 V    | Pin 3 (right)          | From battery via fuse     |

The Uno itself is powered independently (USB or barrel jack) — the 12 V
rail is **not** fed to the Uno.

See `diagram.png` for the full wiring diagram.

---

## 4. Connection Steps

1. **Disconnect the vehicle battery** before any wiring.
2. Locate the 3-pin connector on the Denso module (the small one, *not* the
   big 2-pin high-current fan connector).
3. Identify the wires with a multimeter:
   - Pin 1 ↔ chassis GND should read 0 Ω.
   - Pin 3 ↔ +12 V (ignition on) should read battery voltage.
   - Pin 2 (middle) is your PWM input.
4. Tap into the middle wire. Do **not** cut it unless you must — a
   Scotch-lock or T-tap is fine for prototyping.
5. Wire 12 V (fused) and GND to the Denso module. The existing OEM wiring
   already powers the module — leave that intact and simply intercept the
   middle wire to feed your PWM in.
6. Connect Uno **D9** → Denso **Pin 2** (middle).
7. Connect Uno **GND** → Denso **Pin 1** (left) and to chassis GND
   (star-ground near the module).
8. Power the Uno separately (USB for bench, or a regulated 5 V supply for
   in-vehicle).
9. Reconnect the vehicle battery. With the ignition **off**, flash the Uno.
10. Set duty to 0 % — fan should be stopped. Ramp duty in 10 % steps; the
    fan should follow.

---

## 5. Arduino Sketch (PlatformIO / Arduino)

`src/main.cpp` in this repo contains a working sketch. Highlights:

```cpp
#include <Arduino.h>

constexpr int PIN_PWM_OUT   = 9;     // Timer1 PWM (~490 Hz on D9)
constexpr int PWM_FREQ_HZ   = 100;   // 100 Hz is safer for some Denso revisions;
                                      // 490 Hz (default) works on most.
constexpr int PWM_RES_BITS  = 8;     // 0..255 duty
constexpr int PWM_CHANNEL   = 0;

void setup() {
  pinMode(PIN_PWM_OUT, OUTPUT);

  // Configure Timer1 for ~100 Hz, 8-bit PWM on D9
  // (TCCR1A / TCCR1B manipulation; or use the Arduino `analogWrite` default
  // and live with 490 Hz — most Denso modules accept it fine)
  ledcSetup-style not available on Uno; use analogWrite().

  analogWrite(PIN_PWM_OUT, 0);       // fan off at boot
  Serial.begin(9600);
}

void loop() {
  static int duty = 0;
  static int dir  = 1;
  duty += dir;
  if (duty >= 255) { duty = 255; dir = -1; }
  if (duty <= 0)   { duty = 0;   dir =  1; }

  analogWrite(PIN_PWM_OUT, duty);    // 0..255 → 0..100 % duty
  Serial.print("fan duty = ");
  Serial.println(duty);
  delay(20);
}
```

The `platformio.ini` is already set up for the **Uno** environment
(`[env:uno]`), so no config changes are needed:

```ini
[env:uno]
platform = atmelavr
board = uno
framework = arduino
```

Build and flash:
```
pio run -t upload
```

### Optional: change PWM frequency to ~100 Hz

`analogWrite()` on D9 uses Timer1 with a default ~490 Hz. To go lower:

```cpp
void setup() {
  pinMode(9, OUTPUT);
  TCCR1A = _BV(COM1A1) | _BV(WGM10);   // Fast PWM, 8-bit, clear OC1A on compare
  TCCR1B = _BV(WGM12)  | _BV(CS11);    // prescaler = 8 → ~100 Hz
  OCR1A  = 0;
  Serial.begin(9600);
}
```

If the fan buzzes audibly at 490 Hz, switch to the Timer1 prescaler trick
above.

---

## 6. Verification Checklist

- [ ] Fan is **off** when Uno output is 0 %.
- [ ] Fan reaches full speed at ~90–100 % duty.
- [ ] No flickering or audible buzzing (try ~100 Hz if 490 Hz is rough).
- [ ] Engine bay temperature unchanged from OEM behavior under load
      (Denso module still does its own safety cutoffs).
- [ ] No fault codes on the vehicle when the harness is reconnected.
- [ ] Common GND verified: Denso module ground pin and Uno GND read
      < 0.1 V between them at all times.

---

## 7. Safety / Important Notes

- The Denso 8925-30080 has its **own** thermal and over-current protection.
  Do not bypass it. The Uno is only *requesting* a fan speed; the module
  decides what to actually deliver.
- Automotive 12 V is a noisy environment. Add a TVS diode across 12 V near
  the module and a fuse within 30 cm of the battery tap.
- If the vehicle's ECU also drives the same PWM line, the two outputs
  will fight. Disconnect the OEM feed and let the Uno be the sole source
  (or use a diode-OR with two 1N5817 schottky diodes).
- Bench-test at low duty before connecting the fan; the brushless motor
  can spin up to several thousand RPM and is loud.
- The Uno is **not** powered from the 12 V rail — use USB, a barrel-jack
  supply, or a separate 5 V regulator.

---

## 8. Files in This Repo

- `diagram.png` — wiring diagram (current)
- `diagram.md` — text/ASCII wiring diagram
- `plan.md` — this file
- `src/main.cpp` — Arduino sketch (fan-speed sweep demo)
- `AGENTS.md` — repo conventions for future agents
