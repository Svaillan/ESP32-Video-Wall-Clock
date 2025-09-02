#ifndef MATRIX_DISPLAY_MANAGER_H
#define MATRIX_DISPLAY_MANAGER_H

#include <Arduino.h>

#include <Adafruit_Protomatter.h>

#include "SettingsManager.h"

// Matrix Display Settings
#define MATRIX_WIDTH 128
#define MATRIX_HEIGHT 32
#define BIT_DEPTH 5

// Structure for text area information
struct TextAreaInfo {
    uint16_t width;
    uint16_t height;
    int16_t boundingX;
    int16_t boundingY;
    int centeredX;
    int centeredY;
};

class MatrixDisplayManager {
   public:
    // Constructor
    MatrixDisplayManager(Adafruit_Protomatter* matrix, SettingsManager* settings);

    // Initialization
    void begin();

    // Basic display operations
    void clearScreen();
    void show();
    void fillScreen(uint16_t color);
    void fillRect(int x, int y, int w, int h, uint16_t color);
    void drawPixel(int x, int y, uint16_t color);
    void drawCircle(int x, int y, int radius, uint16_t color);
    void fillCircle(int x, int y, int radius, uint16_t color);

    // Text operations
    void setTextSize(int size);
    void setTextColor(uint16_t color);
    void setCursor(int x, int y);
    void print(const char* text);
    void print(const String& text);
    void getTextBounds(const char* text, int x, int y, int16_t* x1, int16_t* y1, uint16_t* w,
                       uint16_t* h);

    // Color utilities
    uint16_t color565(uint8_t r, uint8_t g, uint8_t b);
    uint16_t applyBrightness(uint16_t color);
    uint16_t applyEffectBrightness(uint16_t color);
    uint16_t scaleBrightness(uint16_t color, float factor);
    uint16_t scaledColor565(uint8_t r, uint8_t g, uint8_t b);
    uint16_t scaledEffectColor565(uint8_t r, uint8_t g, uint8_t b);
    uint16_t randomVividColor();

    // Clock color utilities
    uint16_t getClockColor();

    // Text positioning utilities
    int getCenteredY(int textSize);
    int getCenteredX(const char* text, int textSize);
    int getTimeStringWidth(int textSize);  // For HH:MM:SS format

    // Text drawing utilities
    void drawCenteredText(const char* text, int textSize, uint16_t color, int y = -1);
    void drawCenteredTextWithBox(const char* text, int textSize, uint16_t color,
                                 uint16_t bgColor = 0x0000, int y = -1);
    void drawTightClock(const char* timeStr, int textSize, uint16_t color, int y = -1);

    // Text area management
    TextAreaInfo getTextAreaInfo(const char* text, int textSize);
    bool doesTextFit(const char* text, int textSize);
    void displayTextWithMarquee(const char* text, int textSize, uint16_t color, int& scrollX,
                                int& scrollDirection, unsigned long lastScrollTime,
                                int scrollSpeed);

    // Legacy compatibility functions for clock-specific naming
    void displayTimeWithMarquee(const char* timeStr, int textSize, uint16_t color, int& scrollX,
                                int& scrollDirection, unsigned long lastScrollTime,
                                int scrollSpeed) {
        displayTextWithMarquee(timeStr, textSize, color, scrollX, scrollDirection, lastScrollTime,
                               scrollSpeed);
    }

    // Text area utilities
    void getMainTextBounds(int& x1, int& y1, int& x2, int& y2);
    void getMainTextBounds(int& x1, int& y1, int& x2, int& y2, int textSize);
    void getAuxiliaryTextBounds(int& x1, int& y1, int& x2, int& y2);
    bool isInTextArea(int x, int y, bool hasText = true);
    bool isInTextArea(int x, int y, bool hasText, int textSize);
    bool isInTimeWithDateArea(int x, int y);
    void drawTextBackground();
    void drawTextBackground(int textSize);
    void drawTimeWithDateBackground();  // New method for time with date mode

    // Legacy clock-specific functions (for compatibility)
    void getTimeDisplayBounds(int& x1, int& y1, int& x2, int& y2) {
        getMainTextBounds(x1, y1, x2, y2);
    }
    void getTimeDisplayBounds(int& x1, int& y1, int& x2, int& y2, int textSize) {
        getMainTextBounds(x1, y1, x2, y2, textSize);
    }
    void getAMPMDisplayBounds(int& x1, int& y1, int& x2, int& y2) {
        getAuxiliaryTextBounds(x1, y1, x2, y2);
    }

    // Utility functions
    float generateVelocity(float minSpeed, float maxSpeed, bool allowNegative = true);

    // Message queue API (simple enqueue for scrolling messages)
    void enqueueMessage(const char* id, const char* text, const char* priority);
    void processMessageQueue();
    bool hasQueuedMessages() const;
    bool hasActiveHighPriorityMessage() const;
    void cancelActiveMessage();
    int getQueueCount() const;

    // Access to brightness arrays
    const uint16_t* getTextColors() const {
        return textColors;
    }
    const float* getBrightnessLevels() const {
        return brightnessLevels;
    }

   private:
    // Message queue internals
    struct MessageQueueItem {
        String id;
        String text;
        String priority;
    };

    static const int MESSAGE_QUEUE_SIZE = 8;
    MessageQueueItem messageQueue[MESSAGE_QUEUE_SIZE];
    int mqHead;
    int mqTail;
    int mqCount;

    // Active message state
    bool hasActiveMessage;
    String activeId;
    String activeText;
    String activePriority;
    int activeTextSize;
    int activeScrollX;
    int activeScrollDir;
    unsigned long activeStartTime;
    unsigned long activeLastScroll;
    unsigned long activeDuration;  // ms
    int activeScrollSpeed;         // ms per step
    uint16_t activeColor;

    Adafruit_Protomatter* matrix;
    SettingsManager* settings;

    // Brightness arrays
    uint16_t textColors[BRIGHTNESS_LEVELS] = {0x2104, 0x4208, 0x630C, 0x8410, 0xA514,
                                              0xC618, 0xE71C, 0xEF5D, 0xF79E, 0xFFFF};
    float brightnessLevels[BRIGHTNESS_LEVELS] = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0};
};

#endif  // MATRIX_DISPLAY_MANAGER_H
