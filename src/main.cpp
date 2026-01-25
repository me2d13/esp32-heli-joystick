#include <Arduino.h>
#include "config.h"
#include "logger.h"
#include "web_server.h"
#include "status_led.h"
#include "joystick.h"
#include "buttons.h"
#include "cyclic_serial.h"

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
}

void loop() {
  // Handle button scanning and updates
  handleButtons();
  
  // Process incoming cyclic sensor data and update joystick
  handleCyclicSerial();
  
  // Handle web server and OTA
  handleWebServer();
  
  // Update LED animations (e.g., blinking)
  updateStatusLED();
  
  delay(10); // Small delay to prevent tight loop
}
