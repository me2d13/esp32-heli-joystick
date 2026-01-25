#include "web_server.h"
#include "config.h"
#include "logger.h"
#include "status_led.h"
#include "joystick.h"
#include "cyclic_serial.h"
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

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

// WebSocket update interval
#define WEBSOCKET_UPDATE_MS 50  // 20 Hz update rate

// Last WebSocket update time
unsigned long lastWebSocketUpdate = 0;

// HTML content for the main page
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Heli Joystick</title>
    <style>
        * {
            box-sizing: border-box;
        }
        body {
            font-family: 'Segoe UI', system-ui, -apple-system, sans-serif;
            margin: 0;
            padding: 20px;
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 50%, #0f3460 100%);
            min-height: 100vh;
            color: #e8e8e8;
        }
        .container {
            max-width: 900px;
            margin: 0 auto;
        }
        h1 {
            text-align: center;
            font-size: 2em;
            margin-bottom: 10px;
            color: #fff;
            text-shadow: 0 2px 10px rgba(0,0,0,0.3);
        }
        .subtitle {
            text-align: center;
            color: #8892b0;
            margin-bottom: 30px;
        }
        .card {
            background: rgba(255,255,255,0.05);
            backdrop-filter: blur(10px);
            border-radius: 16px;
            padding: 24px;
            margin-bottom: 20px;
            border: 1px solid rgba(255,255,255,0.1);
        }
        .card-title {
            font-size: 1.1em;
            font-weight: 600;
            margin-bottom: 20px;
            color: #64ffda;
            display: flex;
            align-items: center;
            gap: 10px;
        }
        .status-row {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 8px 0;
            border-bottom: 1px solid rgba(255,255,255,0.05);
        }
        .status-row:last-child {
            border-bottom: none;
        }
        .status-label {
            color: #8892b0;
        }
        .status-value {
            font-weight: 500;
        }
        .status-online {
            color: #64ffda;
        }
        .status-offline {
            color: #ff6b6b;
        }
        .connection-dot {
            width: 10px;
            height: 10px;
            border-radius: 50%;
            display: inline-block;
            margin-right: 8px;
            animation: pulse 2s infinite;
        }
        .connection-dot.connected {
            background: #64ffda;
            box-shadow: 0 0 10px #64ffda;
        }
        .connection-dot.disconnected {
            background: #ff6b6b;
            box-shadow: 0 0 10px #ff6b6b;
            animation: none;
        }
        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }
        .axis-container {
            margin-bottom: 16px;
        }
        .axis-header {
            display: flex;
            justify-content: space-between;
            margin-bottom: 8px;
        }
        .axis-name {
            font-weight: 500;
        }
        .axis-value {
            font-family: 'Consolas', 'Monaco', monospace;
            color: #64ffda;
            min-width: 50px;
            text-align: right;
        }
        .axis-bar-container {
            height: 24px;
            background: rgba(0,0,0,0.3);
            border-radius: 12px;
            overflow: hidden;
            position: relative;
        }
        .axis-bar {
            height: 100%;
            transition: width 0.05s ease-out, background 0.1s;
            border-radius: 12px;
        }
        .axis-bar.positive {
            background: linear-gradient(90deg, #64ffda, #00d4aa);
            margin-left: 50%;
        }
        .axis-bar.negative {
            background: linear-gradient(90deg, #ff6b6b, #ff8e8e);
            margin-left: auto;
            margin-right: 50%;
            transform-origin: right;
        }
        .axis-center-line {
            position: absolute;
            left: 50%;
            top: 0;
            bottom: 0;
            width: 2px;
            background: rgba(255,255,255,0.3);
            transform: translateX(-50%);
        }
        .raw-values {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 10px;
            margin-top: 16px;
            padding-top: 16px;
            border-top: 1px solid rgba(255,255,255,0.1);
        }
        .raw-value {
            font-family: 'Consolas', 'Monaco', monospace;
            font-size: 0.85em;
            color: #8892b0;
        }
        .buttons-grid {
            display: grid;
            grid-template-columns: repeat(8, 1fr);
            gap: 8px;
        }
        .button {
            aspect-ratio: 1;
            background: rgba(0,0,0,0.3);
            border-radius: 8px;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 0.75em;
            font-family: 'Consolas', 'Monaco', monospace;
            color: #8892b0;
            transition: all 0.1s;
            border: 1px solid rgba(255,255,255,0.05);
        }
        .button.pressed {
            background: linear-gradient(135deg, #64ffda, #00d4aa);
            color: #1a1a2e;
            font-weight: bold;
            box-shadow: 0 0 15px rgba(100, 255, 218, 0.5);
            transform: scale(0.95);
        }
        .footer {
            text-align: center;
            color: #5a6a8a;
            font-size: 0.85em;
            margin-top: 30px;
        }
        .logs-section {
            max-height: 400px;
            overflow-y: auto;
            background: rgba(0,0,0,0.2);
            border-radius: 8px;
            padding: 12px;
        }
        .log-entry {
            font-family: 'Consolas', 'Monaco', monospace;
            font-size: 0.85em;
            padding: 6px 10px;
            margin: 3px 0;
            border-radius: 4px;
            border-left: 3px solid;
            background: rgba(255,255,255,0.03);
        }
        .log-entry.info {
            border-left-color: #64ffda;
        }
        .log-entry.warn {
            border-left-color: #ffd700;
            background: rgba(255, 215, 0, 0.05);
        }
        .log-entry.error {
            border-left-color: #ff6b6b;
            background: rgba(255, 107, 107, 0.05);
        }
        .log-timestamp {
            color: #8892b0;
            margin-right: 10px;
        }
        .log-level {
            font-weight: bold;
            margin-right: 10px;
        }
        .log-level.info {
            color: #64ffda;
        }
        .log-level.warn {
            color: #ffd700;
        }
        .log-level.error {
            color: #ff6b6b;
        }
        .log-message {
            color: #e8e8e8;
        }
        @media (max-width: 600px) {
            .buttons-grid {
                grid-template-columns: repeat(4, 1fr);
            }
            body {
                padding: 10px;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🚁 ESP32 Heli Joystick</h1>
        <p class="subtitle">Real-time Controller Monitor</p>
        
        <div class="card">
            <div class="card-title">
                <span class="connection-dot" id="connectionDot"></span>
                System Status
            </div>
            <div class="status-row">
                <span class="status-label">WebSocket</span>
                <span class="status-value" id="wsStatus">Connecting...</span>
            </div>
            <div class="status-row">
                <span class="status-label">Cyclic Data</span>
                <span class="status-value" id="cyclicStatus">--</span>
            </div>
            <div class="status-row">
                <span class="status-label">Update Rate</span>
                <span class="status-value" id="updateRate">-- Hz</span>
            </div>
        </div>
        
        <div class="card">
            <div class="card-title">📊 Axes</div>
            
            <div class="axis-container">
                <div class="axis-header">
                    <span class="axis-name">Cyclic X (Left/Right)</span>
                    <span class="axis-value" id="axisX">0</span>
                </div>
                <div class="axis-bar-container">
                    <div class="axis-center-line"></div>
                    <div class="axis-bar" id="axisXBar"></div>
                </div>
            </div>
            
            <div class="axis-container">
                <div class="axis-header">
                    <span class="axis-name">Cyclic Y (Fwd/Back)</span>
                    <span class="axis-value" id="axisY">0</span>
                </div>
                <div class="axis-bar-container">
                    <div class="axis-center-line"></div>
                    <div class="axis-bar" id="axisYBar"></div>
                </div>
            </div>
            
            <div class="axis-container">
                <div class="axis-header">
                    <span class="axis-name">Collective (Up/Down)</span>
                    <span class="axis-value" id="axisZ">0</span>
                </div>
                <div class="axis-bar-container">
                    <div class="axis-center-line"></div>
                    <div class="axis-bar" id="axisZBar"></div>
                </div>
            </div>
            
            <div class="raw-values">
                <div class="raw-value">Raw X: <span id="rawX">--</span></div>
                <div class="raw-value">Raw Y: <span id="rawY">--</span></div>
            </div>
        </div>
        
        <div class="card">
            <div class="card-title">🔘 Buttons (32)</div>
            <div class="buttons-grid" id="buttonsGrid">
            </div>
        </div>
        
        <div class="card">
            <div class="card-title">📋 System Logs</div>
            <div class="logs-section" id="logsContainer">
                <div class="log-entry">Loading logs...</div>
            </div>
        </div>
        
        <p class="footer">
            WiFi: Connected • OTA: Ready<br>
            <a href="https://github.com/me2d13/esp32-heli-joystick" style="color: #64ffda;">GitHub</a>
        </p>
    </div>
    
    <script>
        let ws;
        let updateCount = 0;
        let lastSecond = Date.now();
        let currentHz = 0;
        
        // Create button grid
        const buttonsGrid = document.getElementById('buttonsGrid');
        for (let i = 0; i < 32; i++) {
            const btn = document.createElement('div');
            btn.className = 'button';
            btn.id = 'btn' + i;
            btn.textContent = i;
            buttonsGrid.appendChild(btn);
        }
        
        function updateAxisBar(barId, value) {
            const bar = document.getElementById(barId);
            const percent = Math.abs(value) / 127 * 50;
            
            bar.className = 'axis-bar';
            if (value >= 0) {
                bar.classList.add('positive');
                bar.style.width = percent + '%';
                bar.style.marginLeft = '50%';
                bar.style.marginRight = '';
            } else {
                bar.classList.add('negative');
                bar.style.width = percent + '%';
                bar.style.marginLeft = (50 - percent) + '%';
                bar.style.marginRight = '';
            }
        }
        
        function connect() {
            const host = window.location.hostname;
            ws = new WebSocket('ws://' + host + ':81');
            
            ws.onopen = function() {
                document.getElementById('wsStatus').textContent = 'Connected';
                document.getElementById('wsStatus').className = 'status-value status-online';
                document.getElementById('connectionDot').className = 'connection-dot connected';
            };
            
            ws.onclose = function() {
                document.getElementById('wsStatus').textContent = 'Disconnected';
                document.getElementById('wsStatus').className = 'status-value status-offline';
                document.getElementById('connectionDot').className = 'connection-dot disconnected';
                // Reconnect after 2 seconds
                setTimeout(connect, 2000);
            };
            
            ws.onerror = function(err) {
                console.error('WebSocket error:', err);
                ws.close();
            };
            
            ws.onmessage = function(event) {
                try {
                    const data = JSON.parse(event.data);
                    
                    // Update axes
                    document.getElementById('axisX').textContent = data.axes[0];
                    document.getElementById('axisY').textContent = data.axes[1];
                    document.getElementById('axisZ').textContent = data.axes[2];
                    
                    updateAxisBar('axisXBar', data.axes[0]);
                    updateAxisBar('axisYBar', data.axes[1]);
                    updateAxisBar('axisZBar', data.axes[2]);
                    
                    // Update raw values
                    document.getElementById('rawX').textContent = data.rawX;
                    document.getElementById('rawY').textContent = data.rawY;
                    
                    // Update cyclic status
                    const cyclicStatus = document.getElementById('cyclicStatus');
                    if (data.cyclicValid) {
                        cyclicStatus.textContent = 'Receiving';
                        cyclicStatus.className = 'status-value status-online';
                    } else {
                        cyclicStatus.textContent = 'No Data';
                        cyclicStatus.className = 'status-value status-offline';
                    }
                    
                    // Update buttons
                    for (let i = 0; i < 32; i++) {
                        const btn = document.getElementById('btn' + i);
                        const pressed = (data.buttons & (1 << i)) !== 0;
                        btn.className = pressed ? 'button pressed' : 'button';
                    }
                    
                    // Calculate update rate
                    updateCount++;
                    const now = Date.now();
                    if (now - lastSecond >= 1000) {
                        currentHz = updateCount;
                        updateCount = 0;
                        lastSecond = now;
                        document.getElementById('updateRate').textContent = currentHz + ' Hz';
                    }
                    
                } catch (e) {
                    console.error('Parse error:', e);
                }
            };
        }
        
        // Load and display system logs
        function loadLogs() {
            fetch('/logs')
                .then(response => response.json())
                .then(logs => {
                    const container = document.getElementById('logsContainer');
                    
                    if (logs.length === 0) {
                        container.innerHTML = '<div class="log-entry">No logs available</div>';
                        return;
                    }
                    
                    container.innerHTML = '';
                    
                    // Display logs in reverse order (newest first)
                    logs.reverse().forEach(log => {
                        const entry = document.createElement('div');
                        const levelClass = log.level.trim().toLowerCase();
                        entry.className = `log-entry ${levelClass}`;
                        
                        entry.innerHTML = `
                            <span class="log-timestamp">${log.timestamp}</span>
                            <span class="log-level ${levelClass}">[${log.level.trim()}]</span>
                            <span class="log-message">${log.message}</span>
                        `;
                        
                        container.appendChild(entry);
                    });
                })
                .catch(error => {
                    console.error('Error loading logs:', error);
                    document.getElementById('logsContainer').innerHTML = 
                        '<div class="log-entry error">Failed to load logs</div>';
                });
        }
        
        // Start connection and load logs
        connect();
        loadLogs();
    </script>
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

// WebSocket event handler
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            LOG_DEBUGF("[WS] Client #%u disconnected", num);
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket.remoteIP(num);
                LOG_DEBUGF("[WS] Client #%u connected from %s", num, ip.toString().c_str());
            }
            break;
        case WStype_TEXT:
            // Handle incoming text messages if needed
            break;
        default:
            break;
    }
}

// Send joystick state to all connected WebSocket clients
void broadcastJoystickState() {
    // Build JSON with joystick state
    StaticJsonDocument<256> doc;
    
    // Axes array
    JsonArray axes = doc.createNestedArray("axes");
    axes.add(getJoystickAxis(AXIS_CYCLIC_X));
    axes.add(getJoystickAxis(AXIS_CYCLIC_Y));
    axes.add(getJoystickAxis(AXIS_COLLECTIVE));
    
    // Raw sensor values
    doc["rawX"] = getCyclicXRaw();
    doc["rawY"] = getCyclicYRaw();
    doc["cyclicValid"] = isCyclicDataValid();
    
    // Buttons as bitmask
    uint32_t buttonMask = 0;
    for (int i = 0; i < 32; i++) {
        if (getJoystickButton(i)) {
            buttonMask |= (1UL << i);
        }
    }
    doc["buttons"] = buttonMask;
    
    // Serialize and send
    String json;
    serializeJson(doc, json);
    webSocket.broadcastTXT(json);
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

void handleWebServer() {
    if (isWiFiEnabled() && wifiConnected) {
        ArduinoOTA.handle();
        server.handleClient();
        webSocket.loop();
        
        // Broadcast joystick state periodically
        unsigned long now = millis();
        if (now - lastWebSocketUpdate >= WEBSOCKET_UPDATE_MS) {
            lastWebSocketUpdate = now;
            broadcastJoystickState();
        }
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
