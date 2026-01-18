#ifndef BUTTONS_H
#define BUTTONS_H

#include <Arduino.h>

// Initialize button handling system
void initButtons();

// Handle button scanning and state updates
// Should be called regularly from loop()
void handleButtons();

#endif // BUTTONS_H
