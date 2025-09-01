#include "MenuSystem.h"

#include <time.h>

// Static menu data
const char* MenuSystem::menuItems[] = {"Text Size",  "Brightness", "Time Format", "Clock Color",
                                       "Effects",    "Timezone",   "Set Clock",   "Sync NTP",
                                       "WiFi Setup", "OTA Setup",  "Exit"};
const int MenuSystem::MENU_ITEMS = sizeof(menuItems) / sizeof(menuItems[0]);
const char* MenuSystem::effectNames[] = {"Confetti", "Acid",      "Rain", "Torrent", "Stars",
                                         "Sparkles", "Fireworks", "Tron", "Off"};
const int MenuSystem::EFFECT_OPTIONS = sizeof(effectNames) / sizeof(effectNames[0]);
const char* MenuSystem::clockColorNames[] = {
    "White",  "Red",  "Green", "Blue", "Yellow", "Cyan", "Magenta", "Orange",
    "Purple", "Pink", "Lime",  "Teal", "Indigo", "Gold", "Silver",  "Rainbow"};
const int MenuSystem::CLOCK_COLOR_OPTIONS = sizeof(clockColorNames) / sizeof(clockColorNames[0]);
const char* MenuSystem::timezoneNames[] = {
    "Arizona", "Hawaii",       "Alaska",  "Pacific", "Mountain", "Central", "Eastern", "Atlantic",
    "Brazil",  "Newfoundland", "UTC",     "London",  "Paris",    "Cairo",   "Moscow",  "Dubai",
    "Tehran",  "India",        "Bangkok", "China",   "Tokyo",    "Seoul",   "Sydney",  "Auckland"};
const int MenuSystem::TIMEZONE_OPTIONS = sizeof(timezoneNames) / sizeof(timezoneNames[0]);

// UTC offsets in hours (can be negative)
const int MenuSystem::timezoneOffsets[] = {-7, -10, -9, -8, -7, -6, -5, -4, -3, -3, 0,  0,
                                           1,  2,   3,  4,  4,  5,  7,  8,  9,  9,  10, 12};

// DST support flags (true if timezone observes DST)
const bool MenuSystem::timezoneDST[] = {false, false, true,  true,  true,  true,  true,  true,
                                        false, true,  false, true,  true,  false, false, false,
                                        true,  false, false, false, false, false, true,  true};

// DST offset in hours (added when DST is active)
const int MenuSystem::timezoneDSTOffset[] = {0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1,
                                             1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1};

// Timezone codes for display
const char* MenuSystem::timezoneCodes[] = {
    "MST",      "HST",      "AKST/AKDT", "PST/PDT", "MST/MDT",   "CST/CDT",
    "EST/EDT",  "AST/ADT",  "BRT",       "NST/NDT", "UTC",       "GMT/BST",
    "CET/CEST", "EET/EEST", "MSK",       "GST",     "IRST/IRDT", "IST",
    "ICT",      "CST",      "JST",       "KST",     "AEST/AEDT", "NZST/NZDT"};

MenuSystem::MenuSystem(MatrixDisplayManager* displayManager, SettingsManager* settingsManager,
                       ButtonManager* buttonManager, EffectsEngine* effectsEngine,
                       RTC_DS3231* rtcInstance, WiFiManager* wifiManager, TimeManager* timeManager)
    : display(displayManager),
      settings(settingsManager),
      buttons(buttonManager),
      effects(effectsEngine),
      rtc(rtcInstance),
      wifi(wifiManager),
      timeManager(timeManager),
      menuIndex(0),
      effectMenuIndex(0),
      clockColorMenuIndex(0),
      timezoneMenuIndex(0),
      setHour(0),
      setMin(0),
      setSec(0),
      inSetMode(false),
      timeSetEntryTime(0),
      lastEnterPress(0),
      entryLockProcessed(false),
      setStep(NONE),
      blockMenuReentry(false),
      enterPressTime(0),
      wasPressed(false),
      previousState(SHOW_TIME),  // Initialize in initialization list
      serialInputMode(false),
      waitingForSSID(false),
      waitingForPassword(false),
      ntpSyncState(NTP_SYNC_IDLE),
      ntpSyncMessage(""),
      ntpSyncStartTime(0),
      ntpSyncAttemptTime(0) {
    // Initialize buffer arrays
    memset(wifiSSIDBuffer, 0, sizeof(wifiSSIDBuffer));
    memset(wifiPasswordBuffer, 0, sizeof(wifiPasswordBuffer));
}

void MenuSystem::begin() {
    // Nothing specific to initialize
}
void MenuSystem::reset() {
    // Reset all menu entry state
    blockMenuReentry = true;
    enterPressTime = 0;
    wasPressed = false;

    // Reset menu state
    menuIndex = 0;

    // Reset time setting state
    inSetMode = false;
    setStep = NONE;
    entryLockProcessed = false;
}

