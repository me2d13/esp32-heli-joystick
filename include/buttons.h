#ifndef BUTTONS_H
#define BUTTONS_H

#include <Arduino.h>

// ----------------------------------------------------------------------------
// Built-in actions that can be assigned to any HID button number.
// Add new entries here; implement them in performButtonAction() in buttons.cpp.
// ----------------------------------------------------------------------------
typedef enum {
  ACTION_NONE              = 0,
  ACTION_TOGGLE_CYCLIC_HOLD,    // Toggle stepper-motor cyclic hold
  ACTION_TOGGLE_COLLECTIVE_HOLD,// Toggle stepper-motor collective hold
  ACTION_TOGGLE_AP,             // Toggle autopilot on/off
  // Future actions go here, e.g.:
  // ACTION_AP_HEADING_MODE,
  // ACTION_AP_HEADING_PLUS10,
} ButtonAction;

// Initialize button handling system
void initButtons();

// Handle button scanning and state updates
// Should be called regularly from loop()
void handleButtons();

#endif // BUTTONS_H
