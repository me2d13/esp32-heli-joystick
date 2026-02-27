#include "profile.h"
#include "config.h"
#include "logger.h"

#define PROFILE_SLOW_MS 50  // Log when any task exceeds this

struct SlotInfo {
    const char* name;
    unsigned long lastMs;
    unsigned long maxMs;
    unsigned long startMs;
};

static SlotInfo slots[PROFILE_SLOT_COUNT] = {
    {"buttons", 0, 0, 0},
    {"cyclicSerial", 0, 0, 0},
    {"simulator", 0, 0, 0},
    {"collective", 0, 0, 0},
    {"ap", 0, 0, 0},
    {"steppers", 0, 0, 0},
    {"cyclicFeedback", 0, 0, 0},
    {"buzzer", 0, 0, 0},
    {"joystick", 0, 0, 0},
    {"statusLed", 0, 0, 0},
};

void initProfile() {
    for (int i = 0; i < PROFILE_SLOT_COUNT; i++) {
        slots[i].lastMs = 0;
        slots[i].maxMs = 0;
        slots[i].startMs = 0;
    }
}

void profileStart(uint8_t slot) {
    if (slot >= PROFILE_SLOT_COUNT) return;
    slots[slot].startMs = millis();
}

void profileEnd(uint8_t slot) {
    if (slot >= PROFILE_SLOT_COUNT) return;
    unsigned long d = millis() - slots[slot].startMs;
    slots[slot].lastMs = d;
    if (d > slots[slot].maxMs) {
        slots[slot].maxMs = d;
    }
    if (d > PROFILE_SLOW_MS) {
        LOG_WARNF("SLOW: %s took %lu ms", slots[slot].name, d);
    }
}

unsigned long profileGetLastMs(uint8_t slot) {
    return (slot < PROFILE_SLOT_COUNT) ? slots[slot].lastMs : 0;
}

unsigned long profileGetMaxMs(uint8_t slot) {
    return (slot < PROFILE_SLOT_COUNT) ? slots[slot].maxMs : 0;
}

const char* profileGetName(uint8_t slot) {
    return (slot < PROFILE_SLOT_COUNT) ? slots[slot].name : "";
}