void MenuSystem::handleMenuEntry() {
    if (blockMenuReentry) {
        if (!buttons->isEnterPressed())
            blockMenuReentry = false;
    } else {
        // Use justPressed logic for consistency with menu navigation
        if (buttons->isEnterJustPressed() && !buttons->isEnterRepeating()) {
            enterPressTime = millis();
            wasPressed = true;
            blockMenuReentry = true;  // Prevent immediate re-entry
        }
    }
}

bool MenuSystem::shouldEnterMenu() {
    // Check if we have a recent press recorded and not blocked
    if (blockMenuReentry || !wasPressed || enterPressTime == 0) {
        return false;
    }

    // Menu entry should happen immediately after the press is detected
    // Add a small debounce to prevent multiple rapid entries
    uint32_t timeSincePress = millis() - enterPressTime;
    if (timeSincePress > 50) {  // Small debounce period
        wasPressed = false;     // Reset the press flag
        return true;
    }

    return false;
}

void MenuSystem::displayMainMenu() {
    // Check if we should display an NTP sync message instead of the menu
    if (ntpSyncState != NTP_SYNC_IDLE) {
        unsigned long elapsed = millis() - ntpSyncStartTime;

        if (elapsed < NTP_SYNC_DISPLAY_DURATION) {
            // Still showing the NTP sync message
            uint16_t color;
            switch (ntpSyncState) {
                case NTP_SYNC_REQUESTED:
                case NTP_SYNC_IN_PROGRESS:
                    color = display->applyBrightness(0xFFE0);  // Yellow
                    break;
                case NTP_SYNC_SUCCESS:
                    color = display->applyBrightness(0x07E0);  // Green
                    break;
                case NTP_SYNC_ERROR:
                default:
                    color = display->applyBrightness(0xF800);  // Red
                    break;
            }
            display->drawCenteredTextWithBox(ntpSyncMessage.c_str(), 1, color);
            return;
        } else {
            // Message display time is over, return to normal menu
            ntpSyncState = NTP_SYNC_IDLE;
        }
    }

    char menuLine[32];
    strcpy(menuLine, menuItems[menuIndex]);

    // Add current values to menu items
    switch (menuIndex) {
        case 0:  // Text Size
            sprintf(menuLine + strlen(menuLine), " (%d)", settings->getTextSize());
            break;
        case 1:  // Brightness
            sprintf(menuLine + strlen(menuLine), " (%d)", settings->getBrightnessIndex() + 1);
            break;
        case 2:  // Time Format
            sprintf(menuLine + strlen(menuLine), " (%s)",
                    settings->getUse24HourFormat() ? "24H" : "12H");
            break;
        case 3:  // Clock Color
            // No value to display
            break;
        case 4:  // Effects
            sprintf(menuLine + strlen(menuLine), " (%s)", effectNames[settings->getEffectMode()]);
            break;
        case 5:  // Timezone
            // No value to display
            break;
        case 6:  // Set Clock
            // No value to display
            break;
        case 7:  // Sync NTP
            // No value to display
            break;
        case 8:  // WiFi Setup
            // No value to display
            break;
        case 9:  // OTA Setup
            // No value to display
            break;
        case 10:  // Exit
            // No value to display
            break;
    }

    display->drawCenteredTextWithBox(
        menuLine, 1, display->applyBrightness(0xF81F));  // Purple menus with brightness scaling
}

void MenuSystem::handleMainMenuInput() {
    if (buttons->isDownJustPressed()) {
        menuIndex = (menuIndex + 1) % MENU_ITEMS;
    }
    if (buttons->isUpJustPressed()) {
        menuIndex = (menuIndex - 1 + MENU_ITEMS) % MENU_ITEMS;
    }
    if (buttons->isEnterJustPressed() && !buttons->isEnterRepeating()) {
        switch (menuIndex) {
            case 0:
                // Will transition to EDIT_TEXT_SIZE in main code
                break;
            case 1:
                // Will transition to EDIT_BRIGHTNESS in main code
                break;
            case 2:
                // Will transition to EDIT_TIME_FORMAT in main code
                break;
            case 3:
                clockColorMenuIndex = settings->getClockColorMode();
                // Will transition to EDIT_CLOCK_COLOR in main code
                break;
            case 4:
                effectMenuIndex = settings->getEffectMode();
                // Will transition to EDIT_EFFECTS in main code
                break;
            case 5:                                                // Timezone
                timezoneMenuIndex = settings->getTimezoneIndex();  // Start with saved timezone
                // Will transition to EDIT_TIMEZONE in main code
                break;
            case 6:  // Set Clock
            {
                DateTime now = rtc->now();
                setHour = now.hour();
                setMin = now.minute();
                setSec = now.second();
                setStep = SET_HOUR;
                inSetMode = true;
                timeSetEntryTime = millis();
                lastEnterPress = 0;
                entryLockProcessed = false;
            } break;
            case 7:  // Sync NTP
            {
                // Check WiFi connectivity first
                if (!wifi || !wifi->isConnected()) {
                    // Set a state that will show the error message
                    ntpSyncState = NTP_SYNC_ERROR;
                    ntpSyncMessage = "WiFi Not Connected";
                    ntpSyncStartTime = millis();
                    break;
                }

                // Just request NTP sync - don't do it here!
                ntpSyncState = NTP_SYNC_REQUESTED;
                ntpSyncMessage = "Starting NTP Sync...";
                ntpSyncStartTime = millis();
                break;
            }
            case 8:  // WiFi Setup
                // Will transition to WIFI_MENU in main code
                break;
            case 9:  // OTA Setup
                // Will transition to OTA_MENU in main code
                break;
            case 10:  // Exit
                // Will transition to SHOW_TIME in main code
                break;
        }
    }
}

