#include "web_server.h"
#include "config.h"
#include "logger.h"
#include "status_led.h"
#include "joystick.h"
#include "cyclic_serial.h"
#include "collective.h"
#include "state.h"
#include "ap.h"

// Autopilot mode to string for JSON API
static const char* apHorizontalModeStr(APHorizontalMode m) {
    switch (m) {
        case APHorizontalMode::Off: return "off";
        case APHorizontalMode::RollHold: return "roll";
        case APHorizontalMode::HeadingHold: return "hdg";
        default: return "off";
    }
}
static const char* apVerticalModeStr(APVerticalMode m) {
    switch (m) {
        case APVerticalMode::Off: return "off";
        case APVerticalMode::PitchHold: return "pitch";
        case APVerticalMode::VerticalSpeed: return "vs";
        case APVerticalMode::AltitudeHold: return "alts";
        default: return "off";
    }
}
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>
#include <ESP.h>
#include <freertos/task.h>
#include "profile.h"

// Helper function to check if WiFi is enabled
bool isWiFiEnabled() {
    return strlen(WIFI_SSID) > 0;
}

// Web server instance
WebServer server(WEB_SERVER_PORT);

// WebSocket server on port 81
WebSocketsServer webSocket(81);

// WiFi connection status
bool wifiConnected = false;

// WebSocket update interval default
#define WEBSOCKET_UPDATE_MS 50  // 20 Hz update rate

#define MAX_WS_CLIENTS 8
struct WsClientState {
    bool active = false;
    unsigned long updateIntervalMs = WEBSOCKET_UPDATE_MS;
    unsigned long lastUpdateMs = 0;
};
WsClientState wsClients[MAX_WS_CLIENTS];

// Get MIME type from file extension
static const char* getContentType(const char* path) {
    if (strstr(path, ".html")) return "text/html";
    if (strstr(path, ".css"))  return "text/css";
    if (strstr(path, ".js"))   return "application/javascript";
    if (strstr(path, ".json")) return "application/json";
    if (strstr(path, ".ico"))  return "image/x-icon";
    if (strstr(path, ".png"))  return "image/png";
    if (strstr(path, ".svg"))  return "image/svg+xml";
    return "text/plain";
}

// Serve static file from LittleFS
static bool serveStaticFile(const char* path) {
    File file = LittleFS.open(path, "r");
    if (!file || file.isDirectory()) {
        if (file) file.close();
        return false;
    }
    server.streamFile(file, getContentType(path));
    file.close();
    return true;
}

// Handle root page - serve index.html
static void handleRoot() {
    if (!serveStaticFile("/index.html")) {
        server.send(500, "text/plain", "Failed to load index.html. Upload filesystem with: pio run -t uploadfs");
    }
}

// Handle static files and 404
static void handleNotFound() {
    String uri = server.uri();
    // Try to serve from LittleFS (e.g. /styles.css, /app.js)
    if (uri.length() > 1) {
        if (serveStaticFile(uri.c_str())) {
            return;
        }
    }

    String message = "File Not Found\n\n";
    message += "URI: ";
    message += uri;
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

// WebSocket event handler
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (num >= MAX_WS_CLIENTS) return;

    switch(type) {
        case WStype_DISCONNECTED:
            LOG_DEBUGF("[WS] Client #%u disconnected", num);
            wsClients[num].active = false;
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket.remoteIP(num);
                LOG_DEBUGF("[WS] Client #%u connected from %s", num, ip.toString().c_str());
                wsClients[num].active = true;
                wsClients[num].updateIntervalMs = WEBSOCKET_UPDATE_MS;
                wsClients[num].lastUpdateMs = millis();
                
                String uri = (char*)payload;
                int rateIdx = uri.indexOf("rate=");
                if (rateIdx != -1) {
                    int rate = uri.substring(rateIdx + 5).toInt();
                    if (rate >= 10) {
                        wsClients[num].updateIntervalMs = rate;
                    }
                    LOG_DEBUGF("[WS] Client #%u requested rate %u ms", num, wsClients[num].updateIntervalMs);
                }
            }
            break;
        case WStype_TEXT:
            {
                // Optional: handle updates via JSON text message too
                StaticJsonDocument<128> doc;
                DeserializationError err = deserializeJson(doc, payload);
                if (!err && doc.containsKey("setUpdateInterval")) {
                    wsClients[num].updateIntervalMs = doc["setUpdateInterval"].as<unsigned long>();
                    LOG_DEBUGF("[WS] Client #%u set interval to %u ms", num, wsClients[num].updateIntervalMs);
                }
            }
            break;
        default:
            break;
    }
}

