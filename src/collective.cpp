#include "collective.h"
#include "config.h"
#include "joystick.h"
#include "logger.h"

// Last raw ADC reading
static uint16_t collectiveRaw = 0;

// Last mapped axis value
static int16_t collectiveAxis = 5000;

void initCollective() {
    // Configure the analog input pin
    pinMode(PIN_COL_I2C_D, INPUT);
    
    LOG_INFO("Collective axis initialized");
    LOG_INFOF("  Analog Pin: GPIO%d", PIN_COL_I2C_D);
    LOG_INFOF("  Calibration: %d - %d (wraps at 0/4095)", COLLECTIVE_SENSOR_MIN, COLLECTIVE_SENSOR_MAX);
    LOG_INFOF("  Inverted: %s", COLLECTIVE_INVERT ? "true" : "false");
}

void handleCollective() {
    // Read analog value from pin
    // ESP32 ADC: 12-bit resolution (0-4095) for 0-3.3V range
    collectiveRaw = analogRead(PIN_COL_I2C_D);
    
    // Handle overflow: The axis wraps around at the ADC boundary
    // Physical range: 1370 (down) → 4095 → 0 → 1500 (up)
    // We need to "unwrap" this into a continuous range
    
    uint16_t sensorValue = collectiveRaw;
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
    
    collectiveAxis = (int16_t)mapped;
    
    // Apply inversion if needed
    if (COLLECTIVE_INVERT) {
        collectiveAxis = AXIS_MAX - (collectiveAxis - AXIS_MIN);
    }
    
    // Update the joystick collective axis (Z axis)
    setJoystickAxis(AXIS_COLLECTIVE, collectiveAxis);
    updateJoystick();
}

uint16_t getCollectiveRaw() {
    return collectiveRaw;
}

int16_t getCollectiveAxis() {
    return collectiveAxis;
}