AppState MenuSystem::getNextState() {
    if (buttons->isEnterJustPressed() && !buttons->isEnterRepeating()) {
        switch (menuIndex) {
            case 0:
                return EDIT_TEXT_SIZE;
            case 1:
                return EDIT_BRIGHTNESS;
            case 2:
                return EDIT_TIME_FORMAT;
            case 3:
                return EDIT_CLOCK_COLOR;
            case 4:
                return EDIT_EFFECTS;
            case 5:
                return EDIT_TIMEZONE;
            case 6:
                return TIME_SET;
            case 7:
                return SYNC_NTP;
            case 8:
                return WIFI_MENU;
            case 9:
                return OTA_MENU;
            case 10:
                // Exiting to previous screen - reset menu entry state
                blockMenuReentry = true;
                wasPressed = false;
                enterPressTime = 0;
                return previousState;
        }
    }
    return MENU;  // Stay in menu
}

void MenuSystem::displayEffectsMenu() {
    // Store current effect mode
    EffectMode originalMode = settings->getEffectMode();

    // Temporarily set effect mode to preview the selected effect
    settings->setEffectMode((EffectMode)effectMenuIndex);

    // Enable menu preview mode for correct text bounding
    effects->setMenuPreviewMode(true, 1);

    // Render the preview effect
    if (settings->getEffectMode() != EFFECT_OFF) {
        effects->updateEffects();
        display->drawTextBackground(1);  // Draw background for text size 1
    } else {
        // Clear screen for "off" effect
        display->fillScreen(0);
    }

    // Disable menu preview mode
    effects->setMenuPreviewMode(false);

    // Restore original effect mode
    settings->setEffectMode(originalMode);

    // Draw menu text on top
    char menuLine[32];
    strcpy(menuLine, effectNames[effectMenuIndex]);

    // Add indicator if this is the currently active effect
    if (effectMenuIndex == settings->getEffectMode()) {
        sprintf(menuLine + strlen(menuLine), " *");
    }

    display->drawCenteredTextWithBox(
        menuLine, 1, display->applyBrightness(0xF81F));  // Purple with brightness scaling
}

void MenuSystem::handleEffectsMenuInput() {
    if (buttons->isDownJustPressed()) {
        effectMenuIndex = (effectMenuIndex + 1) % EFFECT_OPTIONS;
    }
    if (buttons->isUpJustPressed()) {
        effectMenuIndex = (effectMenuIndex - 1 + EFFECT_OPTIONS) % EFFECT_OPTIONS;
    }
    if (buttons->isEnterJustPressed() && !buttons->isEnterRepeating()) {
        settings->setEffectMode((EffectMode)effectMenuIndex);
        settings->saveSettings();  // Save the new effect mode
        // Will transition back to MENU in main code
    }
}

AppState MenuSystem::getEffectsMenuNextState() {
    if (buttons->isEnterJustPressed() && !buttons->isEnterRepeating()) {
        return MENU;
    }
    return EDIT_EFFECTS;  // Stay in effects menu
}

void MenuSystem::displayTextSizeMenu() {
    char settingStr[24];
    sprintf(settingStr, "Text Size: %d", settings->getTextSize());
    display->drawCenteredTextWithBox(
        settingStr, 1, display->applyBrightness(0xF81F));  // Purple with brightness scaling
}

void MenuSystem::handleTextSizeInput() {
    bool settingsChanged = false;

    if (buttons->isUpJustPressed() && settings->getTextSize() < TEXT_SIZE_MAX) {
        settings->setTextSize(settings->getTextSize() + 1);
        settingsChanged = true;
    }
    if (buttons->isDownJustPressed() && settings->getTextSize() > TEXT_SIZE_MIN) {
        settings->setTextSize(settings->getTextSize() - 1);
        settingsChanged = true;
    }
    if (buttons->isEnterJustPressed() && !buttons->isEnterRepeating()) {
        // Will transition back to MENU in main code
    }

    // Save settings if they changed
    if (settingsChanged) {
        settings->saveSettings();
    }
}

