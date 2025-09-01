#ifndef SYSTEM_MANAGER_H
#define SYSTEM_MANAGER_H

#include <Arduino.h>
#include <Wire.h>

#include <Adafruit_Protomatter.h>
#include <RTClib.h>

#include "AppStateManager.h"
#include "ButtonManager.h"
#include "ClockDisplay.h"
#include "EffectsEngine.h"
#include "MatrixDisplayManager.h"
#include "SettingsManager.h"
#include "TimeManager.h"
#include "WiFiInfoDisplay.h"
#include "WiFiManager.h"

// Forward declarations
class MenuSystem;

class SystemManager {
   public:
    SystemManager(Adafruit_Protomatter* matrix, RTC_DS3231* rtc, SettingsManager* settings,
                  ButtonManager* buttons, MatrixDisplayManager* display, TimeManager* timeManager,
                  EffectsEngine* effects, ClockDisplay* clockDisplay,
                  WiFiInfoDisplay* wifiInfoDisplay, AppStateManager* appManager,
                  WiFiManager* wifiManager, unsigned long* systemStartTime);

    void initializeSystem();

    // System coordination methods
    void handleNTPSync(MenuSystem* menu);

   private:
    // Hardware components
    Adafruit_Protomatter* matrix;
    RTC_DS3231* rtc;

    // Manager components
    SettingsManager* settings;
    ButtonManager* buttons;
    MatrixDisplayManager* display;
    TimeManager* timeManager;
    EffectsEngine* effects;
    ClockDisplay* clockDisplay;
    WiFiInfoDisplay* wifiInfoDisplay;
    AppStateManager* appManager;
    WiFiManager* wifiManager;

    // System state
    unsigned long* systemStartTime;

    // Private initialization methods
    void initializeHardware();
    void initializeManagers();
    void initializeWiFiAndOTA();
};

#endif  // SYSTEM_MANAGER_H
