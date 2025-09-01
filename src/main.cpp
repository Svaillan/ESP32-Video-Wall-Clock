#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>

#include <Adafruit_Protomatter.h>
#include <RTClib.h>
#include <time.h>  // For NTP and timezone

#include "AppStateManager.h"
#include "ButtonManager.h"
#include "ClockDisplay.h"
#include "EffectsEngine.h"
#include "MatrixDisplayManager.h"
#include "MenuSystem.h"
#include "SettingsManager.h"
#include "SystemManager.h"
#include "TimeManager.h"
#include "WiFiInfoDisplay.h"
#include "WiFiManager.h"

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

// Hardware Objects
Adafruit_Protomatter matrix(MATRIX_WIDTH, BIT_DEPTH, 1, rgbPins, 4, addrPins, clockPin, latchPin,
                            oePin, true);
RTC_DS3231 rtc;

// TimeManager instance (must be after rtc)
TimeManager timeManager(&rtc);

// System instances
SettingsManager settings;
ButtonManager buttons;
WiFiManager wifiManager(&settings);
MatrixDisplayManager display(&matrix, &settings);
EffectsEngine effects(&display, &settings);
ClockDisplay clockDisplay(&display, &settings, &rtc, &timeManager);
MenuSystem menu(&display, &settings, &buttons, &effects, &rtc, &wifiManager, &timeManager);
WiFiInfoDisplay wifiInfoDisplay(&display, &wifiManager, &settings);
AppStateManager appManager(&buttons, &settings, &display, &effects, &menu, &clockDisplay,
                           &wifiInfoDisplay);

// State Variables
unsigned long systemStartTime = 0;
const uint32_t STARTUP_GRACE_PERIOD = 2000;  // 2 seconds to stabilize

// System Manager - handles initialization
SystemManager systemManager(&matrix, &rtc, &settings, &buttons, &display, &timeManager, &effects,
                            &clockDisplay, &wifiInfoDisplay, &appManager, &wifiManager,
                            &systemStartTime);

/**
 * Initialize hardware and system components
 */
void setup() {
    systemManager.initializeSystem();
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

    // Update button states
    buttons.updateAll();

    // Skip input processing during startup grace period
    if (millis() - systemStartTime >= STARTUP_GRACE_PERIOD) {
        appManager.handleInput();
    }

    // Handle all NTP sync coordination (periodic and manual)
    systemManager.handleNTPSync(&menu);

    appManager.updateDisplay();
    effects.updateEffects();  // Render background effects
    appManager.processDelay();
}
