#ifndef APP_STATE_MANAGER_H
#define APP_STATE_MANAGER_H

#include <Arduino.h>
#include "ButtonManager.h"
#include "SettingsManager.h"
#include "MatrixDisplayManager.h"
#include "EffectsEngine.h"
#include "MenuSystem.h"
#include "ClockDisplay.h"

class AppStateManager {
public:
    // Constructor
    AppStateManager(ButtonManager* buttons, SettingsManager* settings, 
                   MatrixDisplayManager* display, EffectsEngine* effects,
                   MenuSystem* menu, ClockDisplay* clock);
    
    // Initialization
    void begin();
    
    // Main update methods
    void handleInput();
    void updateDisplay();
    
    // State management
    AppState getCurrentState() const { return currentState; }
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
    
    // State management
    AppState currentState;
    
    // Display update methods
    void renderTimeDisplay();
    void renderMenus();
    
    // Timing constants
    static const uint32_t CLOCK_UPDATE_DELAY = 10;
    static const uint32_t APP_MENU_DELAY = 30;
    
    // Legacy variables (kept for compatibility)
    bool blockMenuReentry;
    uint32_t enterPressTime;
    bool wasPressed;
};

#endif // APP_STATE_MANAGER_H
