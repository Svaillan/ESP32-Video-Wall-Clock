#include "AppStateManager.h"

// Define the display state cycle order
const AppState AppStateManager::DISPLAY_STATES[] = {SHOW_TIME, SHOW_TIME_WITH_DATE, SHOW_WIFI_INFO,
                                                    SHOW_MESSAGES};
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
      previousStateBeforeMessage(SHOW_TIME),
      wasInterruptedByMessage(false),
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
    // This includes menu entry from any state, including SHOW_MESSAGES
    menu->handleInput(currentState);
}

void AppStateManager::updateDisplay() {
    // Handle high-priority messages that can interrupt any state
    display->processMessageQueue();
    bool hasHighPriorityMessage = display->hasActiveHighPriorityMessage();

    // If we have a high-priority message and we're not already showing messages,
    // save current state and switch to message mode
    if (hasHighPriorityMessage && currentState != SHOW_MESSAGES) {
        previousStateBeforeMessage = currentState;
        currentState = SHOW_MESSAGES;
        wasInterruptedByMessage = true;  // Flag that we were interrupted
    }

    // Handle enter key press to cancel messages and return to previous state
    // Only consume enter key if there's actually an active message to cancel
    if (currentState == SHOW_MESSAGES && buttons->isEnterJustPressed() &&
        (display->hasQueuedMessages() || display->hasActiveHighPriorityMessage())) {
        display->cancelActiveMessage();
        // If we were interrupted by a message, return to previous state
        if (wasInterruptedByMessage) {
            currentState = previousStateBeforeMessage;
            wasInterruptedByMessage = false;
        }
        return;
    }

    // If we were interrupted by a message and it's now finished, auto-return
    if (currentState == SHOW_MESSAGES && !hasHighPriorityMessage && !display->hasQueuedMessages() &&
        wasInterruptedByMessage) {
        currentState = previousStateBeforeMessage;
        wasInterruptedByMessage = false;  // Clear the flag
        return;
    }

    // If in message mode but no active messages, show waiting screen
    if (currentState == SHOW_MESSAGES) {
        renderMessageDisplay();
        return;
    }

    // Normal rendering for other states
    display->fillScreen(0);
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

void AppStateManager::renderMessageDisplay() {
    // Don't clear screen - let processMessageQueue handle display
    display->processMessageQueue();

    if (display->hasQueuedMessages()) {
        // Message is being displayed, processMessageQueue handles rendering
        // Set display mode so effects respect the message area
        effects->setDisplayMode(SHOW_MESSAGES);
        // Draw effects before showing the final result
        effects->updateEffects();
        // Now call show() to display both message and effects
        display->show();
        return;
    }

    // No messages - show "waiting for message..." screen
    buttons->setAllowButtonRepeat(false);
    effects->setDisplayMode(SHOW_MESSAGES);

    // Clear screen and render background effects
    display->fillScreen(0);
    effects->updateEffects();

    // Draw "waiting for message" in smallest font with proper bounding box
    const char* waitMsg = "waiting for message";
    uint16_t color = display->getClockColor();
    display->drawCenteredTextWithBox(waitMsg, 0, color, 0x0000);

    display->show();
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
