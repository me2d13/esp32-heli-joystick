#include "cyclic_feedback.h"
#include "config.h"
#include "state.h"
#include "steppers.h"
#include "logger.h"

static unsigned long lastStepXMs = 0;
static unsigned long lastStepYMs = 0;

void initCyclicFeedback() {
    lastStepXMs = 0;
    lastStepYMs = 0;
    LOG_INFO("Cyclic feedback module initialized");
}

void handleCyclicFeedback() {
    // Active only when: cyclic feedback on, AP on, cyclic motors held
    if (!state.cyclicFeedbackEnabled || !state.autopilot.enabled || !isCyclicHeld()) {
        return;
    }

    // Need valid sensor data to know current position
    if (!state.sensors.cyclicValid) {
        return;
    }

    unsigned long now = millis();

    // Target = what we're sending to PC (joystick output)
    int16_t targetX = state.joystick.cyclicX;
    int16_t targetY = state.joystick.cyclicY;

    // Current = physical stick position from sensors
    int16_t currentX = state.sensors.cyclicXCalibrated;
    int16_t currentY = state.sensors.cyclicYCalibrated;

    int16_t errorX = targetX - currentX;
    int16_t errorY = targetY - currentY;

#if CYCLIC_FEEDBACK_X_ENABLED
    // X axis
    if (abs(errorX) > CYCLIC_FEEDBACK_DEADBAND &&
        (now - lastStepXMs) >= CYCLIC_FEEDBACK_STEP_MS) {
        bool dirX = (errorX > 0) ? (bool)CYCLIC_FEEDBACK_X_DIR_POS : !(bool)CYCLIC_FEEDBACK_X_DIR_POS;
        stepCyclicX(dirX);
        lastStepXMs = now;
    }
#endif

    // Y axis
    if (abs(errorY) > CYCLIC_FEEDBACK_DEADBAND &&
        (now - lastStepYMs) >= CYCLIC_FEEDBACK_STEP_MS) {
        bool dirY = (errorY > 0) ? (bool)CYCLIC_FEEDBACK_Y_DIR_POS : !(bool)CYCLIC_FEEDBACK_Y_DIR_POS;
        stepCyclicY(dirY);
        lastStepYMs = now;
    }
}
