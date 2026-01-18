#include <Arduino.h>
#include "config.h"
#include "web_server.h"
#include "status_led.h"
#include "joystick.h"
#include "buttons.h"

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n=== ESP32 Heli Joystick ===");
  
  // Initialize USB HID Joystick first
  initJoystick();
  
  // Initialize button handling
  initButtons();
  
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
    Serial.println("LED: Red (WiFi Disabled)");
  } else if (isWiFiConnected()) {
    setLEDStatus(LED_WIFI_CONNECTED);
    Serial.println("LED: Green (WiFi Connected)");
  } else {
    setLEDStatus(LED_WIFI_FAILED);
    Serial.println("LED: Red (WiFi Connection Failed)");
  }
  
  Serial.println("\n=== System Ready ===\n");
}

void loop() {
  // Handle button scanning and updates
  handleButtons();
  
  // Update joystick with demo animation
  updateJoystickDemo();
  
  // Handle web server and OTA
  handleWebServer();
  
  // Update LED animations (e.g., blinking)
  updateStatusLED();
  
  delay(10); // Small delay to prevent tight loop
}
