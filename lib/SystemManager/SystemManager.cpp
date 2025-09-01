#include "SystemManager.h"

#include "MenuSystem.h"

SystemManager::SystemManager(Adafruit_Protomatter* matrix, RTC_DS3231* rtc,
                             SettingsManager* settings, ButtonManager* buttons,
                             MatrixDisplayManager* display, TimeManager* timeManager,
                             EffectsEngine* effects, ClockDisplay* clockDisplay,
                             WiFiInfoDisplay* wifiInfoDisplay, AppStateManager* appManager,
                             WiFiManager* wifiManager, unsigned long* systemStartTime)
    : matrix(matrix),
      rtc(rtc),
      settings(settings),
      buttons(buttons),
      display(display),
      timeManager(timeManager),
      effects(effects),
      clockDisplay(clockDisplay),
      wifiInfoDisplay(wifiInfoDisplay),
      appManager(appManager),
      wifiManager(wifiManager),
      systemStartTime(systemStartTime) {}

void SystemManager::initializeSystem() {
    Serial.begin(9600);
    Serial.println("Matrix Sign Starting...");

    initializeHardware();
    initializeManagers();
    initializeWiFiAndOTA();

    // Set startup time for grace period
    *systemStartTime = millis();

    Serial.println("Setup complete!");
}

void SystemManager::initializeHardware() {
    // Initialize matrix
    ProtomatterStatus status = matrix->begin();
    if (status != PROTOMATTER_OK) {
        Serial.print("Matrix initialization failed: ");
        Serial.println(status);
        while (true)
            ;  // Halt on failure
    }
    Serial.println("Matrix initialized successfully");

    // Initialize RTC
    Wire.begin();
    if (!rtc->begin()) {
        Serial.println("Couldn't find RTC");
        while (true)
            ;
    }
    Serial.println("RTC initialized successfully");

    if (rtc->lostPower()) {
        Serial.println("RTC lost power, setting time!");
        rtc->adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
}

void SystemManager::initializeManagers() {
    // Initialize settings manager
    settings->begin();

    // Initialize button manager
    buttons->begin();

    // Initialize display manager
    display->begin();

    // Initialize TimeManager and set timezone
    timeManager->begin();

    // Set timezone from saved settings
    int savedTimezoneIndex = settings->getTimezoneIndex();

    // Define timezone data (must match MenuSystem arrays)
    const int timezoneOffsets[] = {-7, -10, -9, -8, -7, -6, -5, -4, -3, -3, 0,  0,
                                   1,  2,   3,  4,  4,  5,  7,  8,  9,  9,  10, 12};
    const bool timezoneDST[] = {false, false, true,  true,  true,  true,  true,  true,
                                false, true,  false, true,  true,  false, false, false,
                                true,  false, false, false, false, false, true,  true};
    const int timezoneDSTOffset[] = {0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1,
                                     1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1};
    const int TIMEZONE_OPTIONS = 24;

    // Validate saved timezone index and set timezone
    if (savedTimezoneIndex >= 0 && savedTimezoneIndex < TIMEZONE_OPTIONS) {
        timeManager->setTimezoneOffset(timezoneOffsets[savedTimezoneIndex],
                                       timezoneDST[savedTimezoneIndex],
                                       timezoneDSTOffset[savedTimezoneIndex]);
        Serial.printf("[SystemManager] Setting timezone to index %d: UTC%+d (DST: %s)\n",
                      savedTimezoneIndex, timezoneOffsets[savedTimezoneIndex],
                      timezoneDST[savedTimezoneIndex] ? "yes" : "no");
    } else {
        // Fallback to Arizona timezone if invalid index
        timeManager->setTimezoneOffset(-7, false, 0);
        Serial.println("[SystemManager] Using fallback timezone: UTC-7 (Arizona)");
    }

    // Initialize effects engine
    effects->begin();

    // Initialize clock display
    clockDisplay->begin();

    // Initialize WiFi info display
    wifiInfoDisplay->begin();

    // Initialize app state manager
    appManager->begin();
}

void SystemManager::initializeWiFiAndOTA() {
    // Initialize WiFi and OTA if enabled
    if (settings->isWiFiEnabled()) {
        Serial.println("WiFi enabled, connecting...");
        wifiManager->begin(settings->getWiFiSSID(), settings->getWiFiPassword());

        // Always setup OTA if WiFi is enabled (it will work once WiFi connects)
        wifiManager->setupOTA("matrix-clock",
                              display);  // Uses randomly generated password with display blanking

        if (wifiManager->isConnected()) {
            Serial.println("WiFi connected - OTA ready for uploads!");
            // NTP sync on boot if WiFi is connected
            timeManager->syncTimeWithNTP(true);
        } else {
            Serial.println("WiFi connecting... OTA will be available once connected");
        }
    } else {
        Serial.println("WiFi disabled - use menu to configure");
    }
}

// ===================== SYSTEM COORDINATION METHODS =====================

void SystemManager::handleNTPSync(MenuSystem* menu) {
    // Periodic NTP sync (every 12 hours if WiFi is enabled and connected)
    if (settings->isWiFiEnabled() && wifiManager->isConnected()) {
        timeManager->periodicNTPSync();  // This uses non-blocking approach
    }

    // Handle NTP sync requests from menu system
    if (menu->isNTPSyncRequested()) {
        menu->setNTPSyncInProgress();
        timeManager->startNTPSync(true);
    }

    // Update any ongoing NTP sync (both manual and periodic)
    if (timeManager->isNTPSyncInProgress()) {
        timeManager->updateNTPSync();
        // Don't check result here - will be handled below
    }

    // Check if NTP sync just completed (only triggers once)
    if (timeManager->checkAndClearNTPSyncCompletion()) {
        bool success = timeManager->wasLastNTPSyncSuccessful();
        menu->setNTPSyncResult(success);
    }
}
