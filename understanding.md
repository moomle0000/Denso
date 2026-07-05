# Understanding the Denso 89257-30080 PWM Input

Notes from a forum / photo analysis of how the module is wired and driven
in real aftermarket installations. This explains the wiring and logic so
the schematic in `plan.md` and `diagram.png` makes sense.

---

## 1. The 3-Pin Connector (signal)

Looking at the harness side, the large 3-pin connector is the **signal /
power** connector, **not** the motor connector.

| Pin (left → right)        | Wire (typical)         | Function                                |
| ------------------------- | ---------------------- | --------------------------------------- |
| 1 (left)                  | **Black, thick**       | Battery **GND** (chassis ground)        |
| 2 (middle)                | **Blue / yellow, thin**| **PWM control input**                   |
| 3 (right)                 | **Red, thick**         | **+12 V battery** (via fuse)            |

The middle thin wire is the **PWM input**. The module drives the fan
motor from its own +12 V supply; you only feed it a small control signal.

## 2. The Two Small 2-Pin Connectors (motor)

These are **directly on the radiator fan motor leads** (M+ / M−).
The module's internal MOSFETs switch the high-current fan current.
**You do not connect anything here from the ESP32 / Uno / driver.**
Routing motor current through your microcontroller will destroy it.

## 3. Inverted PWM Logic (Important)

The module is **fail-safe** by design:

| What you send           | What the module sees     | Fan behavior         |
| ----------------------- | ------------------------ | -------------------- |
| 100% HIGH               | signal held HIGH         | **Fan OFF**          |
| 75% HIGH                | signal HIGH most of time | Slow                 |
| 50% HIGH                | half-and-half            | Medium               |
| 25% HIGH                | signal LOW most of time  | Fast                 |
| 0% (signal held LOW)    | signal held LOW          | **Full speed**       |

So the duty-cycle meaning is **inverted** at the module's input:

```
requestedSpeed = 0;     // user wants fan off
outputDuty = 255;       // ...so drive the line HIGH

requestedSpeed = 255;   // user wants full speed
outputDuty = 0;         // ...so drive the line LOW
```

This matches the internal pull-up the module has on the PWM pin: a
"disconnected" line is pulled HIGH, which the module treats as "fan off."

## 4. PWM Frequency

The **3.9 kHz** figure shown in many photos is the **default auxiliary
PWM frequency on aftermarket ECUs** (Link, Haltech, Speeduino,
Megasquirt). It is *not* a hard requirement of the Denso module itself.

Reverse-engineering reports and hobbyist experiments show the module
works over a wide range because it **filters the PWM internally**:

| Frequency | Works?          |
| --------- | --------------- |
| 50 Hz     | borderline      |
| 100 Hz    | yes             |
| 250 Hz    | yes             |
| 500 Hz    | yes             |
| 1 kHz     | yes             |
| **3.9 kHz** | yes (matches aftermarket ECU) |

In our `src/main.cpp` we use **3,921 Hz** (Timer 1, prescaler 1, ICR1 =
4080) to match the canonical ECU frequency. If you hear buzzing, drop
to 100 Hz by changing `PWM_FREQ_HZ` to 100 and `TIMER_PRESCALER` to 64.

## 5. Why the Photo Has No Transistor

In the photo, the yellow PWM wire goes directly from the Arduino / Nano
to the middle pin of the 3-pin connector — **no MOSFET, no 2N2222, no
optocoupler** visible.

Two possible explanations:

1. **That particular Arduino is being used as if it were an ECU.** Many
   aftermarket ECU boards expose open-drain / open-collector outputs
   specifically for this kind of load. An Arduino Nano's GPIO is *not*
   open-drain by default, so this is risky.
2. **The interface electronics are elsewhere** and just not visible in
   the photo (e.g. inline module, on a separate board, etc.).

Without measuring the actual voltage on the middle pin with an
oscilloscope, you cannot tell which is the case.

## 6. Why We Still Use a 2N2222 / 2N7002

Because the module has an **internal pull-up to 5–12 V** and the
ESP32 / Uno only outputs 3.3 V / 5 V push-pull, connecting them directly
risks:

- **Back-feeding current** through the ESP32 / Uno's I/O clamping
  diodes into the 5 V rail, damaging the MCU over time.
- **Unreliable logic levels** when the pull-up fights a 3.3 V high.

A 2N2222 NPN (or 2N7002 N-MOSFET) wired as an **open-drain** driver
solves both problems at almost no cost:

```
ESP32 / Uno GPIO ──[1 kΩ]── Base
                         │
                       ┌─┤
                       │ │  2N2222
                       │ │  (NPN)
                       └─┤
                         │──► Denso Pin 2 (middle)
                         │
   (Denso internal pull-up to +12 V)
```

- GPIO HIGH → transistor ON → line pulled LOW → module sees LOW duty
- GPIO LOW → transistor OFF → line pulled HIGH (12 V) by Denso pull-up
  → module sees HIGH duty

The **open-drain inversion** and the **module's own inverse logic**
cancel out, so your code can stay intuitive: `setFan(50%)` → 50% fan.

## 7. Experimental Verification (Recommended)

Since you have the actual module on your bench, the safest way to be
sure is to measure it directly before committing to a wiring plan:

1. **Measure the open-circuit voltage** on the middle pin with the
   module powered and nothing connected. Expected: ~12 V (from the
   internal pull-up).
2. **Connect a 3.3 V or 5 V PWM source** (ESP32 or Uno) directly to the
   middle pin, **through a series resistor (~1 kΩ)** as a safety
   measure, and watch the fan with an oscilloscope on the wire.
3. **Sweep duty 0 % → 100 %** and observe:
   - Is the relationship inverted (0 % = full speed)?
   - Does the line ever go above 5 V? (If yes, you definitely need a
     open-drain driver.)
   - What is the lowest frequency that works reliably?
4. If at any point the line voltage on the MCU side exceeds 3.3 V
   (ESP32) or the current draw from the GPIO is more than a few mA,
   switch to a 2N2222 / 2N7002 open-drain interface.

This gives you a wiring and firmware setup **confirmed for your exact
module** rather than inferred from generic reverse-engineering notes.

## 8. Summary

| Item                          | Value / Behavior                              |
| ----------------------------- | --------------------------------------------- |
| PWM input pin (signal)        | Middle wire of the large 3-pin connector      |
| Fan motor pins                | Two small 2-pin connectors — *do not touch*   |
| Required interface            | **Open-drain** (2N2222, 2N7002, or PC817)     |
| Logic                         | **Inverted** (0% duty = 100% fan)             |
| Frequency                     | 100 Hz – 4 kHz; **3.9 kHz** is a good default |
| Fail-safe                     | Disconnected wire → fan at 100%               |
| Module +12 V supply           | Pin 3 (right), fused 30 A                     |
| Module GND                    | Pin 1 (left), chassis                         |

## 9. Files in This Repo

- `diagram.png` — current wiring diagram (2N2222 driver)
- `diagram.md` — text/ASCII wiring diagram
- `plan.md` — full project plan
- `src/main.cpp` — Arduino Uno PWM generator (3.9 kHz, inverse logic)
- `src/pwm_reader.cpp` — `pulseIn`-based PWM reader sketch
- `AGENTS.md` — repo conventions for future agents