AppState MenuSystem::getTextSizeMenuNextState() {
    if (buttons->isEnterJustPressed() && !buttons->isEnterRepeating()) {
        return MENU;
    }
    return EDIT_TEXT_SIZE;  // Stay in text size menu
}

void MenuSystem::displayBrightnessMenu() {
    char settingStr[24];
    sprintf(settingStr, "Brightness: %d", settings->getBrightnessIndex() + 1);
    display->drawCenteredTextWithBox(
        settingStr, 1, display->applyBrightness(0xF81F));  // Purple with brightness scaling
}

void MenuSystem::handleBrightnessInput() {
    bool settingsChanged = false;

    if (buttons->isUpJustPressed() && settings->getBrightnessIndex() < BRIGHTNESS_LEVELS - 1) {
        settings->setBrightnessIndex(settings->getBrightnessIndex() + 1);
        settingsChanged = true;
    }
    if (buttons->isDownJustPressed() && settings->getBrightnessIndex() > 0) {
        settings->setBrightnessIndex(settings->getBrightnessIndex() - 1);
        settingsChanged = true;
    }
    if (buttons->isEnterJustPressed() && !buttons->isEnterRepeating()) {
        // Will transition back to MENU in main code
    }

    // Save settings if they changed
    if (settingsChanged) {
        settings->saveSettings();
    }
}

AppState MenuSystem::getBrightnessMenuNextState() {
    if (buttons->isEnterJustPressed() && !buttons->isEnterRepeating()) {
        return MENU;
    }
    return EDIT_BRIGHTNESS;  // Stay in brightness menu
}

void MenuSystem::displayTimeFormatMenu() {
    const char* formatStr = settings->getUse24HourFormat() ? "24 Hour" : "12 Hour";
    char settingStr[24];
    sprintf(settingStr, "Format: %s", formatStr);
    display->drawCenteredTextWithBox(
        settingStr, 1, display->applyBrightness(0xF81F));  // Purple with brightness scaling
}

void MenuSystem::handleTimeFormatInput() {
    bool settingsChanged = false;

    if ((buttons->isUpJustPressed() || buttons->isDownJustPressed()) &&
        !(buttons->isUpRepeating() || buttons->isDownRepeating())) {
        settings->setUse24HourFormat(!settings->getUse24HourFormat());
        settingsChanged = true;
    }
    if (buttons->isEnterJustPressed() && !buttons->isEnterRepeating()) {
        // Will transition back to MENU in main code
    }

    // Save settings if they changed
    if (settingsChanged) {
        settings->saveSettings();
    }
}

AppState MenuSystem::getTimeFormatMenuNextState() {
    if (buttons->isEnterJustPressed() && !buttons->isEnterRepeating()) {
        return MENU;
    }
    return EDIT_TIME_FORMAT;  // Stay in time format menu
}

void MenuSystem::displayClockColorMenu() {
    // Save current mode and temporarily set to preview mode
    ClockColorMode tempMode = settings->getClockColorMode();
    settings->setClockColorMode((ClockColorMode)clockColorMenuIndex);

    // Draw preview time with fixed text size 2
    uint16_t color = display->getClockColor();
    display->drawTightClock("12:34:56", 2, color);

    // Show the color name at the bottom
    int nameY = MATRIX_HEIGHT - 9;
    display->drawCenteredTextWithBox(clockColorNames[clockColorMenuIndex], 1,
                                     display->applyBrightness(0xF81F), 0x0000,
                                     nameY);  // Purple with black box

    // Restore original mode
    settings->setClockColorMode(tempMode);
}

void MenuSystem::handleClockColorInput() {
    bool settingsChanged = false;

    if (buttons->isDownJustPressed()) {
        clockColorMenuIndex = (clockColorMenuIndex + 1) % CLOCK_COLOR_OPTIONS;
        settingsChanged = true;
    }
    if (buttons->isUpJustPressed()) {
        clockColorMenuIndex = (clockColorMenuIndex - 1 + CLOCK_COLOR_OPTIONS) % CLOCK_COLOR_OPTIONS;
        settingsChanged = true;
    }
    if (buttons->isEnterJustPressed() && !buttons->isEnterRepeating()) {
        settings->setClockColorMode((ClockColorMode)clockColorMenuIndex);
        settings->saveSettings();
        // Will transition back to MENU in main code
    }

    // Update the preview immediately
    if (settingsChanged) {
        // No need to save here since we only save on ENTER
    }
}

AppState MenuSystem::getClockColorMenuNextState() {
    if (buttons->isEnterJustPressed() && !buttons->isEnterRepeating()) {
        return MENU;
    }
    return EDIT_CLOCK_COLOR;  // Stay in clock color menu
}

