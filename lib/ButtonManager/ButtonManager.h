#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <Arduino.h>

// Button Pin Configuration
#define PIN_BTN_UP 33
#define PIN_BTN_DOWN 32
#define PIN_BTN_ENTER 18

// Timing Constants
#define DEBOUNCE_DELAY 10        // Reduced from 15ms for even faster response
#define BUTTON_REPEAT_DELAY 500  // Increased from 120ms - longer delay before repeat starts
#define BUTTON_REPEAT_RATE 60    // Reduced from 80ms - faster repeat rate

struct ButtonState {
    uint8_t pin;
    bool pressed, lastPhysical, justPressed, isRepeating;
    uint32_t lastDebounce, pressStartTime, lastRepeat;

    explicit ButtonState(uint8_t p)
        : pin(p),
          pressed(false),
          lastPhysical(false),
          justPressed(false),
          isRepeating(false),
          lastDebounce(0),
          pressStartTime(0),
          lastRepeat(0) {}
};

class ButtonManager {
   public:
    // Constructor
    ButtonManager();

    // Initialization
    void begin();

    // Update all buttons (call this in main loop)
    void updateAll();

    // Button state getters
    bool isUpPressed() const {
        return btnUp.pressed;
    }
    bool isDownPressed() const {
        return btnDown.pressed;
    }
    bool isEnterPressed() const {
        return btnEnter.pressed;
    }

    bool isUpJustPressed() const {
        return btnUp.justPressed;
    }
    bool isDownJustPressed() const {
        return btnDown.justPressed;
    }
    bool isEnterJustPressed() const {
        return btnEnter.justPressed;
    }

    bool isUpRepeating() const {
        return btnUp.isRepeating;
    }
    bool isDownRepeating() const {
        return btnDown.isRepeating;
    }
    bool isEnterRepeating() const {
        return btnEnter.isRepeating;
    }

    // Clear just pressed flags (useful for consuming button events)
    void clearUpJustPressed() {
        btnUp.justPressed = false;
    }
    void clearDownJustPressed() {
        btnDown.justPressed = false;
    }
    void clearEnterJustPressed() {
        btnEnter.justPressed = false;
    }

    // Control repeat functionality
    void setAllowButtonRepeat(bool allow) {
        allowButtonRepeat = allow;
    }
    bool getAllowButtonRepeat() const {
        return allowButtonRepeat;
    }

   private:
    // Button instances
    ButtonState btnUp;
    ButtonState btnDown;
    ButtonState btnEnter;

    // Repeat control
    bool allowButtonRepeat;

    // Update individual button
    void updateButton(ButtonState& button);
};

#endif  // BUTTON_MANAGER_H
