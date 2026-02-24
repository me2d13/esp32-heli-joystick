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

// Rainbow mode - smooth color cycling after system is stable
#define LED_RAINBOW_DELAY_MS 60000  // Time before rainbow starts (60 seconds)
#define LED_RAINBOW_SPEED 5         // Rainbow transition speed (1-10, higher = faster)

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

// array of button mappings. Defines which HW address maps to which button number
// 1-32 means joystick button number, 0 means no button
#define CYCLIC_BUTTONS_MAPPING {3, 0, 5, 0, 7, 0, 6, 0, 4, 0, 2, 0, 8, 0, 1, 0}

#define PIN_COL_DIR 4
#define PIN_COL_STEP 5
#define PIN_COL_BUTT_1 6
#define PIN_COL_BUTT_2 7
#define PIN_COL_FTR 15
#define PIN_COL_ENABLED 16
#define PIN_ADDR3 17
#define PIN_ADDR2 18
#define PIN_ADDR1 8
#define PIN_ADDR0 3
#define PIN_CYCLIC_BUTT 9
#define PIN_CYCLIC_FTR 10
#define PIN_COL_I2C_D 11
#define PIN_COL_I2C_C 12
#define PIN_CYCLIC_RX 13
#define PIN_CYCLIC_TX 14

#define PIN_CYCLIC_X_ENABLED 1
#define PIN_CYCLIC_X_STEP 2
#define PIN_CYCLIC_X_DIR 42
#define PIN_CYCLIC_Y_ENABLED 41
#define PIN_CYCLIC_Y_STEP 39
#define PIN_CYCLIC_Y_DIR 38

#define PIN_BUZZER 21

// ----------------------------------------------------------------------------
// Cyclic Axis Calibration
// ----------------------------------------------------------------------------
// These values define the sensor range for each axis.
// Sensor raw values are 0-4095, but the stick movement doesn't cover 
// the full range. These min/max values are used to map the sensor 
// readings to the joystick axis range (-127 to 127).
//
// To calibrate: Move the stick to its physical limits and note the 
// sensor values, then set these constants accordingly.

// Cyclic X axis (left/right) calibration
#define CYCLIC_X_SENSOR_MIN   00     // Sensor value at full left position
#define CYCLIC_X_SENSOR_MAX   1700   // Sensor value at full right position
#define CYCLIC_X_INVERT       true   // Set to true to invert axis direction

// Cyclic Y axis (forward/back) calibration
#define CYCLIC_Y_SENSOR_MIN   630    // Sensor value at full back position
#define CYCLIC_Y_SENSOR_MAX   2500   // Sensor value at full forward position
#define CYCLIC_Y_INVERT       false  // Set to true to invert axis direction

// Collective axis (up/down) calibration
// Note: This axis wraps around at the ADC boundary (0/4095)
// Physical range: 1370 (down) → 0 → 4095 → 1500 (up)
#define COLLECTIVE_SENSOR_MIN 1370   // Sensor value at full down position
#define COLLECTIVE_SENSOR_MAX 1500   // Sensor value at full up position
#define COLLECTIVE_INVERT     true   // Set to true to invert axis direction

// Serial protocol settings (for receiving data from AS5600 sensor board)
#define CYCLIC_SERIAL_BAUD    115200 // Baud rate for cyclic sensor data

// ----------------------------------------------------------------------------
// Simulator Serial (JSON over UART)
// ----------------------------------------------------------------------------
#define SIM_SERIAL_BAUD     115200  // Baud rate for simulator data
#define PIN_SIM_RX          43      // RX pin (connect to simulator TX)
#define PIN_SIM_TX          44      // TX pin (optional, simulator may be RX-only)

// ----------------------------------------------------------------------------
// Autopilot Configuration
// ----------------------------------------------------------------------------
#define SIMULATOR_VALID_TIMEOUT_MS  5000  // Simulator data considered valid for this long (ms)
#define AP_MIN_SPEED_KNOTS          10    // Min speed (knots) to enable or keep AP on

// PID gains for pitch hold (tune as needed)
#define AP_PITCH_KP                50.0f
#define AP_PITCH_KI                10.0f
#define AP_PITCH_KD                 0.0f

// PID gains for roll hold (tune as needed)
#define AP_ROLL_KP                 50.0f
#define AP_ROLL_KI                 10.0f
#define AP_ROLL_KD                  0.0f

/*
                                                                              
                            ┌─────────────────┐                               
                        ┌───└─────────────────┘───┐                           
                        │3V3                   GND│                           
                        │3v3                GPIO43│  free                     
                        │RST                GPIO44│  free                     
          COL-DIR       │GPIO4               GPIO1│  CYCLIC-X-ENABLED         
          COL-STEP      │GPIO5               GPIO2│  CYCLIC-X-STEP            
          COL-BUTT-1    │GPIO6              GPIO42│  CYCLIC-X-DIR             
          COL-BUTT-2    │GPIO7              GPIO41│  CYCLIC-Y-ENABLED         
          COL-FTR       │GPIO15             GPIO40│                           
          COL-ENABLED   │GPIO16             GPIO39│  CYCLIC-Y-STEP            
          ADDR3         │GPIO17             GPIO38│  CYCLIC-Y-DIR             
          ADDR2         │GPIO18             GPIO37│  PSRAM-RESERVED           
          ADDR1         │GPIO8              GPIO36│  PSRAM-RESERVED           
          ADDR0         │GPIO3              GPIO35│  PSRAM-RESERVED           
                      x │GPIO46              GPIO0│
          CYCLIC-BUTT   │GPIO9              GPIO45│                           
          CYCLIC-FTR    │GPIO10             GPIO48│                           
          COL-I2C-D     │GPIO11             GPIO47│
          COL-I2C-C     │GPIO12             GPIO21│  BUZZER
          CYCLIC-RX     │GPIO13             GPIO20│
          CYCLIC-TX     │GPIO14             GPIO19│                           
                        │5V0                   GND│                           
                        │GND    USB   UART     GND│                           
                        └───────┌──┐──┌──┐────────┘                           
                                └──┘  └──┘                                    
                                                                              
*/ 

// ----------------------------------------------------------------------------
// Logging Configuration
// ----------------------------------------------------------------------------
// Number of log messages to keep in memory for web interface display
// DEBUG level logs are not stored, only INFO, WARN, and ERROR
#define LOG_BUFFER_SIZE     50

#endif // CONFIG_H