void MenuSystem::displayTimezoneMenu() {
    // Display the city/region name at the top
    display->drawCenteredText(timezoneNames[timezoneMenuIndex], 1, display->applyBrightness(0xFFE0),
                              6);  // Yellow

    // Display the timezone code in the middle
    display->drawCenteredText(timezoneCodes[timezoneMenuIndex], 1, display->applyBrightness(0x07FF),
                              14);  // Cyan

    // Display the UTC offset at the bottom
    char offsetStr[32];
    int offset = timezoneOffsets[timezoneMenuIndex];
    bool hasDST = timezoneDST[timezoneMenuIndex];
    int dstOffset = timezoneDSTOffset[timezoneMenuIndex];

    if (hasDST && dstOffset > 0) {
        sprintf(offsetStr, "UTC%+d/%+d (DST)", offset, offset + dstOffset);
    } else {
        sprintf(offsetStr, "UTC%+d", offset);
    }

    display->drawCenteredText(offsetStr, 1, display->applyBrightness(0xF81F), 22);  // Purple
}

void MenuSystem::handleTimezoneInput() {
    bool settingsChanged = false;

    if (buttons->isDownJustPressed()) {
        timezoneMenuIndex = (timezoneMenuIndex + 1) % TIMEZONE_OPTIONS;
        settingsChanged = true;
    }
    if (buttons->isUpJustPressed()) {
        timezoneMenuIndex = (timezoneMenuIndex - 1 + TIMEZONE_OPTIONS) % TIMEZONE_OPTIONS;
        settingsChanged = true;
    }
    if (buttons->isEnterJustPressed() && !buttons->isEnterRepeating()) {
        // Apply the timezone change to TimeManager using offset approach
        timeManager->setTimezoneOffset(timezoneOffsets[timezoneMenuIndex],
                                       timezoneDST[timezoneMenuIndex],
                                       timezoneDSTOffset[timezoneMenuIndex]);

        // Save the timezone index to settings
        settings->setTimezoneIndex(timezoneMenuIndex);
        settings->saveSettings();
        // Will transition back to MENU in main code
    }

    // Update the preview immediately
    if (settingsChanged) {
        // No need to save here since we only save on ENTER
    }
}

AppState MenuSystem::getTimezoneMenuNextState() {
    if (buttons->isEnterJustPressed() && !buttons->isEnterRepeating()) {
        return MENU;
    }
    return EDIT_TIMEZONE;  // Stay in timezone menu
}

void MenuSystem::handleTimeSettingMode() {
    static bool blinkState = true;
    static uint32_t lastBlink = 0;

    // Handle blinking at 500ms intervals
    if (millis() - lastBlink > 500) {
        blinkState = !blinkState;
        lastBlink = millis();
    }

    // Button lock on initial entry to prevent menu bleed-through
    if (setStep == SET_HOUR && !entryLockProcessed &&
        millis() - timeSetEntryTime < BUTTON_LOCK_DURATION) {
        // Just display during lock period, don't process buttons
        char displayStr[12];
        if (blinkState) {
            snprintf(displayStr, sizeof(displayStr), "%02d:%02d:%02d", setHour, setMin, setSec);
        } else {
            snprintf(displayStr, sizeof(displayStr), "  :%02d:%02d", setMin, setSec);
        }
        display->drawTightClock(displayStr, settings->getTextSize(),
                                display->getTextColors()[settings->getBrightnessIndex()]);
        return;
    }

    // Mark entry lock as processed after first check
    if (setStep == SET_HOUR && !entryLockProcessed) {
        entryLockProcessed = true;
    }

    // Handle button inputs - allow repeating for scrolling, but consume each press
    if (buttons->isUpJustPressed()) {
        buttons->clearUpJustPressed();  // Immediately consume the button press
        Serial.print("UP pressed (consumed)! ");
        switch (setStep) {
            case SET_HOUR:
                setHour = (setHour + 1) % 24;
                Serial.print("Hour: ");
                Serial.println(setHour);
                break;
            case SET_MINUTE:
                setMin = (setMin + 1) % 60;
                break;
            case SET_SECOND:
                setSec = (setSec + 1) % 60;
                break;
        }
    }

    if (buttons->isDownJustPressed()) {
        buttons->clearDownJustPressed();
        switch (setStep) {
            case SET_HOUR:
                setHour = (setHour + 23) % 24;
                break;
            case SET_MINUTE:
                setMin = (setMin + 59) % 60;
                break;
            case SET_SECOND:
                setSec = (setSec + 59) % 60;
                break;
        }
    }

    // Handle field progression
    if (buttons->isEnterJustPressed() && !buttons->isEnterRepeating() &&
        millis() - lastEnterPress > 150) {
        buttons->clearEnterJustPressed();
        lastEnterPress = millis();

        switch (setStep) {
            case SET_HOUR:
                setStep = SET_MINUTE;
                break;
            case SET_MINUTE:
                setStep = SET_SECOND;
                break;
            case SET_SECOND:
                rtc->adjust(DateTime(2024, 1, 1, setHour, setMin, setSec));
                setStep = NONE;
                inSetMode = false;
                break;
            default:
                break;
        }
    }

    // Display time with appropriate blinking field (this should run every frame)
    char displayStr[12];
    switch (setStep) {
        case SET_HOUR:
            if (blinkState) {
                snprintf(displayStr, sizeof(displayStr), "%02d:%02d:%02d", setHour, setMin, setSec);
            } else {
                snprintf(displayStr, sizeof(displayStr), "  :%02d:%02d", setMin, setSec);
            }
            break;
        case SET_MINUTE:
            if (blinkState) {
                snprintf(displayStr, sizeof(displayStr), "%02d:%02d:%02d", setHour, setMin, setSec);
            } else {
                snprintf(displayStr, sizeof(displayStr), "%02d:  :%02d", setHour, setSec);
            }
            break;
        case SET_SECOND:
            if (blinkState) {
                snprintf(displayStr, sizeof(displayStr), "%02d:%02d:%02d", setHour, setMin, setSec);
            } else {
                snprintf(displayStr, sizeof(displayStr), "%02d:%02d:  ", setHour, setMin);
            }
            break;
        default:
            snprintf(displayStr, sizeof(displayStr), "%02d:%02d:%02d", setHour, setMin, setSec);
            break;
    }

    display->drawTightClock(displayStr, settings->getTextSize(),
                            display->getTextColors()[settings->getBrightnessIndex()]);
}

