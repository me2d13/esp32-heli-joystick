#include "ap.h"
#include "config.h"
#include "state.h"
#include "logger.h"
#include "joystick.h"
#include <PID_v1.h>

static bool isSimulatorDataValid() {
    if (state.simulator.lastUpdateMs == 0) {
        return false;
    }
    return (millis() - state.simulator.lastUpdateMs) < SIMULATOR_VALID_TIMEOUT_MS;
}

bool canAutopilotBeOn() {
    if (!isSimulatorDataValid()) {
        return false;
    }
    if (state.simulator.speed < AP_MIN_SPEED_KNOTS) {
        return false;
    }
    return true;
}

// PID for pitch hold: setpoint=selectedPitch, input=actual pitch, output=cyclic Y offset
static double pidInput = 0;
static double pidOutput = 0;
static double pidSetpoint = 0;
static PID pitchPid(&pidInput, &pidOutput, &pidSetpoint,
                    AP_PITCH_KP, AP_PITCH_KI, AP_PITCH_KD, 1);  // 1=REVERSE

void initAP() {
    state.autopilot.enabled = false;
    state.autopilot.horizontalMode = APHorizontalMode::Off;
    state.autopilot.verticalMode = APVerticalMode::Off;

    pitchPid.SetOutputLimits(-5000, 5000);
    pitchPid.SetSampleTime(20);  // 50 Hz
    pitchPid.SetMode(1);  // 1=AUTOMATIC

    LOG_INFO("Autopilot module initialized");
}

void setAPEnabled(bool enabled) {
    if (enabled == state.autopilot.enabled) {
        return;
    }

    if (enabled && !canAutopilotBeOn()) {
        if (!isSimulatorDataValid()) {
            LOG_WARNF("Attempt to turn AP ON without simulator data (no data in last %d s)",
                      SIMULATOR_VALID_TIMEOUT_MS / 1000);
        } else {
            LOG_WARNF("Attempt to turn AP ON: speed %.1f < %d knots required",
                      state.simulator.speed, AP_MIN_SPEED_KNOTS);
        }
        return;
    }

    state.autopilot.enabled = enabled;

    if (enabled) {
        // Turn on: default to RollHold + PitchHold
        state.autopilot.horizontalMode = APHorizontalMode::RollHold;
        state.autopilot.verticalMode = APVerticalMode::PitchHold;

        // Capture current pitch/roll from simulator for hold targets
        if (state.simulator.valid) {
            state.autopilot.selectedRoll = state.simulator.roll;
            state.autopilot.selectedPitch = state.simulator.pitch;
        } else {
            state.autopilot.selectedRoll = 0.0f;
            state.autopilot.selectedPitch = 0.0f;
        }

        pitchPid.SetMode(1);  // 1=AUTOMATIC
        LOG_INFO("Autopilot ON (RollHold + PitchHold)");
    } else {
        state.autopilot.horizontalMode = APHorizontalMode::Off;
        state.autopilot.verticalMode = APVerticalMode::Off;
        pitchPid.SetMode(0);  // 0=MANUAL
        LOG_INFO("Autopilot OFF");
    }
}

void setAPHorizontalMode(APHorizontalMode mode) {
    state.autopilot.horizontalMode = mode;

    if (mode == APHorizontalMode::RollHold && state.simulator.valid) {
        state.autopilot.selectedRoll = state.simulator.roll;
    } else if (mode == APHorizontalMode::HeadingHold) {
        state.autopilot.hasSelectedHeading = true;
    }
}

void setAPVerticalMode(APVerticalMode mode) {
    state.autopilot.verticalMode = mode;

    if (mode == APVerticalMode::PitchHold && state.simulator.valid) {
        state.autopilot.selectedPitch = state.simulator.pitch;
    } else if (mode == APVerticalMode::AltitudeHold) {
        state.autopilot.hasSelectedAltitude = true;
    } else if (mode == APVerticalMode::VerticalSpeed) {
        state.autopilot.hasSelectedVerticalSpeed = true;
    }
}

void handleAP() {
    if (!state.autopilot.enabled) {
        return;
    }

    // Check if AP can stay on
    if (!canAutopilotBeOn()) {
        state.autopilot.enabled = false;
        state.autopilot.horizontalMode = APHorizontalMode::Off;
        state.autopilot.verticalMode = APVerticalMode::Off;
        pitchPid.SetMode(0);  // 0=MANUAL
        LOG_WARN("Autopilot OFF (simulator data lost or speed too low)");
        return;
    }

    // Horizontal: pass through sensor for now
    setJoystickAxis(AXIS_CYCLIC_X, state.sensors.cyclicXCalibrated);

    // Vertical: pitch hold via PID
    if (state.autopilot.verticalMode == APVerticalMode::PitchHold) {
        pidSetpoint = state.autopilot.selectedPitch;
        pidInput = state.simulator.pitch;

        if (pitchPid.Compute()) {
            // pidOutput is -5000..+5000, center 0. Joystick Y: 0=back, 10000=forward
            // Pull back (pitch up) = lower Y. REVERSE PID: need pitch up -> negative output
            int16_t cyclicY = (int16_t)(AXIS_CENTER + pidOutput);
            if (cyclicY < AXIS_MIN) cyclicY = AXIS_MIN;
            if (cyclicY > AXIS_MAX) cyclicY = AXIS_MAX;
            setJoystickAxis(AXIS_CYCLIC_Y, cyclicY);
        }
    } else {
        // Other vertical modes: pass through for now
        setJoystickAxis(AXIS_CYCLIC_Y, state.sensors.cyclicYCalibrated);
    }

    // Collective always from sensors
    setJoystickAxis(AXIS_COLLECTIVE, state.sensors.collectiveCalibrated);

    updateJoystick();
}
