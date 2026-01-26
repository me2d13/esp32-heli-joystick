#include "buttons.h"
#include "config.h"
#include "joystick.h"
#include "logger.h"

// Button state tracking
static bool cyclicButtonStates[16] = {false};
static bool collectiveFtrButtonState = false;

static uint8_t cyclicButtonMappings[16] = CYCLIC_BUTTONS_MAPPING;

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
        
        // Use DEBUG level to avoid flooding the log buffer (this is in loop)
        LOG_DEBUGF("Cyclic Button %d (addr %d): %s", 
                   buttonNumber, addr, buttonPressed ? "PRESSED" : "RELEASED");
      }
    }
    
    // TODO: Add collective button handling here when wired
    // For now, we only handle cyclic buttons
    
    // Read collective button 1 signal
    // bool col1Pressed = !digitalRead(PIN_COL_BUTT_1);
    
    // Read collective button 2 signal
    // bool col2Pressed = !digitalRead(PIN_COL_BUTT_2);
  }
  
  // Handle directly-wired collective button (PIN_COL_FTR) - mapped as button 9
  bool collectiveFtrPressed = !digitalRead(PIN_COL_FTR);
  if (collectiveFtrPressed != collectiveFtrButtonState) {
    collectiveFtrButtonState = collectiveFtrPressed;
    
    // Update joystick button 9 (index 8, as buttons are 0-indexed in HID)
    setJoystickButton(8, collectiveFtrPressed);
    dirty = true;
    
    // Use DEBUG level to avoid flooding the log buffer (this is in loop)
    LOG_DEBUGF("Collective FTR Button 9: %s", 
               collectiveFtrPressed ? "PRESSED" : "RELEASED");
  }
  
  if (dirty) {
    updateJoystick();
  }
}
