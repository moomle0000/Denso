# ESP32 → Denso 8925-30080 PWM Input Wiring Diagram

```
                                        DENSO 8925-30080
  +------------------+                  +----------------------------+
  |                  |                  |  Radiator Cooling Fan      |
  |      ESP32       |                  |  Control Module             |
  |                  |   LEVEL SHIFTER  |                            |
  |              GND ├─────┐            |  +-----------------------------+
  |                  │     │            |  |  3-pin PWM input connector |
  |                  │     │            |  |                             |
  |              3V3 ├──┐  │            |  |  Pin 1  ●═══ GND  ════════ |
  |                  │  │  │            |  |           │                 |
  |           GPIO 25├─►LV HV├─── 5V PWM ─►|  Pin 2  ●═══ PWM IN (middle)
  |                  │  │  │            |  |           │     ▲            |
  |                  │  │  │            |  |           │     │            |
  |                  │  │  │            |  |  Pin 3  ●═══ +12 V          |
  |                  │  │  │            |  +────────────┼──────────────────+
  +------------------+  │  │            |               │
                        │  │            |               │
                  [Level│  │Shifter]    |               │
                        │  │            |               │
                  GND ──┘  │            |               │
                           │            |               │
                  5V HV ───┘            |               │
                  rail                  |               │
                                         |               │
                                         |               │
   +12 V Battery ──[FUSE]──► Denso Pin 3 │               │
                                         |               │
   Chassis GND ──────────────────────────┼───────────────┘
                                         │
                                         ▼
                                  (to brushless fan)
```

## Connection Details

```
+----------------+        +-----------------+        +----------------------+
|     ESP32      |        |   LEVEL SHIFTER |        |  DENSO 8925-30080    |
|                |        |    3.3V -> 5V   |        |   3-pin connector    |
+----------------+        +-----------------+        +----------------------+
|                |        |                 |        |                      |
| GPIO 25 (PWM)  |------->| LV IN           |        |                      |
|                |        |                 |        |                      |
| GND            |--------| LV GND          |        |                      |
|                |        |                 |        |                      |
| 3V3            |------->| LV VCC          |        |                      |
|                |        |                 |        |                      |
|                |        | HV OUT (5V)     |------->| Pin 2 (MIDDLE) PWM  |
|                |        |                 |        |                      |
|                |        | HV GND          |--------| Pin 1 (LEFT) GND    |
|                |        |                 |        |                      |
|                |        | HV VCC (5V)     |        |                      |
+----------------+        +-----------------+        | Pin 3 (RIGHT) +12V  |
                                                       | (from battery via   |
                                                       |  fuse)               |
                                                       +----------------------+
```

## 3-Pin Connector on Denso Module (looking at the pins on the module)

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
        │         denso                  │
        │     8925-30080                  │
        │   Radiator Fan Module          │
        │                                │
        └────────────────────────────────┘
```

## Signal Flow

```
+----------------+      +-----------------+      +------------------+
|   ESP32        |      |  LEVEL SHIFTER  |      | DENSO 8925-30080 |
|   GPIO 25      |─────►|  3.3V PWM       |─────►| Pin 2 (middle)   |
|   (3.3V PWM)   |      |  to 5V PWM      |      | PWM input        |
+----------------+      +-----------------+      +------------------+
                                                        │
                                                        ▼
                                                 (Fan speed control)
```

## Notes

- Pin 2 (middle) receives 5V PWM from the level shifter output
- The level shifter converts 3.3V logic from ESP32 to 5V logic for the Denso module
- ESP32 GND must connect to Denso GND (Pin 1, left) for common reference
- The 5V supply for the level shifter HV side must come from a regulated 5V source
- Pin 3 (right) connects to +12V from the battery (with fuse protection)

## Optional: Transistor Level Shifter Circuit

If using a simple NPN transistor instead of a level shifter module:

```
ESP32                    Denso 8925-30080
GPIO 25 ──┐             Pin 2 (middle)
          │             │
          ├── 1kΩ ──┐   │
          │         │   │
          │      ┌──┴───┴──┐
          │      │  NPN    │
          │      │ (2N2222)│
          │      │         │
          │      │ B   C   E
          │      │ │   │   │
          │      │ │   │   └── GND ── Denso Pin 1
          │      │ │   │
          │      └─┤   │
          │        │   │
          │        │   ├── 10kΩ pull-up
          │        │   │
          │        │   └── +5V (from regulator)
          │        │
          └────────┘
```

The transistor inverts the signal. Account for this in software (e.g., set
duty = 255 - desired_duty).