// Build complete state as JSON (for API and WebSocket)
static void buildStateJson(JsonDocument& doc) {
    // Update cyclic validity (computed from last packet time)
    (void)isCyclicDataValid();

    JsonObject sensors = doc.createNestedObject("sensors");
    sensors["cyclicX"] = state.sensors.cyclicXCalibrated;
    sensors["cyclicY"] = state.sensors.cyclicYCalibrated;
    sensors["collective"] = state.sensors.collectiveCalibrated;
    sensors["cyclicValid"] = state.sensors.cyclicValid;
    sensors["rawX"] = state.sensors.cyclicXRaw;
    sensors["rawY"] = state.sensors.cyclicYRaw;
    sensors["rawZ"] = state.sensors.collectiveRaw;

    JsonObject joystick = doc.createNestedObject("joystick");
    joystick["cyclicX"] = state.joystick.cyclicX;
    joystick["cyclicY"] = state.joystick.cyclicY;
    joystick["collective"] = state.joystick.collective;
    joystick["buttons"] = state.joystick.buttons;

    JsonObject autopilot = doc.createNestedObject("autopilot");
    autopilot["enabled"] = state.autopilot.enabled;
    autopilot["horizontalMode"] = apHorizontalModeStr(state.autopilot.horizontalMode);
    autopilot["verticalMode"] = apVerticalModeStr(state.autopilot.verticalMode);
    autopilot["selectedHeading"] = state.autopilot.selectedHeading;
    autopilot["selectedAltitude"] = state.autopilot.selectedAltitude;
    autopilot["capturedAltitude"] = state.autopilot.capturedAltitude;
    autopilot["selectedVerticalSpeed"] = state.autopilot.selectedVerticalSpeed;
    autopilot["hasSelectedAltitude"] = state.autopilot.hasSelectedAltitude;
    autopilot["hasSelectedVerticalSpeed"] = state.autopilot.hasSelectedVerticalSpeed;
    autopilot["altHoldArmed"] = state.autopilot.altHoldArmed;
    autopilot["selectedPitch"] = state.autopilot.selectedPitch;
    autopilot["selectedRoll"] = state.autopilot.selectedRoll;
    autopilot["pitchKp"] = state.autopilot.pitchKp;
    autopilot["pitchKi"] = state.autopilot.pitchKi;
    autopilot["pitchKd"] = state.autopilot.pitchKd;
    autopilot["rollKp"] = state.autopilot.rollKp;
    autopilot["rollKi"] = state.autopilot.rollKi;
    autopilot["rollKd"] = state.autopilot.rollKd;
    autopilot["headingKp"] = state.autopilot.headingKp;
    autopilot["vsKp"] = state.autopilot.vsKp;

    JsonObject simulator = doc.createNestedObject("simulator");
    simulator["speed"] = state.simulator.speed;
    simulator["altitude"] = state.simulator.altitude;
    simulator["pitch"] = state.simulator.pitch;
    simulator["roll"] = state.simulator.roll;
    simulator["heading"] = state.simulator.heading;
    simulator["verticalSpeed"] = state.simulator.verticalSpeed;
    simulator["valid"] = state.simulator.valid;
    
    long age = -1;
    if (state.simulator.lastUpdateMs > 0) {
        age = millis() - state.simulator.lastUpdateMs;
    }
    simulator["lastSimDataAgeMs"] = age;

    doc["telemetryEnabled"] = state.telemetryEnabled;
    doc["cyclicFeedbackEnabled"] = state.cyclicFeedbackEnabled;
    if (state.telemetryEnabled) {
        char buf[256];
        // CSV: ms,ap,hMode,vMode,pitch,roll,hdg,vs,spd,sel_p,sel_r,sel_hdg,sel_vs,outY,outX
        snprintf(buf, sizeof(buf), "%lu,%d,%s,%s,%.2f,%.2f,%.1f,%.1f,%.1f,%.2f,%.2f,%.1f,%.1f,%d,%d",
            millis(),
            state.autopilot.enabled ? 1 : 0,
            apHorizontalModeStr(state.autopilot.horizontalMode),
            apVerticalModeStr(state.autopilot.verticalMode),
            state.simulator.pitch,
            state.simulator.roll,
            state.simulator.heading,
            state.simulator.verticalSpeed,
            state.simulator.speed,
            state.autopilot.selectedPitch,
            state.autopilot.selectedRoll,
            state.autopilot.selectedHeading,
            state.autopilot.selectedVerticalSpeed,
            state.joystick.cyclicY,
            state.joystick.cyclicX
        );
        doc["telemetry"] = buf;
    }

    JsonObject debug = doc.createNestedObject("motorDebug");
    debug["active"] = state.motorDebugActive;
    debug["stepsX"] = state.debugMotorXSteps;
    debug["stepsY"] = state.debugMotorYSteps;
}

