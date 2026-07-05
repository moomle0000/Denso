# Arduino Uno → Toyota 89257-30080 PWM Input Wiring Diagram

The Toyota part number is **89257-30080** (Part Name Code `89257` + suffix
`30080`). The "denso" label on the actual module is the manufacturer's
brand — it is a Genuine Toyota OEM part.

> ⚠️ **Never connect a 5 V Arduino GPIO directly to the PWM input.** The
> module has an internal 12 V pull-up. Use a **2N2222 NPN** (or PC817
> optocoupler) as an open-collector driver.

## Required Hardware

| Part                   | Value     | Where it goes                            |
| ---------------------- | --------- | ---------------------------------------- |
| 2N2222 (NPN)           | —         | Collector → Pin 2; Emitter → GND         |
| Resistor               | 1 kΩ      | Uno D9 → Base                            |
| Fuse                   | 30 A      | Battery + → Denso Pin 3                  |

## Wiring

```
                  +1kΩ
   Arduino D9 ──[████]──┐
                       B╲
                       ╲ E  (2N2222, NPN)
                        ╲
                         C ─────► Toyota 89257-30080
                                      Pin 2 (middle, PWM IN)
   Arduino GND ────────────────────►  Pin 1 (left, GND)

   (Battery + 12 V) ──[30A FUSE]──►  Pin 3 (right, +12 V)
```

The 2N2222 inverts the Arduino's logic:
- Arduino HIGH (5 V) → transistor ON → line pulled LOW (0 V)
- Arduino LOW (0 V) → transistor OFF → line pulled HIGH (12 V via module's internal pull-up)

## 3-Pin Connector Pinout (looking at the pins on the module)

```
         Pin 1         Pin 2         Pin 3
        (LEFT)       (MIDDLE)       (RIGHT)
           │             │             │
           │             │             │
           ●             ●             ●
           │             │             │
           │             │             │
        ┌──┴─────────────┴─────────────┴──┐
        │                                │
        │  denso  (OEM for Toyota)       │
        │  89257-30080                   │
        │  Cooling Fan Computer          │
        │  16.5 × 11.6 × 3.2 in          │
        │  2.40 lb                       │
        │                                │
        └────────────────────────────────┘
```

## High-Current Pins (separate 2-pin connector on the module)

```
        ┌──┴────────┴──┐
        │              │
   M+ ●─►│  Brushless  │◄─● M−
        │  Fan Motor   │
        │              │
        └──────────────┘
```

These go **directly** to the brushless radiator fan. The Uno does not
see this current. Wire them with 12 AWG or similar automotive gauge.

## Why 3.9 kHz and Inverse Logic

According to automotive reverse-engineering data:

- **3,921 Hz** is the canonical PWM frequency for this module.
- The module uses **inverse logic**:
  - 0% duty at the input → 100% fan speed (fail-safe)
  - 100% duty at the input → 0% fan speed (off)
  - 10–90% maps linearly
- **Disconnected wire → 100% fan** (engine won't overheat).

Because the 2N2222 inverts the Arduino's signal level, the inversions
cancel out: typing `50%` in the Serial Monitor gives 50% fan speed.

## Optional: PC817 Optocoupler (galvanic isolation)

For full isolation between the Uno and the vehicle's 12 V system, use a
PC817 instead of a 2N2222:

```
   Arduino D9 ──[220Ω]──┤> PC817     Collector ──► Toyota Pin 2
                       ├─       ├─
                       └─       └─ Emitter ──► GND
```

The PC817 gives true galvanic isolation — no common ground required
between the Uno and the vehicle.

## Why a Level Shifter Is *Not* Enough

A 3.3 V → 5 V level shifter is **not sufficient** here. The issue is
the module's internal 12 V pull-up, not voltage translation. You need
an **open-collector / open-drain** switch to safely pull the line
LOW without letting 12 V back into your microcontroller.
