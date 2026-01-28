#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

// Initialize the buzzer
void initBuzzer();

// Handle buzzer state machine (call from main loop)
void handleBuzzer();

// Single beep (non-blocking)
// duration: beep duration in milliseconds (default: 100ms)
void beep(uint16_t duration = 100);

// Double beep (non-blocking - two short beeps)
// duration: duration of each beep in milliseconds (default: 100ms)
// gap: gap between beeps in milliseconds (default: 100ms)
void doubleBeep(uint16_t duration = 100, uint16_t gap = 100);

// Triple beep (non-blocking - three short beeps)
// duration: duration of each beep in milliseconds (default: 100ms)
// gap: gap between beeps in milliseconds (default: 100ms)
void tripleBeep(uint16_t duration = 100, uint16_t gap = 100);

#endif // BUZZER_H
