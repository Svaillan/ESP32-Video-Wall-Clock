#include "MenuSystem.h"

// Static menu data
const char* MenuSystem::menuItems[] = { "Text Size", "Brightness", "Time Format", "Clock Color", "Effects", "Set Clock", "Exit" };
const int MenuSystem::MENU_ITEMS = sizeof(menuItems) / sizeof(menuItems[0]);
const char* MenuSystem::effectNames[] = {"Confetti", "Acid", "Rain", "Torrent", "Stars", "Sparkles", "Fireworks", "Tron", "Off"};
const int MenuSystem::EFFECT_OPTIONS = sizeof(effectNames) / sizeof(effectNames[0]);
const char* MenuSystem::clockColorNames[] = {"White", "Red", "Green", "Blue", "Yellow", "Cyan", "Magenta", "Orange", "Purple", "Pink", "Lime", "Teal", "Indigo", "Gold", "Silver", "Rainbow"};
const int MenuSystem::CLOCK_COLOR_OPTIONS = sizeof(clockColorNames) / sizeof(clockColorNames[0]);

MenuSystem::MenuSystem(MatrixDisplayManager* displayManager, SettingsManager* settingsManager, 
                       ButtonManager* buttonManager, EffectsEngine* effectsEngine, RTC_DS3231* rtcInstance) {
    display = displayManager;
    settings = settingsManager;
    buttons = buttonManager;
    effects = effectsEngine;
    rtc = rtcInstance;
    
    // Initialize menu state
    menuIndex = 0;
    effectMenuIndex = 0;
    clockColorMenuIndex = 0;
    
    // Initialize time setting state
    setHour = 0;
    setMin = 0;
    setSec = 0;
    inSetMode = false;
    timeSetEntryTime = 0;
    lastEnterPress = 0;
    entryLockProcessed = false;
    setStep = NONE;
    
    // Initialize menu entry state
    blockMenuReentry = false;
    enterPressTime = 0;
    wasPressed = false;
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
        if (!buttons->isEnterPressed()) blockMenuReentry = false;
    } else {
        if (buttons->isEnterPressed() && !wasPressed) {
            enterPressTime = millis();
            wasPressed = true;
        }
        if (!buttons->isEnterPressed() && wasPressed) {
            if (millis() - enterPressTime < 1000) {
                // Will transition to MENU state in main code
            }
            wasPressed = false;
            blockMenuReentry = true;
        }
    }
}

bool MenuSystem::shouldEnterMenu() {
    // Don't enter menu if blocked or if we haven't recorded a valid press time
    if (blockMenuReentry || wasPressed || enterPressTime == 0) {
        return false;
    }
    
    uint32_t timeSincePress = millis() - enterPressTime;
    return (timeSincePress < 1000 && timeSincePress > 50); // Small debounce
}

void MenuSystem::displayMainMenu() {
    char menuLine[32];
    strcpy(menuLine, menuItems[menuIndex]);
    
    // Add current values to menu items
    switch (menuIndex) {
        case 0: // Text Size
            sprintf(menuLine + strlen(menuLine), " (%d)", settings->getTextSize());
            break;
        case 1: // Brightness
            sprintf(menuLine + strlen(menuLine), " (%d)", settings->getBrightnessIndex() + 1);
            break;
        case 2: // Time Format
            sprintf(menuLine + strlen(menuLine), " (%s)", settings->getUse24HourFormat() ? "24H" : "12H");
            break;
        case 3: // Clock Color
            sprintf(menuLine + strlen(menuLine), " (%s)", clockColorNames[settings->getClockColorMode()]);
            break;
        case 4: // Effects
            sprintf(menuLine + strlen(menuLine), " (%s)", effectNames[settings->getEffectMode()]);
            break;
    }
    
    display->drawCenteredTextWithBox(menuLine, 1, display->applyBrightness(0xF81F));  // Purple menus with brightness scaling
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
            case 5: // Set Clock
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
                }
                break;
            case 6: // Exit
                // Will transition to SHOW_TIME in main code
                break;
        }
    }
}

