#ifndef PROFILE_H
#define PROFILE_H

#include <Arduino.h>

// Initialize profiling (call from setup)
void initProfile();

// Start timing a slot (call at start of task)
void profileStart(uint8_t slot);

// End timing and record (call at end of task). Logs if duration exceeds threshold.
void profileEnd(uint8_t slot);

// Slot indices for main loop tasks
#define PROFILE_BUTTONS        0
#define PROFILE_CYCLIC_SERIAL  1
#define PROFILE_SIMULATOR      2
#define PROFILE_COLLECTIVE     3
#define PROFILE_AP             4
#define PROFILE_STEPPERS       5
#define PROFILE_CYCLIC_FEEDBACK 6
#define PROFILE_BUZZER         7
#define PROFILE_JOYSTICK       8
#define PROFILE_STATUS_LED     9
#define PROFILE_SLOT_COUNT     10

// Get last/max ms for a slot (for debug API)
unsigned long profileGetLastMs(uint8_t slot);
unsigned long profileGetMaxMs(uint8_t slot);
const char* profileGetName(uint8_t slot);

#endif // PROFILE_H
