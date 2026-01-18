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

#define NUMBER_OF_CYCLIC_BUTTONS 8

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
          COL-I2C-C     │GPIO12             GPIO21│
          CYCLIC-RX     │GPIO13             GPIO20│
          CYCLIC-TX     │GPIO14             GPIO19│                           
                        │5V0                   GND│                           
                        │GND    USB   UART     GND│                           
                        └───────┌──┐──┌──┐────────┘                           
                                └──┘  └──┘                                    
                                                                              
*/ 

#endif // CONFIG_H
