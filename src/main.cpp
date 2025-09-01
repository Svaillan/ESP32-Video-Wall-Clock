#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>

#include <Adafruit_Protomatter.h>
#include <RTClib.h>

#include "AppStateManager.h"
#include "ButtonManager.h"
#include "ClockDisplay.h"
#include "EffectsEngine.h"
#include "MatrixDisplayManager.h"
#include "MenuSystem.h"
#include "SettingsManager.h"
#include "WiFiInfoDisplay.h"
#include "WiFiManager.h"

// ===============================================
// CONFIGURATION & CONSTANTS
// ===============================================

// Matrix Pin Configuration
uint8_t rgbPins[] = {25, 27, 26, 14, 13, 12};  // {R1, B1, G1, R2, B2, G2}
uint8_t addrPins[] = {23, 19, 5, 17};
uint8_t clockPin = 16;
uint8_t latchPin = 4;
uint8_t oePin = 15;

// Matrix Display Settings
#define MATRIX_WIDTH 128
#define MATRIX_HEIGHT 32
#define BIT_DEPTH 5

// Text Settings
#define TEXT_SIZE_MIN 1
#define TEXT_SIZE_MAX 3
#define BRIGHTNESS_LEVELS 10

// Timing Constants
// (MENU_DELAY now defined in MenuSystem.h)
// (UPDATE_DELAY now defined in AppStateManager.h)

// ===============================================
// DATA STRUCTURES & ENUMS
// ===============================================

// ===============================================
// GLOBAL VARIABLES
// ===============================================

// Hardware Objects
Adafruit_Protomatter matrix(MATRIX_WIDTH, BIT_DEPTH, 1, rgbPins, 4, addrPins, clockPin, latchPin,
                            oePin, true);
RTC_DS3231 rtc;

// System instances
SettingsManager settings;
ButtonManager buttons;
WiFiManager wifiManager(&settings);
MatrixDisplayManager display(&matrix, &settings);
EffectsEngine effects(&display, &settings);
ClockDisplay clockDisplay(&display, &settings, &rtc);
MenuSystem menu(&display, &settings, &buttons, &effects, &rtc, &wifiManager);
WiFiInfoDisplay wifiInfoDisplay(&display, &wifiManager, &settings);
AppStateManager appManager(&buttons, &settings, &display, &effects, &menu, &clockDisplay,
                           &wifiInfoDisplay);

// State Variables
uint32_t systemStartTime = 0;
const uint32_t STARTUP_GRACE_PERIOD = 2000;  // 2 seconds to stabilize

// ===============================================
// UTILITY FUNCTIONS
// ===============================================

/**
 * Get the bounding box of the current time display (main clock only)
 */
void getTimeDisplayBounds(int& x1, int& y1, int& x2, int& y2) {
    clockDisplay.getTimeDisplayBounds(x1, y1, x2, y2);
}

/**
 * Get the bounding box of the AM/PM indicator
 */
void getAMPMDisplayBounds(int& x1, int& y1, int& x2, int& y2) {
    clockDisplay.getAMPMDisplayBounds(x1, y1, x2, y2);
}

/**
 * Check if a point is within the text area (either main clock or AM/PM)
 */
bool isInTextArea(int x, int y) {
    if (appManager.getCurrentState() != SHOW_TIME)
        return false;

    return clockDisplay.isInTextArea(x, y);
}

/**
 * Render background effects
 */
void renderEffects() {
    effects.updateEffects();
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
        while (true)
            ;  // Halt on failure
    }
    Serial.println("Matrix initialized successfully");

    // Initialize display manager
    display.begin();

    // Initialize RTC
    Wire.begin();
    if (!rtc.begin()) {
        Serial.println("Couldn't find RTC");
        while (true)
            ;
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

    // Initialize WiFi info display
    wifiInfoDisplay.begin();

    // Initialize app state manager
    appManager.begin();

    // Initialize WiFi and OTA if enabled
    if (settings.isWiFiEnabled()) {
        Serial.println("WiFi enabled, connecting...");
        wifiManager.begin(settings.getWiFiSSID(), settings.getWiFiPassword());

        // Always setup OTA if WiFi is enabled (it will work once WiFi connects)
        wifiManager.setupOTA("matrix-clock",
                             &display);  // Uses randomly generated password with display blanking

        if (wifiManager.isConnected()) {
            Serial.println("WiFi connected - OTA ready for uploads!");
        } else {
            Serial.println("WiFi connecting... OTA will be available once connected");
        }
    } else {
        Serial.println("WiFi disabled - use menu to configure");
    }

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
    // Handle OTA updates first (highest priority)
    wifiManager.handleOTA();

    // If OTA is in progress, show progress and skip normal operation
    if (wifiManager.isOTAInProgress()) {
        wifiManager.displayStatus(&display);
        delay(100);  // Small delay to prevent flickering
        return;
    }

    buttons.updateAll();
    handleInput();
    updateDisplay();
    appManager.processDelay();
}
