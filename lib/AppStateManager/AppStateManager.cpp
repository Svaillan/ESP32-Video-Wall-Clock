#include "AppStateManager.h"

// Define the display state cycle order
const AppState AppStateManager::DISPLAY_STATES[] = {SHOW_TIME, SHOW_TIME_WITH_DATE, SHOW_WIFI_INFO};
const int AppStateManager::DISPLAY_STATE_COUNT = sizeof(DISPLAY_STATES) / sizeof(DISPLAY_STATES[0]);

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
    // Handle mode switching in display states with up/down keys
    if (getCurrentDisplayStateIndex() != -1) {  // Current state is a display state
        if (buttons->isUpJustPressed()) {
            setState(getPreviousDisplayState(currentState));
            return;
        } else if (buttons->isDownJustPressed()) {
            setState(getNextDisplayState(currentState));
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

        case SHOW_TIME_WITH_DATE:
            renderTimeWithDateDisplay();
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

    // Set display mode for effects engine
    effects->setDisplayMode(SHOW_TIME);

    // Render background effects first
    effects->updateEffects();

    // Display current time on top
    clock->displayTime();
}

void AppStateManager::renderTimeWithDateDisplay() {
    // Disable button repeat for clock with date display
    buttons->setAllowButtonRepeat(false);

    // Set display mode for effects engine
    effects->setDisplayMode(SHOW_TIME_WITH_DATE);

    // Render background effects first
    effects->updateEffects();

    // Display time with date
    clock->displayTimeWithDate();
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

AppState AppStateManager::getNextDisplayState(AppState current) {
    int currentIndex = getCurrentDisplayStateIndex();
    if (currentIndex == -1)
        return SHOW_TIME;  // Fallback

    int nextIndex = (currentIndex + 1) % DISPLAY_STATE_COUNT;
    return DISPLAY_STATES[nextIndex];
}

AppState AppStateManager::getPreviousDisplayState(AppState current) {
    int currentIndex = getCurrentDisplayStateIndex();
    if (currentIndex == -1)
        return SHOW_TIME;  // Fallback

    int prevIndex = (currentIndex - 1 + DISPLAY_STATE_COUNT) % DISPLAY_STATE_COUNT;
    return DISPLAY_STATES[prevIndex];
}

int AppStateManager::getCurrentDisplayStateIndex() {
    for (int i = 0; i < DISPLAY_STATE_COUNT; i++) {
        if (DISPLAY_STATES[i] == currentState) {
            return i;
        }
    }
    return -1;  // Not found
}

void AppStateManager::processDelay() {
    if (currentState == SHOW_TIME || currentState == SHOW_TIME_WITH_DATE ||
        currentState == SHOW_WIFI_INFO) {
        delay(CLOCK_UPDATE_DELAY);
    } else {
        delay(APP_MENU_DELAY);
    }
}
