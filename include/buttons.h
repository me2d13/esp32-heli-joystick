#ifndef BUTTONS_H
#define BUTTONS_H

#include <Arduino.h>

// ----------------------------------------------------------------------------
// Built-in actions that can be assigned to any HID button number.
// Add new entries here; implement them in performButtonAction() in buttons.cpp.
// ----------------------------------------------------------------------------
typedef enum {
  ACTION_NONE                  = 0,
  ACTION_TOGGLE_CYCLIC_HOLD,       // Toggle stepper-motor cyclic hold
  ACTION_TOGGLE_COLLECTIVE_HOLD,   // Toggle stepper-motor collective hold
  ACTION_TOGGLE_AP,                // Toggle autopilot on/off
  ACTION_AP_HDG_MODE,              // Engage AP heading hold mode
  ACTION_AP_VS_MODE,               // Engage AP vertical speed mode
  ACTION_AP_HDG_MINUS10,           // Selected heading -10 degrees
  ACTION_AP_HDG_PLUS10,            // Selected heading +10 degrees
  ACTION_AP_VS_PLUS100,            // Selected VS +100 ft/min
  ACTION_AP_VS_MINUS100,           // Selected VS -100 ft/min
} ButtonAction;

// Initialize button handling system
void initButtons();

// Handle button scanning and state updates
// Should be called regularly from loop()
void handleButtons();

#endif // BUTTONS_H
