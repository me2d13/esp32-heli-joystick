#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>

// Initialize WiFi and web server (if WiFi credentials are configured)
void initWebServer();

// Start web server task (runs handleWebServer in background at low priority)
// Call after initWebServer(). When used, do NOT call handleWebServer() from main loop.
void startWebServerTask();

// Handle web server requests (called from web task, or from loop if task not started)
void handleWebServer();

// Get WiFi connection status
bool isWiFiConnected();

// Check if WiFi is enabled in configuration
bool isWiFiEnabled();

// Get IP address as string
String getIPAddress();

#endif // WEB_SERVER_H
