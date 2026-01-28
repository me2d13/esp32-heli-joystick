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

// Rainbow mode state
bool rainbowModeActive = false;
unsigned long greenStatusStartTime = 0;
uint16_t rainbowHue = 0;  // 0-65535 for full color wheel
unsigned long lastRainbowUpdate = 0;

// Helper function to convert HSV to RGB
// h: 0-65535, s: 0-255, v: 0-255
uint32_t hsvToRgb(uint16_t h, uint8_t s, uint8_t v) {
    uint8_t r, g, b;
    
    // Convert hue to 0-1535 range for easier calculation
    uint16_t hue = (h * 6) / 256;
    uint8_t region = hue / 256;
    uint8_t remainder = (hue % 256);
    
    uint8_t p = (v * (255 - s)) >> 8;
    uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;
    
    switch (region) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
    
    return rgbLed.Color(r, g, b);
}

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
    
    // Reset rainbow mode if status changes from connected
    if (status != LED_WIFI_CONNECTED) {
        rainbowModeActive = false;
        greenStatusStartTime = 0;
    }
    
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
            // Start timer for rainbow mode
            if (greenStatusStartTime == 0) {
                greenStatusStartTime = millis();
            }
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
    unsigned long currentTime = millis();
    
    // Handle blinking animation for connecting status
    if (currentStatus == LED_WIFI_CONNECTING) {
        if (currentTime - lastBlinkTime >= 500) {
            lastBlinkTime = currentTime;
            blinkState = !blinkState;
            setLEDColor(blinkState ? COLOR_WIFI_CONNECTING : COLOR_OFF);
        }
        return;
    }
    
    // Handle rainbow mode for connected status
    if (currentStatus == LED_WIFI_CONNECTED) {
        // Check if enough time has passed to activate rainbow mode
        if (!rainbowModeActive && greenStatusStartTime > 0) {
            if (currentTime - greenStatusStartTime >= LED_RAINBOW_DELAY_MS) {
                rainbowModeActive = true;
                rainbowHue = 0;
                lastRainbowUpdate = currentTime;
            }
        }
        
        // Update rainbow colors
        if (rainbowModeActive) {
            // Update color based on speed setting
            if (currentTime - lastRainbowUpdate >= (20 / LED_RAINBOW_SPEED)) {
                lastRainbowUpdate = currentTime;
                
                // Increment hue for smooth color transition
                rainbowHue += 256;  // Adjust step size for smoothness
                if (rainbowHue >= 65536) {
                    rainbowHue = 0;
                }
                
                // Convert HSV to RGB (full saturation and brightness)
                uint32_t color = hsvToRgb(rainbowHue, 255, 255);
                setLEDColor(color);
            }
        }
    }
}
