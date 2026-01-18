#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// ESP32-S3 Helicopter Joystick - Configuration
// ============================================================================

// ----------------------------------------------------------------------------
// RGB LED Configuration
// ----------------------------------------------------------------------------
#define RGB_LED_PIN 48
#define NUM_PIXELS 1
#define RGB_LED_BRIGHTNESS 50  // Brightness (0-255)

// ----------------------------------------------------------------------------
// WiFi Configuration (Optional)
// ----------------------------------------------------------------------------
// WiFi credentials can be configured in two ways:
// 1. Create "secrets.h" in this directory with WIFI_SSID_SECRET and WIFI_PASSWORD_SECRET
//    (recommended - secrets.h is gitignored)
// 2. Or modify the default values below (not recommended for public repos)
//
// To use secrets.h:
//   - Copy secrets.h.template to secrets.h
//   - Edit secrets.h with your credentials
//   - secrets.h will be automatically used if it exists

// Try to include secrets.h if it exists (this file is gitignored)
#if __has_include("secrets.h")
#include "secrets.h"
  #define WIFI_SSID           WIFI_SSID_SECRET
  #define WIFI_PASSWORD       WIFI_PASSWORD_SECRET
#else
  // Default values (leave empty to disable WiFi)
  #define WIFI_SSID           ""          // Your WiFi SSID
  #define WIFI_PASSWORD       ""          // Your WiFi password
#endif

#define WEB_SERVER_PORT       80          // Web server port
#define WIFI_CONNECT_TIMEOUT  10000       // WiFi connection timeout in milliseconds

// Note: If WIFI_SSID is left empty, WiFi functionality is completely disabled

#endif // CONFIG_H
