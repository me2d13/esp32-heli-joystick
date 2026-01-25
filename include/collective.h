#ifndef COLLECTIVE_H
#define COLLECTIVE_H

#include <Arduino.h>

// Initialize the collective axis reading
void initCollective();

// Handle collective axis reading and update joystick
// Should be called regularly from loop()
void handleCollective();

// Get the last raw ADC reading (0-4095 for ESP32)
uint16_t getCollectiveRaw();

// Get the last mapped axis value (-127 to 127)
int8_t getCollectiveAxis();

#endif // COLLECTIVE_H
