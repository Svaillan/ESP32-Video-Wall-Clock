#include <Arduino.h>
#include <Adafruit_Protomatter.h>
#include <Wire.h>
#include <RTClib.h>
#include <EEPROM.h>
#include "SettingsManager.h"
#include "ButtonManager.h"
#include "MatrixDisplayManager.h"
#include "EffectsEngine.h"
#include "MenuSystem.h"
#include "ClockDisplay.h"
#include "AppStateManager.h"

// ===============================================
// CONFIGURATION & CONSTANTS
// ===============================================

// Matrix Pin Configuration
uint8_t rgbPins[]  = {25, 27, 26, 14, 13, 12}; // {R1, B1, G1, R2, B2, G2}
uint8_t addrPins[] = {23, 19, 5, 17};
uint8_t clockPin   = 16;
uint8_t latchPin   = 4;
uint8_t oePin      = 15;

// Matrix Display Settings
#define MATRIX_WIDTH  128
#define MATRIX_HEIGHT 32
#define BIT_DEPTH     5

// Text Settings
#define TEXT_SIZE_MIN 1
#define TEXT_SIZE_MAX 3
#define BRIGHTNESS_LEVELS 10

// Timing Constants
#define MENU_DELAY 30               // Reduced from 50ms 
#define CLOCK_UPDATE_DELAY 10

// ===============================================
// DATA STRUCTURES & ENUMS
// ===============================================

// ===============================================
// GLOBAL VARIABLES
// ===============================================

// Hardware Objects
Adafruit_Protomatter matrix(
  MATRIX_WIDTH, BIT_DEPTH, 1, rgbPins,
  4, addrPins, clockPin, latchPin, oePin, true);
RTC_DS3231 rtc;

// System instances
SettingsManager settings;
ButtonManager buttons;
MatrixDisplayManager display(&matrix, &settings);
EffectsEngine effects(&display, &settings);
ClockDisplay clockDisplay(&display, &settings, &rtc);
MenuSystem menu(&display, &settings, &buttons, &effects, &rtc);
AppStateManager appManager(&buttons, &settings, &display, &effects, &menu, &clockDisplay);

// State Variables
uint32_t systemStartTime = 0;
const uint32_t STARTUP_GRACE_PERIOD = 2000; // 2 seconds to stabilize

// ===============================================
// EFFECTS SYSTEM
// ===============================================

/**
 * Initialize confetti particles
 */



/**
 * Get the bounding box of the current time display (main clock only)
 */
void getTimeDisplayBounds(int &x1, int &y1, int &x2, int &y2) {
  clockDisplay.getTimeDisplayBounds(x1, y1, x2, y2);
}

/**
 * Get the bounding box of the AM/PM indicator
 */
void getAMPMDisplayBounds(int &x1, int &y1, int &x2, int &y2) {
  clockDisplay.getAMPMDisplayBounds(x1, y1, x2, y2);
}

/**
 * Check if a point is within the text area (either main clock or AM/PM)
 */
bool isInTextArea(int x, int y) {
  if (appManager.getCurrentState() != SHOW_TIME) return false;
  
  return clockDisplay.isInTextArea(x, y);
}



/**
 * Render background effects
 */
void renderEffects() {
  effects.updateEffects();
  // Don't draw text background here - let clock display handle its own background if needed
}

// ===============================================
// MAIN PROGRAM LOGIC
// ===============================================

/**
 * Initialize hardware and system components
 */
void initializeSystem() {
  Serial.begin(9600);
  Serial.println("Matrix Sign Starting...");
  
  // Initialize settings manager
  settings.begin();
  
  // Initialize button manager
  buttons.begin();
  
  // Initialize matrix
  ProtomatterStatus status = matrix.begin();
  if (status != PROTOMATTER_OK) {
    Serial.print("Matrix initialization failed: ");
    Serial.println(status);
    while (true); // Halt on failure
  }
  Serial.println("Matrix initialized successfully");
  
  // Initialize display manager
  display.begin();
  
  // Initialize RTC
  Wire.begin();
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (true);
  }
  Serial.println("RTC initialized successfully");
  
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  // Initialize effects engine
  effects.begin();
  
  // Initialize clock display
  clockDisplay.begin();
  
  // Initialize app state manager
  appManager.begin();
  
  // Set startup time for grace period
  systemStartTime = millis();
  
  Serial.println("Setup complete!");
}

/**
 * Update the main display based on current state
 */
void updateDisplay() {
  appManager.updateDisplay();
}

/**
 * Handle user input based on current state
 */
void handleInput() {
  // Skip input processing during startup grace period
  if (millis() - systemStartTime < STARTUP_GRACE_PERIOD) {
    return;
  }
  appManager.handleInput();
}

// ===============================================
// ARDUINO MAIN FUNCTIONS
// ===============================================

void setup() {
  initializeSystem();
}

void loop() {
  buttons.updateAll();
  handleInput();
  updateDisplay();
  
  // Use AppStateManager for delay management
  appManager.processDelay();
}
