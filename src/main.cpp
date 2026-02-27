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
#include "cyclic_feedback.h"
#include "profile.h"

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  delay(1000);
  
  // Initialize logger
  logger.begin(LOG_BUFFER_SIZE);
  initProfile();
  
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

  // Initialize cyclic feedback (steppers chase joystick when AP + cyclic held)
  initCyclicFeedback();

  // Initialize status LED
  initStatusLED();
  
  // Show startup status
  setLEDStatus(LED_STARTUP);
  delay(1000);
  
  // Initialize WiFi and web server
  initWebServer();
  startWebServerTask();  // Run web/OTA in background task (low priority)
  
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
  static unsigned long lastHeartbeat = 0;
  unsigned long now = millis();

  profileStart(PROFILE_BUTTONS);
  handleButtons();
  profileEnd(PROFILE_BUTTONS);

  profileStart(PROFILE_CYCLIC_SERIAL);
  handleCyclicSerial();
  profileEnd(PROFILE_CYCLIC_SERIAL);

  profileStart(PROFILE_SIMULATOR);
  handleSimulatorSerial();
  profileEnd(PROFILE_SIMULATOR);

  profileStart(PROFILE_COLLECTIVE);
  handleCollective();
  profileEnd(PROFILE_COLLECTIVE);

  profileStart(PROFILE_AP);
  handleAP();
  profileEnd(PROFILE_AP);

  profileStart(PROFILE_STEPPERS);
  handleSteppers();
  profileEnd(PROFILE_STEPPERS);

  profileStart(PROFILE_CYCLIC_FEEDBACK);
  handleCyclicFeedback();
  profileEnd(PROFILE_CYCLIC_FEEDBACK);

  profileStart(PROFILE_BUZZER);
  handleBuzzer();
  profileEnd(PROFILE_BUZZER);

  profileStart(PROFILE_JOYSTICK);
  updateJoystick();
  profileEnd(PROFILE_JOYSTICK);

  profileStart(PROFILE_STATUS_LED);
  updateStatusLED();
  profileEnd(PROFILE_STATUS_LED);

  if (now - lastHeartbeat >= 2000) {
    lastHeartbeat = now;
    LOG_DEBUGF("Heartbeat: %lu ms", now);
  }
  
  delay(10);
}
