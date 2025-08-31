#include "AppStateManager.h"

AppStateManager::AppStateManager(ButtonManager* buttons, SettingsManager* settings,
                                 MatrixDisplayManager* display, EffectsEngine* effects,
                                 MenuSystem* menu, ClockDisplay* clock)
    : buttons(buttons),
      settings(settings),
      display(display),
      effects(effects),
      menu(menu),
      clock(clock),
      currentState(SHOW_TIME),
      blockMenuReentry(false),
      enterPressTime(0),
      wasPressed(false) {}

void AppStateManager::begin() {
    Serial.println("AppStateManager initialized");
    currentState = SHOW_TIME;
}

void AppStateManager::setState(AppState newState) {
    if (currentState != newState) {
        Serial.print("State change: ");
        Serial.print(currentState);
        Serial.print(" -> ");
        Serial.println(newState);
        currentState = newState;
    }
}

void AppStateManager::handleInput() {
    // Let MenuSystem handle all input and state transitions
    menu->handleInput(currentState);
}

void AppStateManager::updateDisplay() {
    // Clear screen
    display->fillScreen(0);

    // Render based on current state
    switch (currentState) {
        case SHOW_TIME:
            renderTimeDisplay();
            break;

        default:
            renderMenus();
            break;
    }

    display->show();
}

void AppStateManager::renderTimeDisplay() {
    // Disable button repeat for normal clock display
    buttons->setAllowButtonRepeat(false);

    // Render background effects first
    effects->updateEffects();

    // Display current time on top
    clock->displayTime();
}

void AppStateManager::renderMenus() {
    // Handle all menu states with MenuSystem
    menu->updateDisplay(currentState);
}

void AppStateManager::processDelay() {
    if (currentState == SHOW_TIME) {
        delay(CLOCK_UPDATE_DELAY);
    } else {
        delay(APP_MENU_DELAY);
    }
}
