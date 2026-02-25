# Autopilot Implementation

Context and change log for the autopilot feature implementation.

## Overview

The ESP32 will receive data from the flight simulator and, via a button, switch to Autopilot (AP) mode. In AP mode, the device controls pitch and roll of the cyclic itself (instead of forwarding sensor data) to control the helicopter.

## State Management

### Design Approach

State is centralized in a composed structure (`AppState`) with four logical groups:

| Group | Purpose |
|-------|---------|
| **Autopilot** | Mode configuration (on/off, horizontal/vertical modes, selected values) |
| **Simulator** | Current values received from simulator (speed, altitude, pitch, roll, heading, vertical speed) |
| **Sensors** | Raw and calibrated readings from physical inputs (cyclic X/Y, collective) |
| **Joystick** | Current output values sent to PC (may differ from sensors when AP is active) |

**Rationale for composition:**
- Clear separation of concerns
- Easy to pass subsets to functions (e.g. AP logic gets `simulator`, `sensors`, `autopilot`)
- Straightforward JSON serialization for web API (each group → object)
- Maintainable as requirements grow

### Validity & Staleness

- **Simulator data**: `valid` flag + `lastUpdateMs` – know when data is stale
- **Sensors**: `cyclicValid` for cyclic serial data (existing pattern)

### Autopilot Modes

**Horizontal:**
- `Off` – Manual, pass-through
- `RollHold` – Hold current roll
- `HeadingHold` – Hold `selectedHeading` (with `hasSelectedHeading`)

**Vertical:**
- `Off` – Manual
- `PitchHold` – Hold current pitch (inner loop: pitch PID)
- `VerticalSpeed` – Hold `selectedVerticalSpeed` (outer loop: VS error → target pitch → pitch PID)
- `AltitudeHold` – Hold `selectedAltitude` (future)

### Selected Values

For hold modes that need a target: `selectedHeading`, `selectedAltitude`, `selectedVerticalSpeed` with corresponding `hasSelected*` flags when the value is armed/active.

### Tuning Parameters

- **Pitch/Roll PID**: `pitchKp`, `pitchKi`, `pitchKd`, `rollKp`, `rollKi`, `rollKd` – inner loop gains
- **Heading (outer loop)**: `headingKp` – bank angle per degree of heading error (tunable via web)
- **Vertical Speed (outer loop)**: `vsKp` – pitch angle per fpm of VS error (tunable via web)

### Data Representation

- **Axis values**: 0–10000 (joystick range, center 5000) – matches existing `joystick.h`
- **Raw sensors**: 0–4095 (AS5600/ADC)
- **Simulator values**: `float` (knots, feet, degrees, etc.)

### Collective Axis

Collective is included in `SensorState` and `JoystickState` for web monitoring, but **is not part of autopilot logic** (for now). When AP is on, cyclic X/Y are computed by the autopilot; collective is always passed through from sensors.

### Usage

- **Autopilot logic**: Reads `simulator`, `sensors`, `autopilot`; writes `joystick.cyclicX`, `joystick.cyclicY` (collective unchanged)
- **Web API**: Serializes full `AppState` (or subgroups) for dashboard display; POST endpoints for mode changes and tuning

---

## Implementation Status

### Done

- [x] State struct definitions in `include/state.h`
- [x] Web interface refactored to LittleFS static files
- [x] This document (AUTOPILOT.md)
- [x] Global state integrated: cyclic_serial, collective, joystick now use `state`
- [x] Simulator data receiver (simulator_serial.cpp)
- [x] AP control logic: PitchHold, RollHold, HeadingHold, VerticalSpeed
- [x] Real-time PID tuning via web (pitch, roll, headingKp, vsKp)

### Pending

- [ ] AltitudeHold mode (future)

### API

- **GET /api/state** – Returns complete state as JSON: `{ sensors, joystick, autopilot, simulator }`
- **POST /api/autopilot** – Set AP state. Body may include:
  - `enabled` (bool)
  - `horizontalMode` ("off" | "roll" | "hdg")
  - `verticalMode` ("off" | "pitch" | "vs" | "alts")
  - `selectedHeading`, `selectedPitch`, `selectedRoll`, `selectedVerticalSpeed` (float)
  - Returns updated state.
- **POST /api/pid** – Update PID gains. Body: `pitchKp`, `pitchKi`, `pitchKd`, `rollKp`, `rollKi`, `rollKd`, `headingKp`, `vsKp`. Returns updated state.
- **WebSocket (port 81)** – Broadcasts full state for real-time updates

### Simulator Serial Protocol

Simulator sends JSON messages over UART (Serial2). Autopilot requires valid simulator data to enable.

**Connection:**
- ESP32 GPIO 43 (SIM_RX) ← Simulator TX
- GND ← GND
- Baud: 115200 (config: `SIM_SERIAL_BAUD`)

**Format:** Newline-separated JSON. Each line is one message. Partial updates supported (only include fields that changed).

**Field names:**

| Field   | Meaning        | Unit              |
|---------|----------------|-------------------|
| `spd`   | Speed          | knots or m/s      |
| `alt`   | Altitude       | feet or meters    |
| `pitch` | Pitch angle    | degrees           |
| `roll`  | Roll angle     | degrees           |
| `hdg`   | Heading        | degrees (0–360)   |
| `vs`    | Vertical speed | ft/min or m/s     |

