# Plan: Arduino Uno → Toyota Cooling Fan Computer

**Target module:** Toyota OEM Cooling Fan Computer (manufactured by Denso)
**Toyota Part Name Code:** `89257`
**Manufacturer Part Number:** `89257-30080`
**Brand:** Genuine Toyota (Denso OEM)
**Official name:** "Computer, Cooling Fan"
**Dimensions:** 16.5 × 11.6 × 3.2 in (42 × 29 × 8 cm)
**Weight:** 2.40 lb (1.09 kg)
**Controller:** Arduino Uno (ATmega328P, 5 V logic)
**Goal:** Drive the fan speed from the Uno by injecting a 3.9 kHz PWM
signal (via a 2N2222 open-collector driver) into the module's 3-pin
control connector (middle wire = PWM input).

---

## 1a. Product Specifications

| Field                  | Value                                                 |
| ---------------------- | ----------------------------------------------------- |
| Brand                  | Genuine Toyota                                        |
| Part Name Code         | `89257`                                               |
| Manufacturer Part No.  | `89257-30080`                                         |
| Description            | Computer, Cooling Fan                                |
| Also called            | Control Module / Controller / Module                 |
| Dimensions             | 16.5 × 11.6 × 3.2 in (42 × 29 × 8 cm)                |
| Weight                 | 2.40 lb (1.09 kg)                                     |
| Manufacturer           | Denso (OEM for Toyota)                                |
| Condition              | New                                                   |
| Fitment Type           | Direct Replacement                                    |

---

## 1. Background

The Toyota 89257-30080 is an OEM brushless-fan controller (built by Denso for
Toyota) used on several Toyota models (e.g. 4Runner, Tacoma, Tundra, GX470
with the 1GR / 2TR / 2UZ fan shrouds). It has a small 3-pin PWM input
connector (signal) and a separate high-current 2-pin connector (motor).

### PWM Behavior

- **Frequency:** ~3.9 kHz (3,921 Hz) is the canonical frequency. Some
  variants accept 100–300 Hz as well, but 3.9 kHz is what automotive
  reverse-engineering data shows works best.
- **Inverse logic (fail-safe):**
  - 0% duty at the module's input = **100% fan speed**
  - 100% duty at the module's input = **0% fan speed** (off)
  - 10–90% maps proportionally (linear)
  - **Disconnected wire → 100% fan** (the engine won't overheat if the
    signal wire breaks)

### High-power pins

| Pin     | Function                                       |
| ------- | ---------------------------------------------- |
| `+12 V` | Battery + via 30 A or 40 A fuse                |
| `GND`   | Chassis / battery −                            |
| `M+`    | Brushless fan motor lead (high current)        |
| `M−`    | Brushless fan motor lead (high current)        |

The motor is driven directly by the module from the +12 V supply; the Uno
never sees the motor current.

### Signal pin

The 3-pin connector's **middle wire** is the PWM input. The module has an
**internal 12 V pull-up resistor** on this pin.

> ⚠️ **Never connect a 5 V Arduino GPIO directly to the PWM input.**
> The 12 V pull-up can back-feed current through the Uno's I/O clamping
> diodes, damaging the microcontroller. Use an open-collector driver
> (2N2222 NPN or PC817 optocoupler) to pull the signal line safely to
> ground.

---

## 2. Components

| Qty | Part                                      | Notes                                       |
| --- | ----------------------------------------- | ------------------------------------------- |
| 1   | Arduino Uno                               | ATmega328P, 5 V logic                       |
| 1   | Toyota 89257-30080 cooling fan computer  | Already in the vehicle                      |
| 1   | 2N2222 NPN transistor (or PC817 opto)     | Open-collector driver                       |
| 1   | 1 kΩ resistor (base)                      | Limits base current to ~5 mA                |
| 1   | Inline fuse 30 A (or 40 A) on +12 V       | Protects harness                            |
| 1   | TVS diode on +12 V line (SMBJ16A)         | Automotive transient protection             |
| —   | 22 AWG automotive wire, ring terminals    | For chassis/12 V connections                |

