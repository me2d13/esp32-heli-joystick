#include "simulator_serial.h"
#include "config.h"
#include "state.h"
#include "logger.h"
#include <ArduinoJson.h>

// Use the standard Serial (UART0) for simulator data as it's hardwired to the CH340 COM port
// Note: With CDC_ON_BOOT=0, Serial is UART0 on GPIO 43/44.
#define SimSerial Serial

// Line buffer for JSON messages
#define SIM_LINE_BUF_SIZE 256
static char lineBuf[SIM_LINE_BUF_SIZE];
static size_t lineLen = 0;

void initSimulatorSerial() {
    // Serial is already initialized in main.cpp, but ensure baud rate matches
    SimSerial.begin(SIM_SERIAL_BAUD);

    LOG_INFO("Simulator serial initialized (using Serial UART0)");
    LOG_INFOF("  Baud rate: %d", SIM_SERIAL_BAUD);
}

static void processLine(const char* line) {
    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, line);

    if (err) {
        LOG_DEBUGF("Simulator JSON parse error: %s", err.c_str());
        return;
    }

    unsigned long now = millis();
    state.simulator.lastUpdateMs = now;
    state.simulator.valid = true;
    state.simulator.dataUpdated = true;

    if (doc.containsKey("spd")) {
        state.simulator.speed = doc["spd"].as<float>();
    }
    if (doc.containsKey("alt")) {
        state.simulator.altitude = doc["alt"].as<float>();
    }
    if (doc.containsKey("pitch")) {
        state.simulator.pitch = doc["pitch"].as<float>();
    }
    if (doc.containsKey("roll")) {
        state.simulator.roll = doc["roll"].as<float>();
    }
    if (doc.containsKey("hdg")) {
        state.simulator.heading = doc["hdg"].as<float>();
    }
    if (doc.containsKey("vs")) {
        state.simulator.verticalSpeed = doc["vs"].as<float>();
    }
}

void handleSimulatorSerial() {
    while (SimSerial.available() > 0) {
        char c = SimSerial.read();

        if (c == '\n' || c == '\r') {
            if (lineLen > 0) {
                lineBuf[lineLen] = '\0';
                processLine(lineBuf);
                lineLen = 0;
            }
            if (c == '\r' && SimSerial.available() && SimSerial.peek() == '\n') {
                SimSerial.read();  // consume \n after \r
            }
        } else if (lineLen < SIM_LINE_BUF_SIZE - 1) {
            lineBuf[lineLen++] = c;
        } else {
            // Buffer overflow, discard line
            lineLen = 0;
            while (SimSerial.available() && SimSerial.peek() != '\n' && SimSerial.peek() != '\r') {
                SimSerial.read();
            }
        }
    }
}
