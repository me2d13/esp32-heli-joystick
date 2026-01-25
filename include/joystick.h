#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <Arduino.h>

// Joystick configuration
#define JOYSTICK_AXIS_COUNT 3
#define JOYSTICK_BUTTON_COUNT 32

// Axis indices
#define AXIS_CYCLIC_X 0     // Cyclic left/right
#define AXIS_CYCLIC_Y 1     // Cyclic forward/back
#define AXIS_COLLECTIVE 2   // Collective up/down

// Axis range: 0 to 10000 (16-bit precision for full sensor resolution)
#define AXIS_MIN 0
#define AXIS_MAX 10000
#define AXIS_CENTER 5000

// Initialize the USB HID joystick
void initJoystick();

// Set axis value (0 to 10000)
void setJoystickAxis(uint8_t axis, int16_t value);

// Set button state (0-31)
void setJoystickButton(uint8_t button, bool pressed);

// Update and send joystick state to host
void updateJoystick();

// Get current axis value
int16_t getJoystickAxis(uint8_t axis);

// Get current button state
bool getJoystickButton(uint8_t button);

// Demo function: Update joystick with smooth animated movements
void updateJoystickDemo();

#endif // JOYSTICK_H
