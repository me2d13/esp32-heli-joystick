#ifndef STEPPERS_H
#define STEPPERS_H

#include <Arduino.h>

// Initialize all stepper motors
void initSteppers();

// Handle stepper motor state (call from main loop)
void handleSteppers();

// Toggle collective motor hold state
void toggleCollectiveHold();

// Toggle cyclic X and Y motors hold state
void toggleCyclicHold();

// Get current hold states
bool isCollectiveHeld();
bool isCyclicHeld();

// Step cyclic motors (for cyclic feedback - only when cyclic is held)
// dir: true = one direction, false = other (direction depends on wiring)
void stepCyclicX(bool dir);
void stepCyclicY(bool dir);

#endif // STEPPERS_H
