#include "status_led.h"
#include "config.h"
#include <Adafruit_NeoPixel.h>

// Create NeoPixel object
Adafruit_NeoPixel rgbLed(NUM_PIXELS, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);

// LED Status Colors
#define COLOR_STARTUP      rgbLed.Color(128, 0, 128)  // Purple - System starting
#define COLOR_WIFI_DISABLED rgbLed.Color(255, 0, 0)   // Red - WiFi disabled
#define COLOR_WIFI_CONNECTING rgbLed.Color(255, 255, 0) // Yellow - Connecting to WiFi
#define COLOR_WIFI_CONNECTED rgbLed.Color(0, 255, 0)  // Green - WiFi connected
#define COLOR_WIFI_FAILED  rgbLed.Color(255, 0, 0)    // Red - WiFi connection failed
#define COLOR_OFF          rgbLed.Color(0, 0, 0)      // Off

// Current status
LEDStatus currentStatus = LED_OFF;

// Blinking state for animations
unsigned long lastBlinkTime = 0;
bool blinkState = false;

void initStatusLED() {
    rgbLed.begin();
    rgbLed.setBrightness(RGB_LED_BRIGHTNESS);
    rgbLed.clear();
    rgbLed.show();
}

void setLEDColor(uint32_t color) {
    rgbLed.setPixelColor(0, color);
    rgbLed.show();
}

void setLEDStatus(LEDStatus status) {
    currentStatus = status;
    
    switch (status) {
        case LED_STARTUP:
            setLEDColor(COLOR_STARTUP);
            break;
        case LED_WIFI_DISABLED:
            setLEDColor(COLOR_WIFI_DISABLED);
            break;
        case LED_WIFI_CONNECTING:
            // Blinking is handled in updateStatusLED()
            blinkState = true;
            setLEDColor(COLOR_WIFI_CONNECTING);
            lastBlinkTime = millis();
            break;
        case LED_WIFI_CONNECTED:
            setLEDColor(COLOR_WIFI_CONNECTED);
            break;
        case LED_WIFI_FAILED:
            setLEDColor(COLOR_WIFI_FAILED);
            break;
        case LED_OFF:
            setLEDColor(COLOR_OFF);
            break;
    }
}

void updateStatusLED() {
    // Handle blinking animation for connecting status
    if (currentStatus == LED_WIFI_CONNECTING) {
        unsigned long currentTime = millis();
        if (currentTime - lastBlinkTime >= 500) {
            lastBlinkTime = currentTime;
            blinkState = !blinkState;
            setLEDColor(blinkState ? COLOR_WIFI_CONNECTING : COLOR_OFF);
        }
    }
}
