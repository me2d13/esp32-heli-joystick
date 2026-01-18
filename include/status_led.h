#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <Arduino.h>

// LED Status enum
enum LEDStatus {
    LED_STARTUP,
    LED_WIFI_DISABLED,
    LED_WIFI_CONNECTING,
    LED_WIFI_CONNECTED,
    LED_WIFI_FAILED,
    LED_OFF
};

// Initialize the status LED
void initStatusLED();

// Set LED to a specific status
void setLEDStatus(LEDStatus status);

// Update LED (call in loop for animations like blinking)
void updateStatusLED();

#endif // STATUS_LED_H