AppState MenuSystem::getTimeSettingNextState() {
    if (setStep == NONE && !inSetMode) {
        // Exiting to previous screen - reset menu entry state
        blockMenuReentry = true;
        wasPressed = false;
        enterPressTime = 0;
        return previousState;
    }
    return TIME_SET;  // Stay in time setting mode
}

void MenuSystem::handleInput(AppState& appState) {
    switch (appState) {
        case SHOW_TIME:
        case SHOW_TIME_WITH_DATE:
        case SHOW_WIFI_INFO:
            handleMenuEntry();
            if (shouldEnterMenu()) {
                previousState = appState;  // Remember which screen we came from
                appState = MENU;
                menuIndex = 0;
                // Reset menu entry state to prevent immediate re-entry
                blockMenuReentry = true;
                wasPressed = false;
            }
            break;

        case MENU:
            handleMainMenuInput();
            appState = getNextState();
            break;

        case EDIT_TEXT_SIZE:
            handleTextSizeInput();
            appState = getTextSizeMenuNextState();
            break;

        case EDIT_BRIGHTNESS:
            handleBrightnessInput();
            appState = getBrightnessMenuNextState();
            break;

        case EDIT_TIME_FORMAT:
            handleTimeFormatInput();
            appState = getTimeFormatMenuNextState();
            break;

        case EDIT_CLOCK_COLOR:
            handleClockColorInput();
            appState = getClockColorMenuNextState();
            break;

        case EDIT_EFFECTS:
            handleEffectsMenuInput();
            appState = getEffectsMenuNextState();
            break;

        case EDIT_TIMEZONE:
            handleTimezoneInput();
            appState = getTimezoneMenuNextState();
            break;

        case TIME_SET:
            handleTimeSettingMode();
            appState = getTimeSettingNextState();
            break;

        case SYNC_NTP:
            // NTP sync state - transition back to menu and let menu handle display
            appState = MENU;
            break;

        case WIFI_MENU:
            // Handle DOWN button long press for forget network
            static uint32_t downPressStartTime = 0;
            static bool downLongPressProcessed = false;

            if (buttons->isDownPressed()) {
                if (downPressStartTime == 0) {
                    downPressStartTime = millis();
                    downLongPressProcessed = false;
                }
                // Check for 5 second hold
                if (!downLongPressProcessed && (millis() - downPressStartTime) > 5000) {
                    // Forget network and enter serial mode
                    settings->setWiFiCredentials("", "");
                    settings->setWiFiEnabled(false);
                    settings->saveSettings();
                    startSerialWiFiSetup();
                    downLongPressProcessed = true;
                    break;
                }
            } else {
                downPressStartTime = 0;
                downLongPressProcessed = false;
            }

            // Handle UP button to toggle WiFi or enter serial mode
            if (buttons->isUpJustPressed()) {
                if (strlen(settings->getWiFiSSID()) > 0) {
                    // Toggle WiFi on/off (keep stored credentials)
                    bool newState = !settings->isWiFiEnabled();
                    settings->setWiFiEnabled(newState);
                    settings->saveSettings();

                    if (newState) {
                        // If we just enabled WiFi, try to connect immediately
                        wifi->reconnectWithNewCredentials(settings->getWiFiSSID(),
                                                          settings->getWiFiPassword());
                    } else {
                        // If we just disabled WiFi, disconnect from network
                        wifi->disconnect();
                    }
                } else {
                    // No stored credentials, enter serial setup mode
                    startSerialWiFiSetup();
                }
            }
            // Handle ENTER button to save and leave menu
            if (buttons->isEnterJustPressed()) {
                appState = MENU;
            }
            break;

        case OTA_MENU:
            // Handle ENTER button to return to main menu
            if (buttons->isEnterJustPressed()) {
                appState = MENU;
            }
            break;
    }
}