AppState MenuSystem::getNextState() {
    if (buttons->isEnterJustPressed() && !buttons->isEnterRepeating()) {
        switch (menuIndex) {
            case 0: return EDIT_TEXT_SIZE;
            case 1: return EDIT_BRIGHTNESS;
            case 2: return EDIT_TIME_FORMAT;
            case 3: return EDIT_CLOCK_COLOR;
            case 4: return EDIT_EFFECTS;
            case 5: return TIME_SET;
            case 6: 
                // Exiting to time display - reset menu entry state
                blockMenuReentry = true;
                wasPressed = false;
                enterPressTime = 0;
                return SHOW_TIME;
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
    
    display->drawCenteredTextWithBox(menuLine, 1, display->applyBrightness(0xF81F));  // Purple with brightness scaling
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
    display->drawCenteredTextWithBox(settingStr, 1, display->applyBrightness(0xF81F));  // Purple with brightness scaling
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
    display->drawCenteredTextWithBox(settingStr, 1, display->applyBrightness(0xF81F));  // Purple with brightness scaling
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
    display->drawCenteredTextWithBox(settingStr, 1, display->applyBrightness(0xF81F));  // Purple with brightness scaling
}

void MenuSystem::handleTimeFormatInput() {
    bool settingsChanged = false;
    
    if ((buttons->isUpJustPressed() || buttons->isDownJustPressed()) && !(buttons->isUpRepeating() || buttons->isDownRepeating())) {
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
    display->drawCenteredTextWithBox(clockColorNames[clockColorMenuIndex], 1, display->applyBrightness(0xF81F), 0x0000, nameY);  // Purple with black box
    
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

void MenuSystem::handleTimeSettingMode() {
    static bool blinkState = true;
    static uint32_t lastBlink = 0;
    static bool debugPrinted = false;
    static SetClockStep lastSetStep = NONE;
    
    if (setStep != lastSetStep) {
        debugPrinted = false;
        lastSetStep = setStep;
    }
    
    if (!debugPrinted) {
        debugPrinted = true;
    }
    
    // Handle blinking at 500ms intervals
    if (millis() - lastBlink > 500) {
        blinkState = !blinkState;
        lastBlink = millis();
    }
    
    // Button lock on initial entry to prevent menu bleed-through
    if (setStep == SET_HOUR && !entryLockProcessed && millis() - timeSetEntryTime < BUTTON_LOCK_DURATION) {
        // Just display during lock period, don't process buttons
        char displayStr[12];
        if (blinkState) {
            snprintf(displayStr, sizeof(displayStr), "%02d:%02d:%02d", setHour, setMin, setSec);
        } else {
            snprintf(displayStr, sizeof(displayStr), "  :%02d:%02d", setMin, setSec);
        }
        display->drawTightClock(displayStr, settings->getTextSize(), display->getTextColors()[settings->getBrightnessIndex()]);
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
                Serial.print("Hour: "); Serial.println(setHour);
                break;
            case SET_MINUTE: 
                setMin = (setMin + 1) % 60; 
                Serial.print("Minute: "); Serial.println(setMin);
                break;
            case SET_SECOND: 
                setSec = (setSec + 1) % 60; 
                Serial.print("Second: "); Serial.println(setSec);
                break;
        }
    }
    
    if (buttons->isDownJustPressed()) {
        buttons->clearDownJustPressed();  // Immediately consume the button press
        Serial.print("DOWN pressed (consumed)! ");
        switch (setStep) {
            case SET_HOUR:   
                setHour = (setHour + 23) % 24; 
                Serial.print("Hour: "); Serial.println(setHour);
                break;
            case SET_MINUTE: 
                setMin = (setMin + 59) % 60; 
                Serial.print("Minute: "); Serial.println(setMin);
                break;
            case SET_SECOND: 
                setSec = (setSec + 59) % 60; 
                Serial.print("Second: "); Serial.println(setSec);
                break;
        }
    }
    
    // Handle field progression
    if (buttons->isEnterJustPressed() && !buttons->isEnterRepeating() && millis() - lastEnterPress > 150) {
        buttons->clearEnterJustPressed();  // Consume the button press
        lastEnterPress = millis();
        Serial.print("ENTER pressed in time setting! Current step: ");
        Serial.println(setStep);
        
        switch (setStep) {
            case SET_HOUR:
                setStep = SET_MINUTE;
                Serial.println("Advancing to SET_MINUTE");
                break;
            case SET_MINUTE:
                setStep = SET_SECOND;
                Serial.println("Advancing to SET_SECOND");
                break;
            case SET_SECOND:
                // Apply the set time to RTC
                Serial.println("Setting RTC time and exiting");
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
    
    display->drawTightClock(displayStr, settings->getTextSize(), display->getTextColors()[settings->getBrightnessIndex()]);
}

AppState MenuSystem::getTimeSettingNextState() {
    if (setStep == NONE && !inSetMode) {
        // Exiting to time display - reset menu entry state
        blockMenuReentry = true;
        wasPressed = false;
        enterPressTime = 0;
        return SHOW_TIME;
    }
    return TIME_SET;  // Stay in time setting mode
}

void MenuSystem::handleInput(AppState& appState) {
    switch (appState) {
        case SHOW_TIME:
            handleMenuEntry();
            if (shouldEnterMenu()) {
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
            
        case TIME_SET:
            handleTimeSettingMode();
            appState = getTimeSettingNextState();
            break;
    }
}

void MenuSystem::updateDisplay(AppState appState) {
    switch (appState) {
        case MENU:
            buttons->setAllowButtonRepeat(false); // Disable repeating for menu navigation
            displayMainMenu();
            break;
            
        case EDIT_TEXT_SIZE:
            buttons->setAllowButtonRepeat(false); // Disable repeating for text size menu
            displayTextSizeMenu();
            break;
            
        case EDIT_BRIGHTNESS:
            buttons->setAllowButtonRepeat(false); // Disable repeating for brightness menu
            displayBrightnessMenu();
            break;
            
        case EDIT_TIME_FORMAT:
            buttons->setAllowButtonRepeat(false); // Disable repeating for time format menu
            displayTimeFormatMenu();
            break;
            
        case EDIT_CLOCK_COLOR:
            buttons->setAllowButtonRepeat(false); // Disable repeating for clock color menu
            displayClockColorMenu();
            break;
            
        case EDIT_EFFECTS:
            buttons->setAllowButtonRepeat(false); // Disable repeating for effects menu
            displayEffectsMenu();
            break;
            
        case TIME_SET:
            buttons->setAllowButtonRepeat(true); // Enable repeating for time setting
            handleTimeSettingMode();
            break;
    }
}
