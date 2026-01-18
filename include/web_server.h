#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>

// Initialize WiFi and web server (if WiFi credentials are configured)
void initWebServer();

// Handle web server requests (call this in loop)
void handleWebServer();

// Get WiFi connection status
bool isWiFiConnected();

// Check if WiFi is enabled in configuration
bool isWiFiEnabled();

// Get IP address as string
String getIPAddress();

#endif // WEB_SERVER_H