void MenuSystem::updateDisplay(AppState appState) {
    // Don't update display if OTA is in progress (to allow screen blanking)
    if (wifi && wifi->isOTAInProgress()) {
        return;
    }

    switch (appState) {
        case MENU:
            buttons->setAllowButtonRepeat(false);  // Disable repeating for menu navigation
            displayMainMenu();
            break;

        case EDIT_TEXT_SIZE:
            buttons->setAllowButtonRepeat(false);  // Disable repeating for text size menu
            displayTextSizeMenu();
            break;

        case EDIT_BRIGHTNESS:
            buttons->setAllowButtonRepeat(false);  // Disable repeating for brightness menu
            displayBrightnessMenu();
            break;

        case EDIT_TIME_FORMAT:
            buttons->setAllowButtonRepeat(false);  // Disable repeating for time format menu
            displayTimeFormatMenu();
            break;

        case EDIT_CLOCK_COLOR:
            buttons->setAllowButtonRepeat(false);  // Disable repeating for clock color menu
            displayClockColorMenu();
            break;

        case EDIT_EFFECTS:
            buttons->setAllowButtonRepeat(false);  // Disable repeating for effects menu
            displayEffectsMenu();
            break;

        case EDIT_TIMEZONE:
            buttons->setAllowButtonRepeat(false);  // Disable repeating for timezone menu
            displayTimezoneMenu();
            break;

        case TIME_SET:
            buttons->setAllowButtonRepeat(true);  // Enable repeating for time setting
            handleTimeSettingMode();
            break;

        case SYNC_NTP:
            // NTP sync state - just display the menu (which handles NTP sync status)
            buttons->setAllowButtonRepeat(false);
            displayMainMenu();
            break;

        case WIFI_MENU:
            buttons->setAllowButtonRepeat(false);
            displayWiFiMenu();
            // Handle serial WiFi input if active
            if (serialInputMode) {
                handleSerialWiFiInput();
                // Return to normal WiFi menu when serial input is complete
                if (!serialInputMode) {
                    // Input complete, stay in WiFi menu to show updated status
                }
            }
            break;

        case OTA_MENU:
            buttons->setAllowButtonRepeat(false);
            displayOTAMenu();
            break;
    }
}

// WiFi Menu Implementation
void MenuSystem::displayWiFiMenu() {
    static const char* instructionText = "";

    // Update instruction text based on current state
    if (serialInputMode) {
        if (waitingForSSID) {
            instructionText = "Enter SSID via Serial Monitor";
        } else if (waitingForPassword) {
            instructionText = "Enter Password via Serial (or Enter for open network)";
        } else {
            instructionText = "Serial Setup Complete - Connecting to WiFi...";
        }
    } else {
        if (strlen(settings->getWiFiSSID()) > 0) {
            instructionText =
                "UP = WiFi ON/OFF | HOLD DOWN (5sec) = Forget Network | E = Exit Menu";
        } else {
            instructionText = "UP = Setup WiFi via Serial | ENTER = Exit Menu";
        }
    }

    // === TOP: Connection Status ===
    char statusLine[64];
    uint16_t statusColor;

    if (wifi->isConnected()) {
        // Just show the SSID in green when connected
        snprintf(statusLine, sizeof(statusLine), "%s", settings->getWiFiSSID());
        statusColor = display->applyBrightness(0x07E0);  // Green
    } else if (strlen(settings->getWiFiSSID()) > 0) {
        // Just show "DISCONNECTED" when have credentials but not connected
        strcpy(statusLine, "DISCONNECTED");
        statusColor = display->applyBrightness(0xF800);  // Red
    } else {
        // No stored credentials
        strcpy(statusLine, "NO NETWORK CONFIGURED");
        statusColor = display->applyBrightness(0xF800);  // Red
    }

    display->drawCenteredTextWithBox(statusLine, 1, statusColor, 0x0000, 2);

    // === MIDDLE: MAC Address ===
    String macAddress = WiFi.macAddress();
    // Center the MAC address in the middle section (between top status and bottom instructions)
    int macY = (MATRIX_HEIGHT / 2) - 4;  // Center of screen minus half text height
    display->drawCenteredText(macAddress.c_str(), 1, display->applyBrightness(0xFFE0),
                              macY);  // Yellow

    // === BOTTOM: Scrolling Instructions ===
    int textWidth = strlen(instructionText) * 6;  // Approximate character width
    int displayWidth = MATRIX_WIDTH;

    // Only scroll if text is wider than display
    if (textWidth > displayWidth) {
        static uint32_t scrollTime = 0;
        static int scrollOffset = 0;

        if (millis() - scrollTime > 80) {  // Scroll every 80ms for smooth movement
            scrollOffset++;
            if (scrollOffset > textWidth + 10) {  // Reset for endless scrolling
                scrollOffset = 0;                 // Reset to start immediately for endless loop
            }
            scrollTime = millis();
        }

        // Draw scrolling text
        display->setTextColor(display->applyBrightness(0xF81F));  // Purple to match menu options
        display->setTextSize(1);
        display->setCursor(displayWidth - scrollOffset, MATRIX_HEIGHT - 8);
        display->print(instructionText);
    } else {
        // Text fits, just center it
        display->drawCenteredText(instructionText, 1, display->applyBrightness(0xF81F),
                                  MATRIX_HEIGHT - 8);  // Purple to match menu options
    }
}

