#include "web_server.h"
#include "config.h"
#include "status_led.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

// Helper function to check if WiFi is enabled
bool isWiFiEnabled() {
    return strlen(WIFI_SSID) > 0;
}

// Web server instance
WebServer server(WEB_SERVER_PORT);

// WiFi connection status
bool wifiConnected = false;

// HTML content for the main page
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Heli Joystick</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 50px auto;
            padding: 20px;
            background-color: #f0f0f0;
        }
        .container {
            background-color: white;
            border-radius: 10px;
            padding: 30px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        h1 {
            color: #333;
            text-align: center;
        }
        .info {
            background-color: #e8f4f8;
            border-left: 4px solid #2196F3;
            padding: 15px;
            margin: 20px 0;
        }
        .status {
            display: flex;
            justify-content: space-between;
            margin: 10px 0;
        }
        .label {
            font-weight: bold;
            color: #555;
        }
        .value {
            color: #2196F3;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🚁 ESP32 Heli Joystick</h1>
        <div class="info">
            <div class="status">
                <span class="label">Status:</span>
                <span class="value">Online</span>
            </div>
            <div class="status">
                <span class="label">WiFi:</span>
                <span class="value">Connected</span>
            </div>
            <div class="status">
                <span class="label">OTA:</span>
                <span class="value">Ready</span>
            </div>
        </div>
        <p style="text-align: center; color: #666; margin-top: 30px;">
            More features coming soon...
        </p>
    </div>
</body>
</html>
)rawliteral";

// Handle root page request
void handleRoot() {
    server.send(200, "text/html", htmlPage);
}

// Handle 404 errors
void handleNotFound() {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    
    for (uint8_t i = 0; i < server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    
    server.send(404, "text/plain", message);
}

void initWebServer() {
    if (isWiFiEnabled()) {
        Serial.println("\n=== WiFi Configuration ===");
        Serial.print("SSID: ");
        Serial.println(WIFI_SSID);
        
        // Set WiFi mode to station
        WiFi.mode(WIFI_STA);
        
        // Set hostname for DHCP
        WiFi.setHostname("esp32-heli-joy");
        
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        
        // Set LED to connecting status (blinking yellow)
        setLEDStatus(LED_WIFI_CONNECTING);
        
        Serial.print("Connecting to WiFi");
        unsigned long startAttemptTime = millis();
        
        // Wait for connection with timeout
        while (WiFi.status() != WL_CONNECTED && 
               millis() - startAttemptTime < WIFI_CONNECT_TIMEOUT) {
            delay(100);
            updateStatusLED(); // Update LED animation
            if ((millis() - startAttemptTime) % 500 == 0) {
                Serial.print(".");
            }
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            wifiConnected = true;
            Serial.println("\nWiFi connected!");
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
            
            // Initialize mDNS
            if (MDNS.begin("esp32-heli")) {
                Serial.println("mDNS responder started: esp32-heli.local");
            }
            
            // Configure OTA
            ArduinoOTA.setHostname("esp32-heli");
            ArduinoOTA.setPassword("admin"); // Change this to a secure password
            
            ArduinoOTA.onStart([]() {
                String type;
                if (ArduinoOTA.getCommand() == U_FLASH) {
                    type = "sketch";
                } else {
                    type = "filesystem";
                }
                Serial.println("Start updating " + type);
            });
            
            ArduinoOTA.onEnd([]() {
                Serial.println("\nEnd");
            });
            
            ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
                Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
            });
            
            ArduinoOTA.onError([](ota_error_t error) {
                Serial.printf("Error[%u]: ", error);
                if (error == OTA_AUTH_ERROR) {
                    Serial.println("Auth Failed");
                } else if (error == OTA_BEGIN_ERROR) {
                    Serial.println("Begin Failed");
                } else if (error == OTA_CONNECT_ERROR) {
                    Serial.println("Connect Failed");
                } else if (error == OTA_RECEIVE_ERROR) {
                    Serial.println("Receive Failed");
                } else if (error == OTA_END_ERROR) {
                    Serial.println("End Failed");
                }
            });
            
            ArduinoOTA.begin();
            Serial.println("OTA ready");
            
            // Setup web server routes
            server.on("/", handleRoot);
            server.onNotFound(handleNotFound);
            
            // Start web server
            server.begin();
            Serial.print("Web server started on port ");
            Serial.println(WEB_SERVER_PORT);
            Serial.print("Visit: http://");
            Serial.print(WiFi.localIP());
            Serial.println("/");
            
        } else {
            wifiConnected = false;
            setLEDStatus(LED_WIFI_FAILED);
            Serial.println("\nWiFi connection failed!");
            Serial.print("WiFi status: ");
            Serial.println(WiFi.status());
            Serial.println("Continuing without WiFi...");
        }
    } else {
        Serial.println("\n=== WiFi Disabled ===");
        Serial.println("WiFi SSID not configured. Running without WiFi.");
    }
}

void handleWebServer() {
    if (isWiFiEnabled() && wifiConnected) {
        ArduinoOTA.handle();
        server.handleClient();
    }
}

bool isWiFiConnected() {
    return wifiConnected;
}

String getIPAddress() {
    if (isWiFiEnabled() && wifiConnected) {
        return WiFi.localIP().toString();
    }
    return "Not connected";
}
