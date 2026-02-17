#ifndef STATE_H
#define STATE_H

#include <Arduino.h>

// =============================================================================
// Application State - Autopilot & Monitoring
// =============================================================================
// Centralized state for autopilot logic and web API display.
// Sensor and joystick state are now integrated; autopilot/simulator pending.
// See AUTOPILOT.md for design rationale and integration plan.
// =============================================================================

// -----------------------------------------------------------------------------
// Autopilot Mode Configuration
// -----------------------------------------------------------------------------

enum class APHorizontalMode : uint8_t {
    Off,        // Manual control - pass through sensor/joystick
    RollHold,   // Hold current roll angle
    HeadingHold // Hold selected heading (uses selectedHeading)
};

enum class APVerticalMode : uint8_t {
    Off,            // Manual control
    PitchHold,      // Hold current pitch angle
    VerticalSpeed,  // Hold selected vertical speed
    AltitudeHold    // Hold selected altitude (future)
};

struct AutopilotState {
    bool enabled = false;

    APHorizontalMode horizontalMode = APHorizontalMode::Off;
    APVerticalMode verticalMode = APVerticalMode::Off;

    // Selected values for hold modes (used when mode is active)
    float selectedHeading = 0.0f;   // degrees, 0-360
    float selectedAltitude = 0.0f;  // feet or meters (future)
    float selectedVerticalSpeed = 0.0f;  // ft/min or m/s

    // Flags: is the selected value currently active/armed?
    bool hasSelectedHeading = false;
    bool hasSelectedAltitude = false;
    bool hasSelectedVerticalSpeed = false;
};

// -----------------------------------------------------------------------------
// Simulator Data (received from flight sim)
// -----------------------------------------------------------------------------

struct SimulatorState {
    bool valid = false;           // Have we received data recently?
    unsigned long lastUpdateMs = 0;  // Timestamp of last update

    float speed = 0.0f;           // knots or m/s
    float altitude = 0.0f;        // feet or meters
    float pitch = 0.0f;          // degrees
    float roll = 0.0f;           // degrees
    float heading = 0.0f;        // degrees, 0-360
    float verticalSpeed = 0.0f;  // ft/min or m/s
};

// -----------------------------------------------------------------------------
// Sensor Readings (raw + calibrated)
// -----------------------------------------------------------------------------
// Raw: 0-4095 (AS5600 / ADC range)
// Calibrated: 0-10000 (joystick axis range, center at 5000)

struct SensorState {
    // Cyclic X (left/right)
    uint16_t cyclicXRaw = 0;
    int16_t cyclicXCalibrated = 5000;  // 0-10000, center 5000

    // Cyclic Y (forward/back)
    uint16_t cyclicYRaw = 0;
    int16_t cyclicYCalibrated = 5000;

    // Collective (up/down)
    // Note: Collective is in state for web monitoring only. Not used by autopilot logic (for now).
    uint16_t collectiveRaw = 0;
    int16_t collectiveCalibrated = 5000;

    bool cyclicValid = false;  // Is cyclic serial data current?
};

// -----------------------------------------------------------------------------
// Joystick Output (values sent to PC)
// -----------------------------------------------------------------------------
// When AP is off: matches sensor calibrated values
// When AP is on: cyclic X/Y computed by autopilot; collective always pass-through

struct JoystickState {
    int16_t cyclicX = 5000;   // 0-10000
    int16_t cyclicY = 5000;
    int16_t collective = 5000;  // Not controlled by AP - always from sensors

    uint32_t buttons = 0;    // Bitmask for 32 buttons
};

// -----------------------------------------------------------------------------
// Combined Application State
// -----------------------------------------------------------------------------

struct AppState {
    AutopilotState autopilot;
    SimulatorState simulator;
    SensorState sensors;
    JoystickState joystick;
};

// Global state instance (defined in state.cpp)
extern AppState state;

#endif // STATE_H
