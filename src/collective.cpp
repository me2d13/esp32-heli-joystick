#include "collective.h"
#include "config.h"
#include "joystick.h"
#include "logger.h"
#include "state.h"
#include <Wire.h>
#include <AS5600.h>

// AS5600 sensor instance using I2C bus 1 (to avoid USB conflicts)
static AS5600 collectiveSensor(&Wire1);

// Sensor connection status
static bool sensorConnected = false;

// Timing for sensor reading (20Hz = 50ms interval)
static unsigned long lastReadTime = 0;
static const unsigned long READ_INTERVAL_MS = 50;

void initCollective() {
    // Initialize I2C bus 1 with custom pins for collective sensor
    // Using Wire1 (I2C1) instead of Wire (I2C0) to avoid USB peripheral conflicts
    // SDA = PIN_COL_I2C_D, SCL = PIN_COL_I2C_C
    Wire1.begin(PIN_COL_I2C_D, PIN_COL_I2C_C);
    
    // Set I2C clock speed to 100kHz (standard mode) for stability
    Wire1.setClock(100000);
    
    // Initialize AS5600 sensor
    collectiveSensor.begin();
    
    // Check if sensor is connected and store the status
    sensorConnected = collectiveSensor.isConnected();
    
    LOG_INFO("Collective axis initialized");
    LOG_INFOF("  I2C Bus: Wire1 (I2C1, avoids USB conflicts)");
    LOG_INFOF("  I2C SDA Pin: GPIO%d", PIN_COL_I2C_D);
    LOG_INFOF("  I2C SCL Pin: GPIO%d", PIN_COL_I2C_C);
    LOG_INFOF("  I2C Clock: 100 kHz");
    LOG_INFOF("  AS5600 Sensor: %s", sensorConnected ? "Connected" : "NOT FOUND");
    LOG_INFOF("  Update Rate: 20 Hz (50ms interval)");
    LOG_INFOF("  Calibration: %d - %d (wraps at 0/4095)", COLLECTIVE_SENSOR_MIN, COLLECTIVE_SENSOR_MAX);
    LOG_INFOF("  Inverted: %s", COLLECTIVE_INVERT ? "true" : "false");
    
    if (!sensorConnected) {
        LOG_WARN("AS5600 sensor not detected on I2C bus! Collective axis will not be updated.");
    }
}

void handleCollective() {
    // Skip processing if sensor is not connected
    if (!sensorConnected) {
        return;
    }
    
    // Limit reading frequency to 20Hz (every 50ms)
    unsigned long currentTime = millis();
    if (currentTime - lastReadTime < READ_INTERVAL_MS) {
        return;
    }
    lastReadTime = currentTime;
    
    // Read raw angle from AS5600 sensor
    // AS5600 returns 12-bit value (0-4095) representing 0-360 degrees
    state.sensors.collectiveRaw = collectiveSensor.rawAngle();
    
    // Handle overflow: The axis wraps around at the ADC boundary
    // Physical range: 1370 (down) → 4095 → 0 → 1500 (up)
    // We need to "unwrap" this into a continuous range
    
    uint16_t sensorValue = state.sensors.collectiveRaw;
    uint16_t sensorMin = COLLECTIVE_SENSOR_MIN;  // 1370
    uint16_t sensorMax = COLLECTIVE_SENSOR_MAX;  // 1500
    
    // Detect overflow: if max < min, OR if the range is suspiciously small (< 200)
    // indicating it wraps around the 0/4095 boundary
    // In our case: 1500 - 1370 = 130, which is small, so this wraps around
    bool hasOverflow = (sensorMax < sensorMin) || ((sensorMax - sensorMin) < 200 && sensorMax < 2000);
    
    int32_t normalizedValue;
    int32_t normalizedMin;
    int32_t normalizedMax;
    
    if (hasOverflow) {
        // Overflow case: range wraps around 0/4095
        // Physical path: sensorMin → 4095 → 0 → sensorMax
        // Convert to continuous range by adding 4096 to wrapped values
        
        if (sensorValue <= sensorMax) {
            // Value has wrapped around (it's in the 0 to sensorMax range)
            normalizedValue = sensorValue + 4096;
        } else {
            // Value hasn't wrapped yet (it's in the sensorMin to 4095 range)
            normalizedValue = sensorValue;
        }
        
        normalizedMin = (int32_t)sensorMin;
        normalizedMax = (int32_t)sensorMax + 4096;
        
    } else {
        // Normal case: no overflow, direct mapping
        normalizedValue = (int32_t)sensorValue;
        normalizedMin = (int32_t)sensorMin;
        normalizedMax = (int32_t)sensorMax;
    }
    
    // Clamp to calibration range
    if (normalizedValue < normalizedMin) {
        normalizedValue = normalizedMin;
    }
    if (normalizedValue > normalizedMax) {
        normalizedValue = normalizedMax;
    }
    
    // Map from normalized range to joystick axis range (0 to 10000)
    int32_t inputOffset = normalizedValue - normalizedMin;
    int32_t inputRange = normalizedMax - normalizedMin;
    int32_t outputRange = AXIS_MAX - AXIS_MIN;  // 10000
    
    // Multiply first, then divide to preserve precision
    long mapped = ((long)inputOffset * (long)outputRange) / (long)inputRange + AXIS_MIN;
    
    state.sensors.collectiveCalibrated = (int16_t)mapped;
    
    // Apply inversion if needed
    if (COLLECTIVE_INVERT) {
        state.sensors.collectiveCalibrated = AXIS_MAX - (state.sensors.collectiveCalibrated - AXIS_MIN);
    }
    
    // Update the joystick collective axis (Z axis)
    setJoystickAxis(AXIS_COLLECTIVE, state.sensors.collectiveCalibrated);
    updateJoystick();
}

uint16_t getCollectiveRaw() {
    return state.sensors.collectiveRaw;
}

int16_t getCollectiveAxis() {
    return state.sensors.collectiveCalibrated;
}
