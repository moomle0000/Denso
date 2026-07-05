# AGENTS.md

## Project

PlatformIO project targeting **Arduino Uno** (`platform = atmelavr`, `board = uno`, `framework = arduino`). The single environment is `[env:uno]`.

## Layout

- `platformio.ini` — project config. One env: `uno`.
- `src/main.cpp` — Arduino sketch entry point (`setup()` / `loop()`). This is the only source file.
- `include/` — project headers (`#include "header.h"`).
- `lib/<name>/` — private libraries; each is a folder with its own `src/`, `library.json` optional.
- `test/` — PlatformIO Test Runner location. Currently empty (only the stock `README`); no tests exist.
- `.pio/` — build artifacts. **Already gitignored.** Do not commit.
- `.vscode/` — editor config. Recommends the `platformio.platformio-ide` extension and explicitly **excludes** `ms-vscode.cpptools-extension-pack` (intellisense is provided by the PlatformIO extension).

## Commands

Use the PlatformIO CLI (or the VS Code PlatformIO extension). Common tasks:

- `pio run` — build the `uno` env (produces `.pio/build/uno/firmware.elf` + `.hex`).
- `pio run -t upload` — build and flash to the connected Uno (requires a serial port; on Windows typically `COM3` etc., auto-detected).
- `pio run -t clean` — clean build artifacts.
- `pio test` — runs `test/`. Will report "no test found" until tests are added.
- `pio device monitor -b 9600` — open the serial monitor at the baud rate the sketch uses.

No custom targets, scripts, or extra envs are defined; `platformio.ini` is the minimal 3-line default.

## Conventions

- Code style: standard Arduino C++ (`.cpp` source, `.h` headers in `include/`). No formatter, linter, or pre-commit hooks are configured — do not invent one without asking.
- No CI workflows are present (`.github/workflows/` does not exist).
- No external library dependencies are declared. If you add one, use `lib_deps = ...` in `platformio.ini`, not vendoring.
- `Serial.begin(9600)` is called inside `loop()` in the starter `main.cpp`; this is a bug in the boilerplate (will reset the baud rate every iteration). If touching that file, fix it by moving `Serial.begin` into `setup()`.

## Constraints

- No existing tests, fixtures, or harness — adding `pio test` support means creating `test/test_<name>/` with a Unity/PlatformIO test file.
- There is no release / branch / PR convention documented in the repo.
