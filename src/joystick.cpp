#include "joystick.h"
#include <Joystick_ESP32S2.h>
#include "USB.h"
#include "logger.h"

// Create joystick instance
// Parameters: (hidReportId, joystickType, buttonCount, hatSwitchCount, 
//              includeXAxis, includeYAxis, includeZAxis, includeRxAxis, includeRyAxis, includeRzAxis,
//              includeRudder, includeThrottle, includeAccelerator, includeBrake, includeSteering)
Joystick_ Joystick(
    JOYSTICK_DEFAULT_REPORT_ID,
    JOYSTICK_TYPE_JOYSTICK,
    32,     // 32 buttons
    0,      // 0 hat switches
    true,   // X axis (Cyclic X)
    true,   // Y axis (Cyclic Y)
    true,   // Z axis (Collective)
    false,  // No Rx
    false,  // No Ry
    false,  // No Rz
    false,  // No rudder
    false,  // No throttle
    false,  // No accelerator
    false,  // No brake
    false   // No steering
);

// State tracking
static int8_t axisValues[JOYSTICK_AXIS_COUNT] = {0, 0, 0};
static uint32_t buttonStates = 0;

// Smooth animation variables for demo
static unsigned long lastUpdateTime = 0;
static float animationPhase = 0.0f;

void initJoystick() {
    // Configure USB device name BEFORE starting joystick
    USB.productName("esp-heli-v1");
    USB.manufacturerName("ESP32");
    USB.begin();
    
    // Set axis ranges (-127 to 127)
    Joystick.setXAxisRange(AXIS_MIN, AXIS_MAX);
    Joystick.setYAxisRange(AXIS_MIN, AXIS_MAX);
    Joystick.setZAxisRange(AXIS_MIN, AXIS_MAX);
    
    // Begin joystick with manual send mode (false = disable AutoSendState)
    Joystick.begin(false);
    
    LOG_INFO("USB HID Joystick initialized: esp-heli-v1");
    LOG_INFO("3 axes (Cyclic X, Cyclic Y, Collective) + 32 buttons");
}

void setJoystickAxis(uint8_t axis, int8_t value) {
    if (axis >= JOYSTICK_AXIS_COUNT) return;
    
    // Clamp value to valid range
    if (value < AXIS_MIN) value = AXIS_MIN;
    if (value > AXIS_MAX) value = AXIS_MAX;
    
    // Store value
    axisValues[axis] = value;
    
    // Update joystick
    switch (axis) {
        case AXIS_CYCLIC_X:
            Joystick.setXAxis(value);
            break;
        case AXIS_CYCLIC_Y:
            Joystick.setYAxis(value);
            break;
        case AXIS_COLLECTIVE:
            Joystick.setZAxis(value);
            break;
    }
}

void setJoystickButton(uint8_t button, bool pressed) {
    if (button < JOYSTICK_BUTTON_COUNT) {
        // Update state tracking
        if (pressed) {
            buttonStates |= (1UL << button);
        } else {
            buttonStates &= ~(1UL << button);
        }
        
        // Update joystick
        Joystick.setButton(button, pressed);
    }
}

void updateJoystick() {
    // Send HID report
    Joystick.sendState();
}

int8_t getJoystickAxis(uint8_t axis) {
    if (axis < JOYSTICK_AXIS_COUNT) {
        return axisValues[axis];
    }
    return 0;
}

bool getJoystickButton(uint8_t button) {
    if (button < JOYSTICK_BUTTON_COUNT) {
        return (buttonStates & (1UL << button)) != 0;
    }
    return false;
}

// Demo function: Generate smooth sine wave movements for axes
void updateJoystickDemo() {
    unsigned long currentTime = millis();
    
    // Update animation at 60 Hz
    if (currentTime - lastUpdateTime >= 16) {
        lastUpdateTime = currentTime;
        
        // Increment animation phase
        animationPhase += 0.02f;
        if (animationPhase > TWO_PI) {
            animationPhase -= TWO_PI;
        }
        
        // Generate smooth sine wave movements
        // Cyclic X: slow sine wave
        int8_t cyclicX = (int8_t)(sin(animationPhase) * 100.0f);
        
        // Cyclic Y: slower sine wave with phase offset
        int8_t cyclicY = (int8_t)(sin(animationPhase * 0.7f + PI/4) * 100.0f);
        
        // Collective: very slow sine wave
        int8_t collective = (int8_t)(sin(animationPhase * 0.5f) * 80.0f);
        
        // Update axes
        setJoystickAxis(AXIS_CYCLIC_X, cyclicX);
        setJoystickAxis(AXIS_CYCLIC_Y, cyclicY);
        setJoystickAxis(AXIS_COLLECTIVE, collective);
        
        // Send updated state
        updateJoystick();
    }
}
