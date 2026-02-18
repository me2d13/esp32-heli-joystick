#include "ap.h"
#include "config.h"
#include "state.h"
#include "logger.h"

static bool isSimulatorDataValid() {
    if (state.simulator.lastUpdateMs == 0) {
        return false;
    }
    return (millis() - state.simulator.lastUpdateMs) < SIMULATOR_VALID_TIMEOUT_MS;
}

void initAP() {
    state.autopilot.enabled = false;
    state.autopilot.horizontalMode = APHorizontalMode::Off;
    state.autopilot.verticalMode = APVerticalMode::Off;
    LOG_INFO("Autopilot module initialized");
}

void setAPEnabled(bool enabled) {
    if (enabled == state.autopilot.enabled) {
        return;
    }

    if (enabled && !isSimulatorDataValid()) {
        LOG_WARNF("Attempt to turn AP ON without simulator data (no data in last %d s)",
                  SIMULATOR_VALID_TIMEOUT_MS / 1000);
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

        LOG_INFO("Autopilot ON (RollHold + PitchHold)");
    } else {
        state.autopilot.horizontalMode = APHorizontalMode::Off;
        state.autopilot.verticalMode = APVerticalMode::Off;
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
