#include "steppers.h"
#include "config.h"
#include "logger.h"
#include "buzzer.h"

// Motor hold states
static bool collectiveHeld = false;
static bool cyclicHeld = false;

// Button state tracking for debouncing (collective FTR only)
static bool lastCollectiveFTRState = HIGH;
static unsigned long lastCollectiveDebounceTime = 0;
static const unsigned long DEBOUNCE_DELAY = 50;  // 50ms debounce

void initSteppers() {
    // Collective motor pins
    pinMode(PIN_COL_DIR, OUTPUT);
    pinMode(PIN_COL_STEP, OUTPUT);
    pinMode(PIN_COL_ENABLED, OUTPUT);
    digitalWrite(PIN_COL_DIR, LOW);
    digitalWrite(PIN_COL_STEP, LOW);
    digitalWrite(PIN_COL_ENABLED, HIGH);  // Active LOW - HIGH = disabled
    
    // Cyclic X motor pins
    pinMode(PIN_CYCLIC_X_DIR, OUTPUT);
    pinMode(PIN_CYCLIC_X_STEP, OUTPUT);
    pinMode(PIN_CYCLIC_X_ENABLED, OUTPUT);
    digitalWrite(PIN_CYCLIC_X_DIR, LOW);
    digitalWrite(PIN_CYCLIC_X_STEP, LOW);
    digitalWrite(PIN_CYCLIC_X_ENABLED, HIGH);  // Active LOW - HIGH = disabled
    
    // Cyclic Y motor pins
    pinMode(PIN_CYCLIC_Y_DIR, OUTPUT);
    pinMode(PIN_CYCLIC_Y_STEP, OUTPUT);
    pinMode(PIN_CYCLIC_Y_ENABLED, OUTPUT);
    digitalWrite(PIN_CYCLIC_Y_DIR, LOW);
    digitalWrite(PIN_CYCLIC_Y_STEP, LOW);
    digitalWrite(PIN_CYCLIC_Y_ENABLED, HIGH);  // Active LOW - HIGH = disabled
    
    // Collective FTR button pin - input with pullup
    pinMode(PIN_COL_FTR, INPUT_PULLUP);
    
    LOG_INFO("Stepper motors initialized");
    LOG_INFO("  Collective motor: GPIO4(DIR), GPIO5(STEP), GPIO16(EN)");
    LOG_INFOF("  Cyclic X motor: GPIO%d(DIR), GPIO%d(STEP), GPIO%d(EN)", PIN_CYCLIC_X_DIR, PIN_CYCLIC_X_STEP, PIN_CYCLIC_X_ENABLED);
    LOG_INFOF("  Cyclic Y motor: GPIO%d(DIR), GPIO%d(STEP), GPIO%d(EN)", PIN_CYCLIC_Y_DIR, PIN_CYCLIC_Y_STEP, PIN_CYCLIC_Y_ENABLED);
    LOG_INFO("  All motors disabled (free movement)");
    LOG_INFO("  Enable pins: Active LOW");
}

void toggleCollectiveHold() {
    collectiveHeld = !collectiveHeld;
    
    if (collectiveHeld) {
        // Enable motor (active LOW)
        digitalWrite(PIN_COL_ENABLED, LOW);
        LOG_INFO("Collective motor ENGAGED (holding position)");
        doubleBeep();  // Double beep on activation
    } else {
        // Disable motor (active LOW)
        digitalWrite(PIN_COL_ENABLED, HIGH);
        LOG_INFO("Collective motor RELEASED (free movement)");
        beep();  // Single beep on release
    }
}

void toggleCyclicHold() {
    cyclicHeld = !cyclicHeld;
    
    if (cyclicHeld) {
        // Enable both X and Y motors (active LOW)
        digitalWrite(PIN_CYCLIC_X_ENABLED, LOW);
        digitalWrite(PIN_CYCLIC_Y_ENABLED, LOW);
        LOG_INFO("Cyclic motors ENGAGED (holding position)");
        doubleBeep();  // Double beep on activation
    } else {
        // Disable both X and Y motors (active LOW)
        digitalWrite(PIN_CYCLIC_X_ENABLED, HIGH);
        digitalWrite(PIN_CYCLIC_Y_ENABLED, HIGH);
        LOG_INFO("Cyclic motors RELEASED (free movement)");
        beep();  // Single beep on release
    }
}

void handleSteppers() {
    unsigned long currentTime = millis();
    
    // Handle collective FTR button with debouncing
    bool collectiveFTRReading = digitalRead(PIN_COL_FTR);
    
    // Reset debounce timer if reading changed
    if (collectiveFTRReading != lastCollectiveFTRState) {
        lastCollectiveDebounceTime = currentTime;
        lastCollectiveFTRState = collectiveFTRReading;
    }
    
    // Check if button state has been stable long enough
    if ((currentTime - lastCollectiveDebounceTime) > DEBOUNCE_DELAY) {
        // Check for button press (transition from HIGH to LOW)
        static bool lastCollectiveStableState = HIGH;
        
        if (collectiveFTRReading == LOW && lastCollectiveStableState == HIGH) {
            // Button pressed (falling edge)
            toggleCollectiveHold();
            lastCollectiveStableState = LOW;
        } else if (collectiveFTRReading == HIGH && lastCollectiveStableState == LOW) {
            // Button released (rising edge)
            lastCollectiveStableState = HIGH;
        }
    }
}

bool isCollectiveHeld() {
    return collectiveHeld;
}

bool isCyclicHeld() {
    return cyclicHeld;
}

void stepCyclicX(bool dir) {
    digitalWrite(PIN_CYCLIC_X_DIR, dir ? HIGH : LOW);
    digitalWrite(PIN_CYCLIC_X_STEP, HIGH);
    delayMicroseconds(2);  // Minimal pulse width for stepper driver
    digitalWrite(PIN_CYCLIC_X_STEP, LOW);
}

void stepCyclicY(bool dir) {
    digitalWrite(PIN_CYCLIC_Y_DIR, dir ? HIGH : LOW);
    digitalWrite(PIN_CYCLIC_Y_STEP, HIGH);
    delayMicroseconds(2);
    digitalWrite(PIN_CYCLIC_Y_STEP, LOW);
}
