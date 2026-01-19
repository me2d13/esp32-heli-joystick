#ifndef CYCLIC_SERIAL_H
#define CYCLIC_SERIAL_H

#include <Arduino.h>

// Binary protocol constants (from esp32-as5600 project)
#define PACKET_START_MARKER   0xAA
#define PACKET_END_MARKER     0x55
#define PACKET_SIZE           7

// Initialize the serial receiver for cyclic sensor data
void initCyclicSerial();

// Process incoming serial data
// Call this in the main loop - it handles buffering and packet parsing
void handleCyclicSerial();

// Get the last received raw sensor values (0-4095)
uint16_t getCyclicXRaw();
uint16_t getCyclicYRaw();

// Check if we have received valid data recently
bool isCyclicDataValid();

// Get time since last valid packet (milliseconds)
unsigned long getCyclicDataAge();

#endif // CYCLIC_SERIAL_H
