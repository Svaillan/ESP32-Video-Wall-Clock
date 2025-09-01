#include "AppStateManager.h"

AppStateManager::AppStateManager(ButtonManager* buttons, SettingsManager* settings,
                                 MatrixDisplayManager* display, EffectsEngine* effects,
                                 MenuSystem* menu, ClockDisplay* clock, WiFiInfoDisplay* wifiInfo)
    : buttons(buttons),
      settings(settings),
      display(display),
      effects(effects),
      menu(menu),
      clock(clock),
      wifiInfo(wifiInfo),
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
    // Handle mode switching in clock display states
    if (currentState == SHOW_TIME) {
        if (buttons->isDownJustPressed()) {
            setState(SHOW_WIFI_INFO);
            return;
        }
    } else if (currentState == SHOW_WIFI_INFO) {
        if (buttons->isDownJustPressed()) {
            setState(SHOW_TIME);
            return;
        }
    }

    // Let MenuSystem handle all other input and state transitions
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

        case SHOW_WIFI_INFO:
            renderWiFiInfoDisplay();
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

void AppStateManager::renderWiFiInfoDisplay() {
    // Disable button repeat for WiFi info display
    buttons->setAllowButtonRepeat(false);

    // Update WiFi info display
    wifiInfo->updateDisplay();
}

void AppStateManager::renderMenus() {
    // Handle all menu states with MenuSystem
    menu->updateDisplay(currentState);
}

void AppStateManager::processDelay() {
    if (currentState == SHOW_TIME || currentState == SHOW_WIFI_INFO) {
        delay(CLOCK_UPDATE_DELAY);
    } else {
        delay(APP_MENU_DELAY);
    }
}