// Send complete state only to clients that are due for an update
void updateWebSocketClients() {
    unsigned long now = millis();
    bool needBuild = false;
    
    for (int i = 0; i < MAX_WS_CLIENTS; i++) {
        if (wsClients[i].active && (now - wsClients[i].lastUpdateMs >= wsClients[i].updateIntervalMs)) {
            needBuild = true;
            break;
        }
    }
    
    if (needBuild) {
        StaticJsonDocument<1024> doc;
        buildStateJson(doc);
        String json;
        serializeJson(doc, json);
        
        for (int i = 0; i < MAX_WS_CLIENTS; i++) {
            if (wsClients[i].active && (now - wsClients[i].lastUpdateMs >= wsClients[i].updateIntervalMs)) {
                wsClients[i].lastUpdateMs = now;
                webSocket.sendTXT(i, json);
            }
        }
    }
}

void initWebServer() {
    if (isWiFiEnabled()) {
        LOG_INFO("=== WiFi Configuration ===");
        LOG_INFOF("SSID: %s", WIFI_SSID);
        
        // Set WiFi mode to station
        WiFi.mode(WIFI_STA);
        
        // Set hostname for DHCP
        WiFi.setHostname("esp32-heli-joy");
        
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        
        // Set LED to connecting status (blinking yellow)
        setLEDStatus(LED_WIFI_CONNECTING);
        
        LOG_INFO("Connecting to WiFi...");
        unsigned long startAttemptTime = millis();
        
        // Wait for connection with timeout
        while (WiFi.status() != WL_CONNECTED && 
               millis() - startAttemptTime < WIFI_CONNECT_TIMEOUT) {
            delay(100);
            updateStatusLED(); // Update LED animation
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            wifiConnected = true;
            LOG_INFO("WiFi connected!");
            LOG_INFOF("IP Address: %s", WiFi.localIP().toString().c_str());

            // Initialize LittleFS for static file serving
            if (!LittleFS.begin(true)) {
                LOG_ERROR("LittleFS mount failed! Web UI may not work. Run: pio run -t uploadfs");
            } else {
                LOG_INFO("LittleFS mounted");
            }

            // Initialize mDNS
            if (MDNS.begin("esp32-heli")) {
                LOG_INFO("mDNS responder started: esp32-heli.local");
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
                LOG_INFO("Start OTA updating " + type);
            });
            
            ArduinoOTA.onEnd([]() {
                LOG_INFO("OTA update complete");
            });
            
            ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
                // Use DEBUG to avoid flooding log buffer
                LOG_DEBUGF("OTA Progress: %u%%", (progress / (total / 100)));
            });
            
            ArduinoOTA.onError([](ota_error_t error) {
                if (error == OTA_AUTH_ERROR) {
                    LOG_ERROR("OTA Error: Auth Failed");
                } else if (error == OTA_BEGIN_ERROR) {
                    LOG_ERROR("OTA Error: Begin Failed");
                } else if (error == OTA_CONNECT_ERROR) {
                    LOG_ERROR("OTA Error: Connect Failed");
                } else if (error == OTA_RECEIVE_ERROR) {
                    LOG_ERROR("OTA Error: Receive Failed");
                } else if (error == OTA_END_ERROR) {
                    LOG_ERROR("OTA Error: End Failed");
                }
            });
            
            ArduinoOTA.begin();
            LOG_INFO("OTA ready");
            
            // Setup web server routes
            server.on("/", handleRoot);
            server.on("/debug", []() {
                if (!serveStaticFile("/debug.html")) {
                    server.send(500, "text/plain", "Failed to load debug.html. Upload filesystem with: pio run -t uploadfs");
                }
            });
            server.on("/ap", []() {
                if (!serveStaticFile("/ap.html")) {
                    server.send(500, "text/plain", "Failed to load ap.html. Upload filesystem with: pio run -t uploadfs");
                }
            });
            server.on("/api/debug", []() {
                StaticJsonDocument<768> doc;
                doc["uptimeMs"] = millis();
                doc["freeHeap"] = ESP.getFreeHeap();
                doc["minFreeHeap"] = ESP.getMinFreeHeap();
                doc["heapSize"] = ESP.getHeapSize();
                doc["chipModel"] = ESP.getChipModel();
                doc["chipRevision"] = ESP.getChipRevision();
                doc["cpuFreqMHz"] = ESP.getCpuFreqMHz();
                TaskHandle_t t = xTaskGetCurrentTaskHandle();
                UBaseType_t stackLeft = (t != NULL) ? uxTaskGetStackHighWaterMark(t) : 0;
                doc["stackHighWaterMark"] = stackLeft * 4;  // words to bytes
                JsonArray tasks = doc.createNestedArray("loopTasks");
                for (uint8_t i = 0; i < PROFILE_SLOT_COUNT; i++) {
                    JsonObject o = tasks.createNestedObject();
                    o["name"] = profileGetName(i);
                    o["lastMs"] = profileGetLastMs(i);
                    o["maxMs"] = profileGetMaxMs(i);
                }
                String json;
                serializeJson(doc, json);
                server.send(200, "application/json", json);
            });
            server.on("/api/state", []() {
                StaticJsonDocument<1024> doc;
                buildStateJson(doc);
                String json;
                serializeJson(doc, json);
                server.send(200, "application/json", json);
            });
            server.on("/api/autopilot/alt_arm", HTTP_POST, []() {
                if (server.hasArg("armed")) {
                    state.autopilot.altHoldArmed = (server.arg("armed") == "true");
                    server.send(200, "application/json", "{\"status\":\"ok\"}");
                } else {
                    server.send(400, "application/json", "{\"error\":\"missing param 'armed'\"}");
                }
            });

            server.on("/api/autopilot/selected_pitch", HTTP_POST, []() {
                if (server.hasArg("plain")) {
                    String body = server.arg("plain");
                    StaticJsonDocument<128> doc;
                    DeserializationError err = deserializeJson(doc, body);
                    if (!err && doc.containsKey("selectedPitch")) {
                        state.autopilot.selectedPitch = doc["selectedPitch"].as<float>();
                        server.send(200, "application/json", "{\"status\":\"ok\"}");
                    } else {
                        server.send(400, "application/json", "{\"error\":\"Invalid JSON or missing selectedPitch\"}");
                    }
                } else {
                    server.send(400, "application/json", "{\"error\":\"Missing body\"}");
                }
            });
            server.on("/api/autopilot", HTTP_POST, []() {
                if (!server.hasArg("plain")) {
                    server.send(400, "application/json", "{\"error\":\"JSON body required\"}");
                    return;
                }
                String body = server.arg("plain");
                StaticJsonDocument<256> doc;
                DeserializationError err = deserializeJson(doc, body);
                if (err) {
                    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                    return;
                }
                if (doc.containsKey("enabled")) {
                    setAPEnabled(doc["enabled"].as<bool>());
                }
                if (doc.containsKey("horizontalMode")) {
                    String hMode = doc["horizontalMode"].as<String>();
                    if (hMode == "off") setAPHorizontalMode(APHorizontalMode::Off);
                    else if (hMode == "roll") setAPHorizontalMode(APHorizontalMode::RollHold);
                    else if (hMode == "hdg") setAPHorizontalMode(APHorizontalMode::HeadingHold);
                }
                if (doc.containsKey("verticalMode")) {
                    String vMode = doc["verticalMode"].as<String>();
                    if (vMode == "off") setAPVerticalMode(APVerticalMode::Off);
                    else if (vMode == "pitch") setAPVerticalMode(APVerticalMode::PitchHold);
                    else if (vMode == "vs") setAPVerticalMode(APVerticalMode::VerticalSpeed);
                    else if (vMode == "alts") setAPVerticalMode(APVerticalMode::AltitudeHold);
                }
                if (doc.containsKey("selectedHeading")) {
                    state.autopilot.selectedHeading = doc["selectedHeading"].as<float>();
                    state.autopilot.hasSelectedHeading = true;
                }
                if (doc.containsKey("selectedPitch")) {
                    state.autopilot.selectedPitch = doc["selectedPitch"].as<float>();
                }
                if (doc.containsKey("selectedRoll")) {
                    state.autopilot.selectedRoll = doc["selectedRoll"].as<float>();
                }
                if (doc.containsKey("selectedVerticalSpeed")) {
                    state.autopilot.selectedVerticalSpeed = doc["selectedVerticalSpeed"].as<float>();
                    state.autopilot.hasSelectedVerticalSpeed = true;
                }
                if (doc.containsKey("selectedAltitude")) {
                    state.autopilot.selectedAltitude = doc["selectedAltitude"].as<float>();
                    state.autopilot.hasSelectedAltitude = true;
                }
                // Return updated state
                StaticJsonDocument<512> stateDoc;
                buildStateJson(stateDoc);
                String json;
                serializeJson(stateDoc, json);
                server.send(200, "application/json", json);
            });
            server.on("/api/pid", HTTP_POST, []() {
                if (!server.hasArg("plain")) {
                    server.send(400, "application/json", "{\"error\":\"JSON body required\"}");
                    return;
                }
                String body = server.arg("plain");
                StaticJsonDocument<256> doc;
                DeserializationError err = deserializeJson(doc, body);
                if (err) {
                    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                    return;
                }
                
                bool changed = false;
                if (doc.containsKey("pitchKp")) {
                    state.autopilot.pitchKp = doc["pitchKp"].as<float>();
                    changed = true;
                }
                if (doc.containsKey("pitchKi")) {
                    state.autopilot.pitchKi = doc["pitchKi"].as<float>();
                    changed = true;
                }
                if (doc.containsKey("pitchKd")) {
                    state.autopilot.pitchKd = doc["pitchKd"].as<float>();
                    changed = true;
                }
                if (doc.containsKey("rollKp")) {
                    state.autopilot.rollKp = doc["rollKp"].as<float>();
                    changed = true;
                }
                if (doc.containsKey("rollKi")) {
                    state.autopilot.rollKi = doc["rollKi"].as<float>();
                    changed = true;
                }
                if (doc.containsKey("rollKd")) {
                    state.autopilot.rollKd = doc["rollKd"].as<float>();
                    changed = true;
                }
                if (doc.containsKey("headingKp")) {
                    state.autopilot.headingKp = doc["headingKp"].as<float>();
                    changed = true;
                }
                if (doc.containsKey("vsKp")) {
                    state.autopilot.vsKp = doc["vsKp"].as<float>();
                    changed = true;
                }
                
                if (changed) {
                    syncAPPidTunings();
                    LOG_INFOF("PID Pitch updated: P:%.2f I:%.2f D:%.2f", 
                               state.autopilot.pitchKp, state.autopilot.pitchKi, state.autopilot.pitchKd);
                    LOG_INFOF("PID Roll updated:  P:%.2f I:%.2f D:%.2f", 
                               state.autopilot.rollKp, state.autopilot.rollKi, state.autopilot.rollKd);
                }

                // Return updated state
                StaticJsonDocument<512> stateDoc;
                buildStateJson(stateDoc);
                String json;
                serializeJson(stateDoc, json);
                server.send(200, "application/json", json);
            });
            server.on("/api/cyclic_feedback", HTTP_POST, []() {
                if (!server.hasArg("plain")) {
                    server.send(400, "application/json", "{\"error\":\"JSON body required\"}");
                    return;
                }
                String body = server.arg("plain");
                StaticJsonDocument<128> doc;
                deserializeJson(doc, body);
                if (doc.containsKey("enabled")) {
                    state.cyclicFeedbackEnabled = doc["enabled"].as<bool>();
                    LOG_INFOF("Cyclic feedback: %s", state.cyclicFeedbackEnabled ? "ON" : "OFF");
                }
                server.send(200, "application/json", state.cyclicFeedbackEnabled ? "{\"enabled\":true}" : "{\"enabled\":false}");
            });
            server.on("/api/motor_debug", HTTP_POST, []() {
                if (!server.hasArg("plain")) {
                    server.send(400, "application/json", "{\"error\":\"JSON body required\"}");
                    return;
                }
                String body = server.arg("plain");
                StaticJsonDocument<128> doc;
                deserializeJson(doc, body);
                if (doc.containsKey("active")) {
                    state.motorDebugActive = doc["active"].as<bool>();
                    if (state.motorDebugActive) {
                        state.cyclicFeedbackEnabled = false; // Disable to not interfere
                        state.debugMotorXSteps = 0;
                        state.debugMotorYSteps = 0;
                        LOG_INFO("Motor Debug Active: Cyclic FFB disabled.");
                    } else {
                        LOG_INFO("Motor Debug Inactive.");
                    }
                }
                if (doc.containsKey("stepsX")) {
                    int steps = doc["stepsX"].as<int>();
                    state.debugMotorXSteps = steps * CYCLIC_MICROSTEPPING;
                    LOG_INFOF("Motor Debug: Move X %d steps (%d microsteps)", steps, state.debugMotorXSteps);
                }
                if (doc.containsKey("stepsY")) {
                    int steps = doc["stepsY"].as<int>();
                    state.debugMotorYSteps = steps * CYCLIC_MICROSTEPPING;
                    LOG_INFOF("Motor Debug: Move Y %d steps (%d microsteps)", steps, state.debugMotorYSteps);
                }
                server.send(200, "application/json", "{\"status\":\"ok\"}");
            });
            server.on("/api/telemetry", HTTP_POST, []() {
                if (!server.hasArg("plain")) {
                    server.send(400, "application/json", "{\"error\":\"JSON body required\"}");
                    return;
                }
                String body = server.arg("plain");
                StaticJsonDocument<128> doc;
                deserializeJson(doc, body);
                if (doc.containsKey("enabled")) {
                    state.telemetryEnabled = doc["enabled"].as<bool>();
                    LOG_INFOF("Telemetry recording: %s", state.telemetryEnabled ? "ON" : "OFF");
                }
                server.send(200, "application/json", state.telemetryEnabled ? "{\"enabled\":true}" : "{\"enabled\":false}");
            });
            server.on("/logs", []() {
                String logsJSON = logger.getEntriesJSON();
                server.send(200, "application/json", logsJSON);
            });
            server.onNotFound(handleNotFound);
            
            // Start web server
            server.begin();
            LOG_INFOF("Web server started on port %d", WEB_SERVER_PORT);
            
            // Start WebSocket server
            webSocket.begin();
            webSocket.onEvent(webSocketEvent);
            LOG_INFO("WebSocket server started on port 81");
            
            LOG_INFOF("Visit: http://%s/", WiFi.localIP().toString().c_str());
            
        } else {
            wifiConnected = false;
            setLEDStatus(LED_WIFI_FAILED);
            LOG_ERROR("WiFi connection failed!");
            LOG_ERRORF("WiFi status: %d", WiFi.status());
            LOG_WARN("Continuing without WiFi...");
        }
    } else {
        LOG_INFO("=== WiFi Disabled ===");
        LOG_INFO("WiFi SSID not configured. Running without WiFi.");
    }
}

static void webTask(void* arg) {
    (void)arg;
    for (;;) {
        handleWebServer();
        vTaskDelay(1);  // Yield to other tasks
    }
}

void startWebServerTask() {
    if (!isWiFiEnabled()) return;
    xTaskCreatePinnedToCore(webTask, "web", 4096, NULL, 0, NULL, 0);
    LOG_INFO("Web server task started (Core 0, low priority)");
}

void handleWebServer() {
    if (isWiFiEnabled() && wifiConnected) {
        ArduinoOTA.handle();
        server.handleClient();
        webSocket.loop();
        
        // Broadcast joystick state to ready clients
        updateWebSocketClients();
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
