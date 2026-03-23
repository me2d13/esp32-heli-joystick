#include "buttons.h"
#include "config.h"
#include "joystick.h"
#include "logger.h"
#include "steppers.h"
#include "ap.h"
#include "state.h"
// Button state tracking
static bool cyclicButtonStates[16] = {false};
static bool collectiveButt1States[16] = {false};
static bool collectiveButt2States[16] = {false};
static bool collectiveFtrButtonState = false;

static uint8_t cyclicButtonMappings[16] = CYCLIC_BUTTONS_MAPPING;

// Collective multiplexed button mappings (button number, 0 = not wired)
// Positive = normal logic, negative = inverted logic (mounted upside-down etc.)
// Butt1/addr2 → button 12 (inverted), Butt1/addr11 → button 10, Butt1/addr15 → button 11
static int8_t collectiveButt1Mappings[16] = {18, 13, -12, 23, 19, 21, 16, 20, 17, 22, 0, 10, 15, 14, 0, 11};
static int8_t collectiveButt2Mappings[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0};

// Built-in action mapping: index = HID button number (1-indexed), value = ButtonAction.
// Buttons not listed here (or ACTION_NONE) only produce HID output with no side-effect.
static ButtonAction buttonActionMappings[33] = {
  ACTION_NONE,               //  0  (unused, buttons are 1-indexed)
  ACTION_NONE,               //  1
  ACTION_TOGGLE_AP,          //  2
  ACTION_TOGGLE_CYCLIC_HOLD, //  3
  ACTION_NONE,               //  4
  ACTION_NONE,               //  5
  ACTION_NONE,               //  6
  ACTION_NONE,               //  7
  ACTION_NONE,               //  8
  ACTION_TOGGLE_COLLECTIVE_HOLD, //  9 (collective FTR button)
  // 10-32: ACTION_NONE (zero-initialised)
};

void initButtons() {
  LOG_INFO("Initializing button handling...");
  
  // Set address pins as OUTPUT
  pinMode(PIN_ADDR0, OUTPUT);
  pinMode(PIN_ADDR1, OUTPUT);
  pinMode(PIN_ADDR2, OUTPUT);
  pinMode(PIN_ADDR3, OUTPUT);
  
  // Initialize address pins to 0
  digitalWrite(PIN_ADDR0, LOW);
  digitalWrite(PIN_ADDR1, LOW);
  digitalWrite(PIN_ADDR2, LOW);
  digitalWrite(PIN_ADDR3, LOW);
  
  // Set button signal pins as INPUT_PULLUP (active LOW)
  pinMode(PIN_CYCLIC_BUTT, INPUT_PULLUP);
  pinMode(PIN_COL_BUTT_1, INPUT_PULLUP);
  pinMode(PIN_COL_BUTT_2, INPUT_PULLUP);
  pinMode(PIN_COL_FTR, INPUT_PULLUP);  // Direct collective button
  
  LOG_INFO("Button handling initialized");
  LOG_INFO("  - Address pins: shared across all multiplexers");
  LOG_INFO("  - Signal pins: 3 (1 cyclic, 2 collective)");
}

void setMultiplexerAddress(uint8_t address) {
  // Set the 4-bit address on the shared address pins
  digitalWrite(PIN_ADDR0, (address & 0x01) ? HIGH : LOW);
  digitalWrite(PIN_ADDR1, (address & 0x02) ? HIGH : LOW);
  digitalWrite(PIN_ADDR2, (address & 0x04) ? HIGH : LOW);
  digitalWrite(PIN_ADDR3, (address & 0x08) ? HIGH : LOW);
  
  // Small delay to allow multiplexer to settle
  delayMicroseconds(10);
}

// Dispatches built-in side-effects for a button. Called after every HID button
// event, for all button sources (cyclic, collective, FTR).
// Only acts on press unless the action also needs release (none do, currently).
static void performButtonAction(uint8_t buttonNumber, bool pressed) {
  if (buttonNumber == 0 || buttonNumber > 32) return;
  ButtonAction action = buttonActionMappings[buttonNumber];
  if (action == ACTION_NONE) return;

  switch (action) {
    case ACTION_TOGGLE_CYCLIC_HOLD:
      if (pressed) toggleCyclicHold();
      break;
    case ACTION_TOGGLE_COLLECTIVE_HOLD:
      if (pressed) toggleCollectiveHold();
      break;
    case ACTION_TOGGLE_AP:
      if (pressed) setAPEnabled(!state.autopilot.enabled);
      break;
    default:
      break;
  }
}