---

## 3. Wiring

**Three active wires from the Uno + driver to the Denso module:**

| Uno side                    | Wire            | Denso 3-pin connector |
| --------------------------- | --------------- | --------------------- |
| **D9** → 1 kΩ → 2N2222 base | signal (open-drain) | Pin 2 (middle)    |
| **GND** → 2N2222 emitter    | ground          | Pin 1 (left)          |
| (n/a — module pulls 12 V)   | —               | Pin 3 (right, +12 V via fuse) |

**Transistor:**
- Emitter → GND
- Collector → Denso Pin 2 (middle)
- Base → 1 kΩ → Uno D9

The 2N2222 inverts the Arduino's 5 V output: Arduino HIGH → transistor ON
→ line pulled LOW; Arduino LOW → transistor OFF → line pulled HIGH (12 V
via the module's internal pull-up).

**High-current side:**
- Battery + → 30 A fuse → Denso +12 V (Pin 3, right)
- Chassis GND → Denso GND (Pin 1, left)
- Brushless fan → Denso M+ / M− (separate 2-pin connector)

The Uno itself is powered independently (USB or barrel jack) — the 12 V
rail is **not** fed to the Uno.

See `diagram.png` for the full wiring diagram.

---

## 4. Connection Steps

1. **Disconnect the vehicle battery** before any wiring.
2. Locate the 3-pin signal connector on the Denso module (the small one,
   *not* the big 2-pin motor connector).
3. Identify the wires with a multimeter:
   - Pin 1 ↔ chassis GND should read 0 Ω.
   - Pin 3 ↔ +12 V (ignition on) should read battery voltage.
   - Pin 2 (middle) is the PWM input.
4. Tap into the middle wire. A Scotch-lock / T-tap is fine for prototyping.
5. Build the 2N2222 driver:
   - Emitter → Uno GND
   - Base → 1 kΩ → Uno D9
   - Collector → Denso Pin 2 (middle)
6. Wire +12 V (fused 30 A) → Denso Pin 3.
7. Wire chassis GND → Denso Pin 1.
8. Connect the brushless fan to the module's M+ / M− (separate connector).
9. Power the Uno separately (USB for bench, regulated 5 V for in-vehicle).
10. Reconnect the vehicle battery. Flash the Uno. Type a duty in the
    Serial Monitor — the fan should respond.

---

## 5. Arduino Sketch (PlatformIO / Arduino)

`src/main.cpp` in this repo contains a working sketch.

### Key technical choices

| Parameter           | Value         | Why                                      |
| ------------------- | ------------- | ---------------------------------------- |
| PWM pin             | **D9**        | OC1A — Timer 1 (16-bit)                  |
| Timer mode          | **14** (Fast PWM, ICR1 as TOP) | Variable frequency, full duty range |
| Prescaler           | **1**         | Best resolution at 3.9 kHz               |
| PWM frequency       | **3,921 Hz**  | IC1 = 4080 → f = 16 MHz / 4081           |
| Resolution (user)   | **10-bit (0..1023)** | Maps to 0..4080 OCR1A counts    |
| Serial baud         | **9600**      | Default                                   |

### Timer1 configuration

```cpp
TCCR1A = _BV(COM1A1) | _BV(WGM11);              // clear OC1A on compare, mode 14
TCCR1B = _BV(WGM13)  | _BV(WGM12) | _BV(CS10);  // mode 14, prescaler 1
ICR1   = 4080;                                  // 16 MHz / 4081 ≈ 3,921 Hz
```

### Why Timer 1?

The Arduino Uno has three timers:
- **Timer 0** (D5, D6) — drives `millis()` / `delay()`. Don't touch.
- **Timer 1** (D9, D10) — 16-bit, fully programmable. **We use this.**
- **Timer 2** (D11, D3) — 8-bit, can be used for other PWM.

Timer 1's mode 14 (Fast PWM, ICR1 as TOP) gives independent control of
frequency (via ICR1) and duty (via OCR1A). The default Arduino pins 9
and 10 use Timer 1 in phase-correct mode at ~490 Hz; we override that
on D9 only — D10 will be left in its default state unless changed.

### Off-by-one in Fast PWM

In mode 14 with ICR1 as TOP, the timer counts `0..ICR1` (ICR1+1 cycles
per period). The output is HIGH for `OCR1A` of those cycles and LOW for
the rest. So:
- `OCR1A = 0` → 0% duty (true zero)
- `OCR1A = ICR1` → `ICR1/(ICR1+1)` ≈ 99.98% duty (one tick short of 100%)

This tiny non-linearity at the high end is negligible for fan control.

### Inverse logic + open-collector = user-friendly API

Because the module uses **inverse logic** (0% = full, 100% = off) and the
2N2222 inverts the Arduino's logic level, the inversions cancel out and
the user-facing API stays intuitive:

| User input  | Arduino duty | Module sees | Fan speed |
| ----------- | ------------ | ----------- | --------- |
| `0` / `off` | 100%         | 100% duty   | **0%** (off) |
| `512`       | ~50%         | ~50% duty   | **~50%** |
| `1023` / `on` | 0%        | 0% duty     | **100%** (full) |

The code handles the inversion in `setFanDuty()`:
```cpp
uint16_t arduinoDuty = PWM_DUTY_MAX - userDuty;
```

### Serial commands (9600 baud)

| Type        | Action                                  |
| ----------- | --------------------------------------- |
| `512`       | Set raw 10-bit duty (0..1023)           |
| `50%`       | Set duty as percent (0..100)            |
| `off`       | Fan off (0% speed)                      |
| `on`        | Fan full (100% speed)                   |
| `read`      | Print current duty                      |
| `help` / `?`| Show command list                       |

Build and flash:
```
pio run -t upload
```

---

## 6. Verification Checklist

- [ ] Fan is **off** when Uno output is "off" / 0%.
- [ ] Fan reaches full speed at "on" / 100% duty.
- [ ] No flickering or audible buzzing at 3.9 kHz.
- [ ] Pull the Uno's USB / power: fan should ramp to **100%** (fail-safe).
- [ ] Disconnect the signal wire (collector floating): fan should go to
      100% (fail-safe).
- [ ] Engine bay temperature unchanged from OEM behavior under load.
- [ ] No fault codes on the vehicle when the harness is reconnected.
- [ ] Common GND verified: Denso module ground pin, 2N2222 emitter, and
      Uno GND all read < 0.1 V between them at all times.

---

## 7. Safety / Important Notes

- The Toyota 89257-30080 has its **own** thermal and over-current
  protection. Do not bypass it. The Uno is only *requesting* a fan speed;
  the module decides what to actually deliver.
- The 2N2222 driver **isolates** the Uno's 5 V GPIO from the module's
  internal 12 V pull-up. Without it, you risk burning out the Uno.
- If the vehicle's ECU also drives the same PWM line, the two outputs
  will fight. Disconnect the OEM feed and let the Uno be the sole source
  (or use a diode-OR with two 1N5817 schottky diodes).
- Automotive 12 V is a noisy environment. Add a TVS diode across 12 V
  near the module and a fuse within 30 cm of the battery tap.
- Bench-test at low duty before connecting the fan; the brushless motor
  can spin up to several thousand RPM and is loud.
- The Uno is **not** powered from the 12 V rail — use USB, a barrel-jack
  supply, or a separate 5 V regulator.

---

## 8. Files in This Repo

- `diagram.png` — wiring diagram (current)
- `diagram.md` — text/ASCII wiring diagram
- `plan.md` — this file
- `src/main.cpp` — Arduino sketch (3.9 kHz PWM, serial-controlled duty)
- `AGENTS.md` — repo conventions for future agents