**Example:**
```json
{"spd":85,"alt":2500,"pitch":2.5,"roll":-1,"hdg":270,"vs":0}
```

**Validity:** Data is considered valid for `SIMULATOR_VALID_TIMEOUT_MS` (default 5 s) after last received message. AP cannot be enabled without valid simulator data.

### AP Module (ap.cpp)

- `initAP()`, `setAPEnabled(bool)`, `setAPHorizontalMode()`, `setAPVerticalMode()`, `handleAP()`, `syncAPPidTunings()`
- `canAutopilotBeOn()`: simulator data valid + speed ≥ `AP_MIN_SPEED_KNOTS` (10)
- When turning ON: defaults to RollHold + PitchHold, captures pitch/roll from simulator (or 0)
- AP enable rejected if `!canAutopilotBeOn()`
- `handleAP()`:
  - Safety: turns AP off if `!canAutopilotBeOn()` (triple beep alert)
  - **Vertical**: PitchHold (pitch PID) or VerticalSpeed (outer loop: VS error × vsKp → target pitch → pitch PID). Cyclic Y from PID when either mode active.
  - **Horizontal**: RollHold or HeadingHold (outer loop: heading error × headingKp → target roll → roll PID). Cyclic X from roll PID when either mode active.
  - Collective always pass-through from sensors
- **cyclic_serial**: Does NOT update joystick cyclic X when AP controls roll (RollHold or HeadingHold); does NOT update cyclic Y when AP controls pitch (PitchHold or VerticalSpeed)
- **PI Control (VS)**: Vertical Speed uses a PI controller (Kp=0.02, Ki=0.0001) to eliminate steady-state error.
- **Bumpless Transfer**: When AP turns ON, the PID output is initialized to the current physical cyclic position (captured from sensors) to prevent control kicks.
- **Smoothing**: Navigation targets (target bank/target pitch) are smoothed using an exponential filter (alpha=0.1) to damping oscillations.
- **Safety**: Triple beep alert on automatic safety disconnect.

### Config (config.h)

- `AP_PITCH_KP/KI/KD`, `AP_ROLL_KP/KI/KD` – inner loop defaults
- `AP_HEADING_KP`, `AP_MAX_BANK_ANGLE` – heading outer loop
- `AP_VS_KP`, `AP_MAX_PITCH_ANGLE` – vertical speed outer loop

---

## Change Log

| Date | Change |
|------|--------|
| (initial) | Created state.h with AutopilotState, SimulatorState, SensorState, JoystickState, AppState. Created AUTOPILOT.md. |
| (integrated) | Added state.cpp with global `state`. cyclic_serial, collective, joystick now read/write state. Getters (getJoystickAxis, getCyclicXRaw, etc.) read from state. Web API continues to work via existing getters. |
| (web ui) | New axis display: X-Y box with sensor (cyan) and joystick (orange) points; collective as rising bar. Added GET /api/state. WebSocket now sends full state (sensors + joystick). |
| (ap control) | Added ap.cpp/ap.h skeleton. POST /api/autopilot with `{enabled:bool}`. Toggle button on web page. State: selectedPitch, selectedRoll. Turn-on defaults to RollHold+PitchHold. |
| (simulator) | Added simulator_serial.cpp/h. JSON over Serial2 (GPIO43/44), newline-separated. Fields: spd, alt, pitch, roll, hdg, vs. Updates state.simulator + lastUpdateMs. |
| (ap logic) | handleAP() in main loop. canAutopilotBeOn() = simulator valid + speed ≥ 10 kt. Pitch hold via PID (Kp=200, Kd=50). Cyclic X from sensors, cyclic Y from PID. |
| (ap stability)| Gated sensor connection: Sensor modules now update joystick axes directly ONLY if that axis is not being controlled by AP. handleAP is now the ONLY place that touches the Pitch axis during PitchHold. Roll and Collective remain directly connected to sensors to maintain full manual control. |
| (pid tuning) | Added real-time PID tuning via web interface. POST /api/pid endpoint updates pitchKp, pitchKi, pitchKd, rollKp, rollKi, rollKd, headingKp in global state. Controller update rate synced with simulator data. |
| (heading hold) | HeadingHold mode: outer loop (heading error × headingKp → target roll) feeds roll PID. headingKp tunable via web. |
| (vs mode) | VerticalSpeed mode: outer loop (VS error × vsKp → target pitch) feeds pitch PID. selectedVerticalSpeed from web (sync/adjust). cyclic_serial blocks pitch pass-through when VS active. |
| (vs tuning) | vsKp tunable via web (PID Tuning section). POST /api/pid accepts vsKp. |
| (bumpless) | Implemented bumpless transfer. On AP Enable, captured sensor physical offset is pre-loaded into PID integral/output to prevent engagement "kick". |
| (vs upgrade) | VS mode upgraded to PI control. Added vsIntegral with anti-windup to kill steady-state error. Fixed direction: (Sim - Target) error correctly commands pitch up (negative) for climb capture. Added 0.1 alpha smoothing filter to nav target. |
| (alerts) | Added audible safety alerts. Triple beep on safety-induced AP disconnect. |