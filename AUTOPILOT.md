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
- `PitchHold` – Hold current pitch
- `VerticalSpeed` – Hold `selectedVerticalSpeed`
- `AltitudeHold` – Hold `selectedAltitude` (future)

### Selected Values

For hold modes that need a target: `selectedHeading`, `selectedAltitude`, `selectedVerticalSpeed` with corresponding `hasSelected*` flags when the value is armed/active.

### Data Representation

- **Axis values**: 0–10000 (joystick range, center 5000) – matches existing `joystick.h`
- **Raw sensors**: 0–4095 (AS5600/ADC)
- **Simulator values**: `float` (knots, feet, degrees, etc.)

### Collective Axis

Collective is included in `SensorState` and `JoystickState` for web monitoring, but **is not part of autopilot logic** (for now). When AP is on, cyclic X/Y are computed by the autopilot; collective is always passed through from sensors.

### Usage (Planned)

- **Autopilot logic**: Reads `simulator`, `sensors`, `autopilot`; writes `joystick.cyclicX`, `joystick.cyclicY` (collective unchanged)
- **Web API**: Serializes full `AppState` (or subgroups) for dashboard display

---

## Implementation Status

### Done

- [x] State struct definitions in `include/state.h`
- [x] Web interface refactored to LittleFS static files
- [x] This document (AUTOPILOT.md)
- [x] Global state integrated: cyclic_serial, collective, joystick now use `state`

### Pending

- [ ] Simulator data receiver
- [ ] Simulator data receiver (protocol TBD)
- [ ] Autopilot mode switching (button binding)
- [ ] Autopilot control logic
- [ ] Web API endpoints for state display
- [ ] Web UI updates for state monitoring

---

## Change Log

| Date | Change |
|------|--------|
| (initial) | Created state.h with AutopilotState, SimulatorState, SensorState, JoystickState, AppState. Created AUTOPILOT.md. |
| (integrated) | Added state.cpp with global `state`. cyclic_serial, collective, joystick now read/write state. Getters (getJoystickAxis, getCyclicXRaw, etc.) read from state. Web API continues to work via existing getters. |