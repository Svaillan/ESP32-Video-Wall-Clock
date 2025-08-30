#ifndef MENUSYSTEM_H
#define MENUSYSTEM_H

#include <Arduino.h>
#include "RTClib.h"
#include "SettingsManager.h"
#include "MatrixDisplayManager.h"
#include "ButtonManager.h"
#include "EffectsEngine.h"

// Menu timing constants
#define MENU_DELAY 30               // Reduced from 50ms 

// Application states
enum AppState { 
  SHOW_TIME, 
  MENU, 
  EDIT_TEXT_SIZE, 
  EDIT_BRIGHTNESS, 
  EDIT_TIME_FORMAT,
  EDIT_CLOCK_COLOR,
  EDIT_EFFECTS, 
  TIME_SET 
};

// Time setting steps
enum SetClockStep { 
  NONE, 
  SET_HOUR, 
  SET_MINUTE, 
  SET_SECOND, 
  CONFIRM 
};

class MenuSystem {
private:
    // Dependencies
    MatrixDisplayManager* display;
    SettingsManager* settings;
    ButtonManager* buttons;
    EffectsEngine* effects;
    RTC_DS3231* rtc;
    
    // Menu configuration
    static const char* menuItems[];
    static const int MENU_ITEMS;
    static const char* effectNames[];
    static const int EFFECT_OPTIONS;
    static const char* clockColorNames[];
    static const int CLOCK_COLOR_OPTIONS;
    
    // Menu state
    int menuIndex;
    int effectMenuIndex;
    int clockColorMenuIndex;
    
    // Time setting state
    int setHour, setMin, setSec;
    bool inSetMode;
    uint32_t timeSetEntryTime;
    const uint32_t BUTTON_LOCK_DURATION = 1000;
    uint32_t lastEnterPress;
    const uint32_t ENTER_COOLDOWN = 300;
    bool entryLockProcessed;
    SetClockStep setStep;
    
    // Menu entry state
    bool blockMenuReentry;
    uint32_t enterPressTime;
    bool wasPressed;
    
    // Menu display functions
    void displayMainMenu();
    void displayEffectsMenu();
    void displayTextSizeMenu();
    void displayBrightnessMenu();
    void displayTimeFormatMenu();
    void displayClockColorMenu();
    
    // Menu input handlers
    void handleMainMenuInput();
    void handleEffectsMenuInput();
    void handleTextSizeInput();
    void handleBrightnessInput();
    void handleTimeFormatInput();
    void handleClockColorInput();
    
    // Time setting functions
    void handleTimeSettingMode();
    
    // Menu entry logic
    void handleMenuEntry();
    
public:
    MenuSystem(MatrixDisplayManager* displayManager, SettingsManager* settingsManager, 
               ButtonManager* buttonManager, EffectsEngine* effectsEngine, RTC_DS3231* rtcInstance);
    
    void begin();
    void reset(); // Reset menu system to ensure clean start
    void handleInput(AppState& appState);
    void updateDisplay(AppState appState);
    
    // Menu entry check for main loop
    bool shouldEnterMenu();
    
    // State transition helpers
    AppState getNextState();
    AppState getEffectsMenuNextState();
    AppState getTextSizeMenuNextState();
    AppState getBrightnessMenuNextState();
    AppState getTimeFormatMenuNextState();
    AppState getClockColorMenuNextState();
    AppState getTimeSettingNextState();
    
    // Getters for main loop
    int getMenuDelay() const { return MENU_DELAY; }
};

#endif
