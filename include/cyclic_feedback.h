#ifndef CYCLIC_FEEDBACK_H
#define CYCLIC_FEEDBACK_H

// Initialize cyclic feedback module
void initCyclicFeedback();

// Main loop - call when cyclic feedback may be active.
// Moves X-Y steppers to chase joystick position (smooth, rate-limited).
void handleCyclicFeedback();

#endif // CYCLIC_FEEDBACK_H
