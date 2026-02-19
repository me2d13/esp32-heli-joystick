#include <Arduino.h>
#include "config.h"
#include "logger.h"
#include "web_server.h"
#include "status_led.h"
#include "joystick.h"
#include "buttons.h"
#include "cyclic_serial.h"
#include "simulator_serial.h"
#include "collective.h"
#include "buzzer.h"
#include "steppers.h"
#include "ap.h"

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  delay(1000);
  
  // Initialize logger
  logger.begin(LOG_BUFFER_SIZE);
  
  LOG_INFO("=== ESP32 Heli Joystick ===");
  
  // Initialize USB HID Joystick first
  initJoystick();
  
  // Initialize button handling
  initButtons();
  
  // Initialize cyclic serial receiver (for AS5600 sensor data)
  initCyclicSerial();

  // Initialize simulator serial (JSON over UART)
  initSimulatorSerial();
  
  // Initialize collective axis (AS5600 sensor)
  initCollective();
  
  // Initialize buzzer (now on GPIO 21)
  initBuzzer();
  
  // Initialize stepper motors
  initSteppers();

  // Initialize autopilot
  initAP();

  // Initialize status LED
  initStatusLED();
  
  // Show startup status
  setLEDStatus(LED_STARTUP);
  delay(1000);
  
  // Initialize WiFi and web server
  initWebServer();
  
  // Set LED status based on WiFi state
  if (!isWiFiEnabled()) {
    setLEDStatus(LED_WIFI_DISABLED);
    LOG_INFO("LED: Red (WiFi Disabled)");
  } else if (isWiFiConnected()) {
    setLEDStatus(LED_WIFI_CONNECTED);
    LOG_INFO("LED: Green (WiFi Connected)");
  } else {
    setLEDStatus(LED_WIFI_FAILED);
    LOG_WARN("LED: Red (WiFi Connection Failed)");
  }
  
  LOG_INFO("=== System Ready ===");
  
  // Double beep to indicate system is ready
  doubleBeep();
}

void loop() {
  // Handle button scanning and updates
  handleButtons();
  
  // Process incoming cyclic sensor data and update joystick
  handleCyclicSerial();

  // Process incoming simulator data (JSON)
  handleSimulatorSerial();
  
  // Read collective axis and update joystick
  handleCollective();

  // Autopilot logic (overrides joystick when AP on)
  handleAP();

  // Handle stepper motor buttons (FTR toggles)
  handleSteppers();
  
  // Handle buzzer state machine (non-blocking beeps)
  handleBuzzer();
  
  // Handle web server and OTA
  handleWebServer();
  
  // Update LED animations (e.g., blinking)
  updateStatusLED();
  
  delay(10); // Small delay to prevent tight loop
}
