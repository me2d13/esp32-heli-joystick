#include "state.h"

// Global application state - single source of truth for sensors, joystick output,
// and (future) autopilot/simulator data. Written by cyclic_serial, collective,
// joystick modules; read by web API and autopilot logic.
AppState state;
