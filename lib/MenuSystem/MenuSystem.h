#ifndef MENUSYSTEM_H
#define MENUSYSTEM_H

#include <Arduino.h>

#include "AppState.h"
#include "ButtonManager.h"
#include "EffectsEngine.h"
#include "MatrixDisplayManager.h"
#include "RTClib.h"
#include "SettingsManager.h"
#include "TimeManager.h"
#include "WiFiManager.h"

// Menu timing constants
#define MENU_DELAY 20  // Reduced from 30ms for snappier response

// Time setting steps
enum SetClockStep { NONE, SET_HOUR, SET_MINUTE, SET_SECOND, CONFIRM };

class MenuSystem {
   private:
    // Dependencies
    MatrixDisplayManager* display;
    SettingsManager* settings;
    ButtonManager* buttons;
    EffectsEngine* effects;
    RTC_DS3231* rtc;
    WiFiManager* wifi;
    TimeManager* timeManager;

    // Menu configuration
    static const char* menuItems[];
    static const int MENU_ITEMS;
    static const char* effectNames[];
    static const int EFFECT_OPTIONS;
    static const char* clockColorNames[];
    static const int CLOCK_COLOR_OPTIONS;
    static const char* timezoneNames[];
    static const int TIMEZONE_OPTIONS;
    static const int timezoneOffsets[];
    static const bool timezoneDST[];
    static const int timezoneDSTOffset[];
    static const char* timezoneCodes[];

    // Menu state
    int menuIndex;
    int effectMenuIndex;
    int clockColorMenuIndex;
    int timezoneMenuIndex;

    // Time setting state
    int setHour, setMin, setSec;
    bool inSetMode;
    uint32_t timeSetEntryTime;
    const uint32_t BUTTON_LOCK_DURATION = 750;  // Reduced from 1000ms for faster access
    uint32_t lastEnterPress;
    const uint32_t ENTER_COOLDOWN = 300;
    bool entryLockProcessed;
    SetClockStep setStep;

    // Menu entry state
    bool blockMenuReentry;
    uint32_t enterPressTime;
    bool wasPressed;
    AppState previousState;  // Store the state we came from

    // WiFi serial entry state
    char wifiSSIDBuffer[32];
    char wifiPasswordBuffer[64];
    bool serialInputMode;     // True when accepting serial input
    bool waitingForSSID;      // True when waiting for SSID via serial
    bool waitingForPassword;  // True when waiting for password via serial

    // NTP sync state management
    enum NTPSyncState {
        NTP_SYNC_IDLE,
        NTP_SYNC_REQUESTED,
        NTP_SYNC_IN_PROGRESS,
        NTP_SYNC_SUCCESS,
        NTP_SYNC_ERROR
    };
    NTPSyncState ntpSyncState;
    String ntpSyncMessage;
    unsigned long ntpSyncStartTime;
    unsigned long ntpSyncAttemptTime;
    static const unsigned long NTP_SYNC_DISPLAY_DURATION = 1500;  // ms
    static const unsigned long NTP_SYNC_TIMEOUT = 8000;           // 8 second timeout

    // Menu display functions
    void displayMainMenu();
    void displayEffectsMenu();
    void displayTextSizeMenu();
    void displayBrightnessMenu();
    void displayTimeFormatMenu();
    void displayClockColorMenu();
    void displayTimezoneMenu();

    // Specialized menus
    void displayWiFiMenu();
    void displayOTAMenu();
    void handleSerialWiFiInput();
    void startSerialWiFiSetup();

    // Menu input handlers
    void handleMainMenuInput();
    void handleEffectsMenuInput();
    void handleTextSizeInput();
    void handleBrightnessInput();
    void handleTimeFormatInput();
    void handleClockColorInput();
    void handleTimezoneInput();

    // Time setting functions
    void handleTimeSettingMode();

    // Menu entry logic
    void handleMenuEntry();

   public:
    MenuSystem(MatrixDisplayManager* displayManager, SettingsManager* settingsManager,
               ButtonManager* buttonManager, EffectsEngine* effectsEngine, RTC_DS3231* rtcInstance,
               WiFiManager* wifiManager, TimeManager* timeManager);

    void begin();
    void reset();
    void handleInput(AppState& appState);
    void updateDisplay(AppState appState);

    // NTP sync management
    bool isNTPSyncRequested() const {
        return ntpSyncState == NTP_SYNC_REQUESTED;
    }
    void setNTPSyncInProgress() {
        ntpSyncState = NTP_SYNC_IN_PROGRESS;
        ntpSyncMessage = "Syncing NTP...";
        ntpSyncStartTime = millis();
    }
    void setNTPSyncResult(bool success);

    // Menu entry check for main loop
    bool shouldEnterMenu();

    // State transition helpers
    AppState getNextState();
    AppState getEffectsMenuNextState();
    AppState getTextSizeMenuNextState();
    AppState getBrightnessMenuNextState();
    AppState getTimeFormatMenuNextState();
    AppState getClockColorMenuNextState();
    AppState getTimezoneMenuNextState();
    AppState getTimeSettingNextState();

    // Getters for main loop
    int getMenuDelay() const {
        return MENU_DELAY;
    }
};

#endif
