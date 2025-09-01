#ifndef APP_STATE_MANAGER_H
#define APP_STATE_MANAGER_H

#include <Arduino.h>

#include "AppState.h"
#include "ButtonManager.h"
#include "ClockDisplay.h"
#include "EffectsEngine.h"
#include "MatrixDisplayManager.h"
#include "MenuSystem.h"
#include "SettingsManager.h"
#include "WiFiInfoDisplay.h"

class AppStateManager {
   public:
    // Constructor
    AppStateManager(ButtonManager* buttons, SettingsManager* settings,
                    MatrixDisplayManager* display, EffectsEngine* effects, MenuSystem* menu,
                    ClockDisplay* clock, WiFiInfoDisplay* wifiInfo);

    // Initialization
    void begin();

    // Main update methods
    void handleInput();
    void updateDisplay();

    // State management
    AppState getCurrentState() const {
        return currentState;
    }
    void setState(AppState newState);

    // Delay management
    void processDelay();

   private:
    // Component references
    ButtonManager* buttons;
    SettingsManager* settings;
    MatrixDisplayManager* display;
    EffectsEngine* effects;
    MenuSystem* menu;
    ClockDisplay* clock;
    WiFiInfoDisplay* wifiInfo;

    // State management
    AppState currentState;

    // Display update methods
    void renderTimeDisplay();
    void renderTimeWithDateDisplay();
    void renderWiFiInfoDisplay();
    void renderMenus();

    // Helper methods for state cycling
    AppState getNextDisplayState(AppState current);
    AppState getPreviousDisplayState(AppState current);

    // Display state cycle definition
    static const AppState DISPLAY_STATES[];
    static const int DISPLAY_STATE_COUNT;
    int getCurrentDisplayStateIndex();

    // Timing constants
    static const uint32_t CLOCK_UPDATE_DELAY = 5;  // Reduced from 10ms for more responsive buttons
    static const uint32_t APP_MENU_DELAY = 20;     // Reduced from 30ms for snappier menu response

    // State tracking variables
    bool blockMenuReentry;
    uint32_t enterPressTime;
    bool wasPressed;
};

#endif  // APP_STATE_MANAGER_H