void handleButtons() {
  bool dirty = false;
  // Scan through all 16 possible addresses (0-15)
  for (uint8_t addr = 0; addr < 16; addr++) {
    // Set the multiplexer address
    setMultiplexerAddress(addr);
    
    // Only read cyclic buttons for addresses 0-7 (as only 8 buttons are wired)
    if (cyclicButtonMappings[addr] != 0) {
      // Read the cyclic button signal (active LOW, so invert)
      bool buttonPressed = !digitalRead(PIN_CYCLIC_BUTT);
      
      // Check if state has changed
      if (buttonPressed != cyclicButtonStates[addr]) {
        cyclicButtonStates[addr] = buttonPressed;
        
        // Update joystick button (buttons are 1-indexed in HID)
        uint8_t buttonNumber = cyclicButtonMappings[addr]; // Button 1-8
        setJoystickButton(buttonNumber - 1, buttonPressed);
        dirty = true;
        
        // Dispatch any built-in side-effect assigned to this button
        performButtonAction(buttonNumber, buttonPressed);
        
        // Use DEBUG level to avoid flooding the log buffer (this is in loop)
        LOG_DEBUGF("Cyclic Button %d (addr %d): %s", 
                   buttonNumber, addr, buttonPressed ? "PRESSED" : "RELEASED");
      }
    }
    
    // Read collective button signal 1
    bool col1Pressed = !digitalRead(PIN_COL_BUTT_1);
    if (col1Pressed != collectiveButt1States[addr]) {
      collectiveButt1States[addr] = col1Pressed;
      if (collectiveButt1Mappings[addr] != 0) {
        int8_t  mapping      = collectiveButt1Mappings[addr];
        uint8_t buttonNumber = (uint8_t)abs(mapping);
        bool    joySent      = (mapping > 0) ? col1Pressed : !col1Pressed; // invert when negative
        setJoystickButton(buttonNumber - 1, joySent);
        dirty = true;
        performButtonAction(buttonNumber, joySent);
        LOG_DEBUGF("Collective Butt1 Button %d (addr %d)%s: %s",
                   buttonNumber, addr, mapping < 0 ? " [INV]" : "",
                   joySent ? "PRESSED" : "RELEASED");
      } else {
        LOG_INFOF("Collective Butt1 signal (addr %d): %s", addr, col1Pressed ? "PRESSED" : "RELEASED");
      }
    }

    // Read collective button signal 2
    bool col2Pressed = !digitalRead(PIN_COL_BUTT_2);
    if (col2Pressed != collectiveButt2States[addr]) {
      collectiveButt2States[addr] = col2Pressed;
      if (collectiveButt2Mappings[addr] != 0) {
        int8_t  mapping      = collectiveButt2Mappings[addr];
        uint8_t buttonNumber = (uint8_t)abs(mapping);
        bool    joySent      = (mapping > 0) ? col2Pressed : !col2Pressed; // invert when negative
        setJoystickButton(buttonNumber - 1, joySent);
        dirty = true;
        performButtonAction(buttonNumber, joySent);
        LOG_DEBUGF("Collective Butt2 Button %d (addr %d)%s: %s",
                   buttonNumber, addr, mapping < 0 ? " [INV]" : "",
                   joySent ? "PRESSED" : "RELEASED");
      } else {
        LOG_INFOF("Collective Butt2 signal (addr %d): %s", addr, col2Pressed ? "PRESSED" : "RELEASED");
      }
    }
  }
  
  // Handle directly-wired collective button (PIN_COL_FTR) - mapped as button 9
  bool collectiveFtrPressed = !digitalRead(PIN_COL_FTR);
  if (collectiveFtrPressed != collectiveFtrButtonState) {
    collectiveFtrButtonState = collectiveFtrPressed;
    
    // Update joystick button 9 (index 8, as buttons are 0-indexed in HID)
    setJoystickButton(8, collectiveFtrPressed);
    dirty = true;
    performButtonAction(9, collectiveFtrPressed);
    
    // Use DEBUG level to avoid flooding the log buffer (this is in loop)
    LOG_DEBUGF("Collective FTR Button 9: %s", 
               collectiveFtrPressed ? "PRESSED" : "RELEASED");
  }
}
