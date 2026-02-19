#ifndef SIMULATOR_SERIAL_H
#define SIMULATOR_SERIAL_H

#include <Arduino.h>

// Initialize simulator serial receiver
void initSimulatorSerial();

// Process incoming serial data (call from main loop)
void handleSimulatorSerial();

/*
 * JSON protocol: newline-separated messages (\n)
 * Each message updates state.simulator. Partial updates supported.
 *
 * Field names (all optional):
 *   spd   - speed (knots or m/s)
 *   alt   - altitude (feet or meters)
 *   pitch - pitch angle (degrees)
 *   roll  - roll angle (degrees)
 *   hdg   - heading (degrees, 0-360)
 *   vs    - vertical speed (ft/min or m/s)
 *
 * Example: {"spd":85,"alt":2500,"pitch":2.5,"roll":-1,"hdg":270,"vs":0}\n
 */

#endif // SIMULATOR_SERIAL_H