void MenuSystem::displayOTAMenu() {
    if (wifi->isConnected()) {
        // Show IP address
        char ipLine[32];
        snprintf(ipLine, sizeof(ipLine), "%s", wifi->getIPAddress().c_str());
        display->drawCenteredText(ipLine, 1, display->applyBrightness(0x07FF), 10);  // Cyan

        // Show current OTA password
        String otaPassword = wifi->getOTAPassword();
        char passwordLine[32];
        snprintf(passwordLine, sizeof(passwordLine), "%s", otaPassword.c_str());
        display->drawCenteredText(passwordLine, 1, display->applyBrightness(0xFFE0), 18);  // Yellow
    } else {
        display->drawCenteredText("WiFi Not Connected", 1, display->applyBrightness(0xF800),
                                  16);  // Red
    }
}

void MenuSystem::startSerialWiFiSetup() {
    serialInputMode = true;
    waitingForSSID = true;
    waitingForPassword = false;

    Serial.println();
    Serial.println("=== WiFi Setup via Serial ===");
    Serial.println("Enter WiFi SSID (network name):");
    Serial.print("> ");

    // Clear any existing input
    while (Serial.available()) {
        Serial.read();
    }
}

void MenuSystem::handleSerialWiFiInput() {
    if (!serialInputMode)
        return;

    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();  // Remove whitespace

        if (waitingForSSID) {
            if (input.length() > 0 && input.length() < 32) {
                strncpy(wifiSSIDBuffer, input.c_str(), 31);
                wifiSSIDBuffer[31] = '\0';

                Serial.println("SSID: " + input);
                Serial.println("Enter WiFi Password (or press Enter for open network):");
                Serial.print("> ");

                waitingForSSID = false;
                waitingForPassword = true;
            } else {
                Serial.println("Invalid SSID length (1-31 characters). Try again:");
                Serial.print("> ");
            }
        } else if (waitingForPassword) {
            if (input.length() < 64) {  // Allow empty password for open networks
                strncpy(wifiPasswordBuffer, input.c_str(), 63);
                wifiPasswordBuffer[63] = '\0';

                if (input.length() == 0) {
                    Serial.println("Password: (none - open network)");
                } else {
                    Serial.println("Password: " +
                                   String('*').substring(0, input.length()));  // Show asterisks
                }
                Serial.println();
                Serial.println("WiFi credentials saved!");
                Serial.println("SSID: " + String(wifiSSIDBuffer));
                if (strlen(wifiPasswordBuffer) == 0) {
                    Serial.println("Password: (none - open network)");
                } else {
                    Serial.println("Password: " +
                                   String('*').substring(0, strlen(wifiPasswordBuffer)));
                }

                // Save credentials
                settings->setWiFiCredentials(wifiSSIDBuffer, wifiPasswordBuffer);
                settings->saveSettings();

                // Try to connect immediately instead of waiting for restart
                Serial.println("Attempting to connect to WiFi...");
                wifi->reconnectWithNewCredentials(wifiSSIDBuffer, wifiPasswordBuffer);

                // Reset state
                serialInputMode = false;
                waitingForSSID = false;
                waitingForPassword = false;

                // Check connection status and inform user
                if (wifi->isConnected()) {
                    Serial.println("WiFi connected successfully!");
                    Serial.println("IP Address: " + wifi->getIPAddress());
                } else {
                    Serial.println("WiFi connection failed. Check credentials and try again.");
                    Serial.println("You can also restart the device to retry connection.");
                }
            } else {
                Serial.println("Invalid password length (0-63 characters). Try again:");
                Serial.print("> ");
            }
        }
    }
}

// ===================== NTP SYNC RESULT SETTER =====================

void MenuSystem::setNTPSyncResult(bool success) {
    if (success) {
        ntpSyncState = NTP_SYNC_SUCCESS;
        ntpSyncMessage = "NTP Sync Success";
    } else {
        ntpSyncState = NTP_SYNC_ERROR;
        ntpSyncMessage = "NTP Sync Failed";
    }
    ntpSyncStartTime = millis();
}
