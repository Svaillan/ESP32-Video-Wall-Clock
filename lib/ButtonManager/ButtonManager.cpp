#include "ButtonManager.h"

ButtonManager::ButtonManager() : btnUp(PIN_BTN_UP), btnDown(PIN_BTN_DOWN), btnEnter(PIN_BTN_ENTER) {
    allowButtonRepeat = false;
}

void ButtonManager::begin() {
    pinMode(PIN_BTN_UP, INPUT_PULLUP);
    pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
    pinMode(PIN_BTN_ENTER, INPUT_PULLUP);
    Serial.println("Button Manager initialized");
}

void ButtonManager::updateAll() {
    updateButton(btnUp);
    updateButton(btnDown);
    updateButton(btnEnter);
}

void ButtonManager::updateButton(ButtonState &button) {
    bool currentReading = digitalRead(button.pin) == LOW;
    uint32_t currentTime = millis();
    
    // Handle physical state changes
    if (currentReading != button.lastPhysical) {
        button.lastDebounce = currentTime;
        button.lastPhysical = currentReading;
    }
    
    // Process debounced state
    if ((currentTime - button.lastDebounce) > DEBOUNCE_DELAY) {
        bool previousPressed = button.pressed;
        button.pressed = currentReading;
        
        if (button.pressed && !previousPressed) {
            // Button just pressed
            button.justPressed = true;
            button.pressStartTime = currentTime;
            button.lastRepeat = currentTime;
            button.isRepeating = false;
        } else if (!button.pressed && previousPressed) {
            // Button just released
            button.justPressed = false;
            button.isRepeating = false;
        } else if (button.pressed && previousPressed) {
            // Button held down - check for repeat (only if repeat is allowed)
            if (allowButtonRepeat && !button.isRepeating && (currentTime - button.pressStartTime) > BUTTON_REPEAT_DELAY) {
                button.isRepeating = true;
                button.lastRepeat = currentTime;
                button.justPressed = true; // Trigger repeat
            } else if (allowButtonRepeat && button.isRepeating && (currentTime - button.lastRepeat) > BUTTON_REPEAT_RATE) {
                button.lastRepeat = currentTime;
                button.justPressed = true; // Trigger repeat
            } else {
                button.justPressed = false;
            }
        } else {
            button.justPressed = false;
        }
    } else {
        button.justPressed = false;
    }
}
