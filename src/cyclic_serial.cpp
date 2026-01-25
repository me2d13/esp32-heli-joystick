#include "cyclic_serial.h"
#include "config.h"
#include "joystick.h"
#include "logger.h"

// Use Serial1 for cyclic data (separate from USB debug Serial)
HardwareSerial CyclicSerial(1);

// Receive buffer for building packets
static uint8_t rxBuffer[PACKET_SIZE];
static uint8_t rxIndex = 0;

// Last valid sensor values
static uint16_t cyclicXRaw = 0;
static uint16_t cyclicYRaw = 0;

// Timestamp of last valid packet
static unsigned long lastValidPacketTime = 0;

// Data validity timeout (milliseconds)
#define DATA_VALID_TIMEOUT 500

// Forward declarations
static bool validatePacket(const uint8_t* packet);
static void processPacket(const uint8_t* packet);
static int16_t mapSensorToAxis(uint16_t sensorValue, uint16_t sensorMin, uint16_t sensorMax, bool invert);

void initCyclicSerial() {
    // Initialize Serial1 with custom pins
    CyclicSerial.begin(CYCLIC_SERIAL_BAUD, SERIAL_8N1, PIN_CYCLIC_RX, PIN_CYCLIC_TX);
    
    LOG_INFO("Cyclic serial receiver initialized");
    LOG_INFOF("  RX Pin: GPIO%d", PIN_CYCLIC_RX);
    LOG_INFOF("  Baud rate: %d", CYCLIC_SERIAL_BAUD);
    LOG_INFOF("  X calibration: %d - %d", CYCLIC_X_SENSOR_MIN, CYCLIC_X_SENSOR_MAX);
    LOG_INFOF("  Y calibration: %d - %d", CYCLIC_Y_SENSOR_MIN, CYCLIC_Y_SENSOR_MAX);
}

void handleCyclicSerial() {
    // Read all available bytes
    while (CyclicSerial.available() > 0) {
        uint8_t byte = CyclicSerial.read();
        
        // If we're starting fresh, look for start marker
        if (rxIndex == 0) {
            if (byte == PACKET_START_MARKER) {
                rxBuffer[rxIndex++] = byte;
            }
            // Otherwise ignore the byte (out of sync)
            continue;
        }
        
        // Add byte to buffer
        rxBuffer[rxIndex++] = byte;
        
        // Check if we have a complete packet
        if (rxIndex >= PACKET_SIZE) {
            // Validate and process the packet
            if (validatePacket(rxBuffer)) {
                processPacket(rxBuffer);
            }
            // Reset for next packet
            rxIndex = 0;
        }
    }
}

static bool validatePacket(const uint8_t* packet) {
    // Check start marker
    if (packet[0] != PACKET_START_MARKER) {
        return false;
    }
    
    // Check end marker
    if (packet[6] != PACKET_END_MARKER) {
        // Bad end marker - we're probably out of sync
        // Try to find start marker in the buffer and shift
        for (int i = 1; i < PACKET_SIZE; i++) {
            if (packet[i] == PACKET_START_MARKER) {
                // Shift buffer to realign
                memmove(rxBuffer, &packet[i], PACKET_SIZE - i);
                rxIndex = PACKET_SIZE - i;
                return false;
            }
        }
        return false;
    }
    
    // Calculate checksum (XOR of angle bytes)
    uint8_t checksum = packet[1] ^ packet[2] ^ packet[3] ^ packet[4];
    
    if (checksum != packet[5]) {
        // Checksum mismatch
        return false;
    }
    
    return true;
}

static void processPacket(const uint8_t* packet) {
    // Extract sensor values (little-endian uint16_t)
    uint16_t sensor1 = packet[1] | (packet[2] << 8);
    uint16_t sensor2 = packet[3] | (packet[4] << 8);
    
    // Store raw values
    cyclicXRaw = sensor1;
    cyclicYRaw = sensor2;
    
    // Update timestamp
    lastValidPacketTime = millis();
    
    // Map sensor values to joystick axis range
    int16_t axisX = mapSensorToAxis(sensor1, CYCLIC_X_SENSOR_MIN, CYCLIC_X_SENSOR_MAX, CYCLIC_X_INVERT);
    int16_t axisY = mapSensorToAxis(sensor2, CYCLIC_Y_SENSOR_MIN, CYCLIC_Y_SENSOR_MAX, CYCLIC_Y_INVERT);
    
    // Update joystick
    setJoystickAxis(AXIS_CYCLIC_X, axisX);
    setJoystickAxis(AXIS_CYCLIC_Y, axisY);
    updateJoystick();
}

static int16_t mapSensorToAxis(uint16_t sensorValue, uint16_t sensorMin, uint16_t sensorMax, bool invert) {
    // Clamp sensor value to calibration range
    if (sensorValue < sensorMin) {
        sensorValue = sensorMin;
    }
    if (sensorValue > sensorMax) {
        sensorValue = sensorMax;
    }
    
    // Map from sensor range to joystick axis range (0 to 10000)
    // Using long to avoid overflow during calculation
    long mapped = (long)(sensorValue - sensorMin) * (AXIS_MAX - AXIS_MIN) / (sensorMax - sensorMin) + AXIS_MIN;
    
    // Apply inversion if needed
    if (invert) {
        mapped = AXIS_MAX - (mapped - AXIS_MIN);
    }
    
    return (int16_t)mapped;
}

uint16_t getCyclicXRaw() {
    return cyclicXRaw;
}

uint16_t getCyclicYRaw() {
    return cyclicYRaw;
}

bool isCyclicDataValid() {
    if (lastValidPacketTime == 0) {
        return false;
    }
    return (millis() - lastValidPacketTime) < DATA_VALID_TIMEOUT;
}

unsigned long getCyclicDataAge() {
    if (lastValidPacketTime == 0) {
        return ULONG_MAX;
    }
    return millis() - lastValidPacketTime;
}
