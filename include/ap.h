#ifndef AP_H
#define AP_H

#include "state.h"

// Initialize autopilot module
void initAP();

// Enable or disable autopilot
// When turning on without explicit modes: starts with RollHold + PitchHold
// Captures current pitch/roll from simulator (or 0 if no simulator data)
void setAPEnabled(bool enabled);

// Set horizontal mode (for future use)
void setAPHorizontalMode(APHorizontalMode mode);

// Set vertical mode (for future use)
void setAPVerticalMode(APVerticalMode mode);

#endif // AP_H
