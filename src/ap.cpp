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
static double pitchInput = 0;
static double pitchOutput = 0;
static double pitchSetpoint = 0;
static PID pitchPid(&pitchInput, &pitchOutput, &pitchSetpoint,
                    AP_PITCH_KP, AP_PITCH_KI, AP_PITCH_KD, 1);  // 1=REVERSE

// PID for roll hold: setpoint=selectedRoll, input=actual roll, output=cyclic X offset
static double rollInput = 0;
static double rollOutput = 0;
static double rollSetpoint = 0;
static PID rollPid(&rollInput, &rollOutput, &rollSetpoint,
                   AP_ROLL_KP, AP_ROLL_KI, AP_ROLL_KD, REVERSE);  // REVERSE: Input (roll right) up -> Output (joystick) down (move left)

void initAP() {
    state.autopilot.enabled = false;
    state.autopilot.horizontalMode = APHorizontalMode::Off;
    state.autopilot.verticalMode = APVerticalMode::Off;

    // Load initial PID parameters from config.h
    state.autopilot.pitchKp = AP_PITCH_KP;
    state.autopilot.pitchKi = AP_PITCH_KI;
    state.autopilot.pitchKd = AP_PITCH_KD;
    state.autopilot.rollKp = AP_ROLL_KP;
    state.autopilot.rollKi = AP_ROLL_KI;
    state.autopilot.rollKd = AP_ROLL_KD;

    pitchPid.SetTunings(state.autopilot.pitchKp, state.autopilot.pitchKi, state.autopilot.pitchKd);
    pitchPid.SetOutputLimits(-5000, 5000);
    pitchPid.SetSampleTime(0);
    pitchPid.SetMode(1);

    rollPid.SetTunings(state.autopilot.rollKp, state.autopilot.rollKi, state.autopilot.rollKd);
    rollPid.SetOutputLimits(-5000, 5000);
    rollPid.SetSampleTime(0);
    rollPid.SetMode(1);

    LOG_INFO("Autopilot module initialized");
}

void syncAPPidTunings() {
    pitchPid.SetTunings(state.autopilot.pitchKp, state.autopilot.pitchKi, state.autopilot.pitchKd);
    rollPid.SetTunings(state.autopilot.rollKp, state.autopilot.rollKi, state.autopilot.rollKd);
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
        rollPid.SetMode(1);
        LOG_INFO("Autopilot ON (RollHold + PitchHold)");
    } else {
        state.autopilot.horizontalMode = APHorizontalMode::Off;
        state.autopilot.verticalMode = APVerticalMode::Off;
        pitchPid.SetMode(0);  // 0=MANUAL
        rollPid.SetMode(0);
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
    bool newData = state.simulator.dataUpdated;
    state.simulator.dataUpdated = false;

    // 1. Safety check: Turn AP OFF if conditions lost
    if (state.autopilot.enabled && !canAutopilotBeOn()) {
        state.autopilot.enabled = false;
        state.autopilot.horizontalMode = APHorizontalMode::Off;
        state.autopilot.verticalMode = APVerticalMode::Off;
        pitchPid.SetMode(0);  // 0=MANUAL
        rollPid.SetMode(0);
        LOG_WARN("Autopilot OFF (simulator data lost or speed too low)");
    }

    if (!state.autopilot.enabled) {
        return;
    }

    // 2. Vertical: pitch hold via PID
    if (state.autopilot.verticalMode == APVerticalMode::PitchHold) {
        if (newData) {
            pitchPid.SetTunings(state.autopilot.pitchKp, state.autopilot.pitchKi, state.autopilot.pitchKd);
            pitchSetpoint = state.autopilot.selectedPitch;
            pitchInput = state.simulator.pitch;
            pitchPid.Compute();
        }
        
        int16_t cyclicY = (int16_t)(AXIS_CENTER + pitchOutput);
        if (cyclicY < AXIS_MIN) cyclicY = AXIS_MIN;
        if (cyclicY > AXIS_MAX) cyclicY = AXIS_MAX;
        setJoystickAxis(AXIS_CYCLIC_Y, cyclicY);
    }

    // 3. Horizontal: roll hold via PID
    if (state.autopilot.horizontalMode == APHorizontalMode::RollHold) {
        if (newData) {
            rollPid.SetTunings(state.autopilot.rollKp, state.autopilot.rollKi, state.autopilot.rollKd);
            rollSetpoint = state.autopilot.selectedRoll;
            rollInput = state.simulator.roll;
            rollPid.Compute();
        }

        int16_t cyclicX = (int16_t)(AXIS_CENTER + rollOutput);
        if (cyclicX < AXIS_MIN) cyclicX = AXIS_MIN;
        if (cyclicX > AXIS_MAX) cyclicX = AXIS_MAX;
        setJoystickAxis(AXIS_CYCLIC_X, cyclicX);
    }
}
