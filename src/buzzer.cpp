#include "buzzer.h"
#include "config.h"
#include "logger.h"

// Buzzer state machine
enum BuzzerState {
    BUZZER_IDLE,
    BUZZER_ON,
    BUZZER_GAP
};

// Beep sequence structure
struct BeepSequence {
    uint8_t beepsRemaining;  // Number of beeps left to play
    uint16_t beepDuration;   // Duration of each beep
    uint16_t gapDuration;    // Duration of gap between beeps
};

static BuzzerState buzzerState = BUZZER_IDLE;
static BeepSequence currentSequence = {0, 0, 0};
static unsigned long stateStartTime = 0;

void initBuzzer() {
    // Configure buzzer pin as output
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);
    
    LOG_INFO("Buzzer initialized");
    LOG_INFOF("  Pin: GPIO%d", PIN_BUZZER);
    LOG_INFO("  Type: Active buzzer (3.3V)");
    LOG_INFO("  Mode: Non-blocking");
}

void handleBuzzer() {
    if (buzzerState == BUZZER_IDLE) {
        return;  // Nothing to do
    }
    
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - stateStartTime;
    
    switch (buzzerState) {
        case BUZZER_ON:
            // Check if beep duration has elapsed
            if (elapsed >= currentSequence.beepDuration) {
                // Turn off buzzer
                digitalWrite(PIN_BUZZER, LOW);
                currentSequence.beepsRemaining--;
                
                if (currentSequence.beepsRemaining > 0) {
                    // More beeps to play - enter gap state
                    buzzerState = BUZZER_GAP;
                    stateStartTime = currentTime;
                } else {
                    // Sequence complete
                    buzzerState = BUZZER_IDLE;
                }
            }
            break;
            
        case BUZZER_GAP:
            // Check if gap duration has elapsed
            if (elapsed >= currentSequence.gapDuration) {
                // Start next beep
                digitalWrite(PIN_BUZZER, HIGH);
                buzzerState = BUZZER_ON;
                stateStartTime = currentTime;
            }
            break;
            
        case BUZZER_IDLE:
        default:
            // Should not reach here
            break;
    }
}

void beep(uint16_t duration) {
    // Set up single beep sequence
    currentSequence.beepsRemaining = 1;
    currentSequence.beepDuration = duration;
    currentSequence.gapDuration = 0;  // Not used for single beep
    
    // Start beep
    digitalWrite(PIN_BUZZER, HIGH);
    buzzerState = BUZZER_ON;
    stateStartTime = millis();
}

void doubleBeep(uint16_t duration, uint16_t gap) {
    // Set up double beep sequence
    currentSequence.beepsRemaining = 2;
    currentSequence.beepDuration = duration;
    currentSequence.gapDuration = gap;
    
    // Start first beep
    digitalWrite(PIN_BUZZER, HIGH);
    buzzerState = BUZZER_ON;
    stateStartTime = millis();
}

void tripleBeep(uint16_t duration, uint16_t gap) {
    // Set up triple beep sequence
    currentSequence.beepsRemaining = 3;
    currentSequence.beepDuration = duration;
    currentSequence.gapDuration = gap;
    
    // Start first beep
    digitalWrite(PIN_BUZZER, HIGH);
    buzzerState = BUZZER_ON;
    stateStartTime = millis();
}
