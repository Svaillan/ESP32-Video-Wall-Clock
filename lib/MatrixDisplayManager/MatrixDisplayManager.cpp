#include "MatrixDisplayManager.h"

MatrixDisplayManager::MatrixDisplayManager(Adafruit_Protomatter* matrix, SettingsManager* settings)
    : matrix(matrix), settings(settings) {}

void MatrixDisplayManager::begin() {
    matrix->setTextWrap(false);
    matrix->setTextColor(textColors[settings->getBrightnessIndex()]);
    matrix->setTextSize(settings->getTextSize());
    Serial.println("Matrix Display Manager initialized");
}

// Basic display operations
void MatrixDisplayManager::clearScreen() {
    matrix->fillScreen(0);
}

void MatrixDisplayManager::show() {
    matrix->show();
}

void MatrixDisplayManager::fillScreen(uint16_t color) {
    matrix->fillScreen(color);
}

void MatrixDisplayManager::fillRect(int x, int y, int w, int h, uint16_t color) {
    matrix->fillRect(x, y, w, h, color);
}

void MatrixDisplayManager::drawPixel(int x, int y, uint16_t color) {
    matrix->drawPixel(x, y, color);
}

void MatrixDisplayManager::drawCircle(int x, int y, int radius, uint16_t color) {
    matrix->drawCircle(x, y, radius, color);
}

void MatrixDisplayManager::fillCircle(int x, int y, int radius, uint16_t color) {
    matrix->fillCircle(x, y, radius, color);
}

// Text operations
void MatrixDisplayManager::setTextSize(int size) {
    matrix->setTextSize(size);
}

void MatrixDisplayManager::setTextColor(uint16_t color) {
    matrix->setTextColor(color);
}

void MatrixDisplayManager::setCursor(int x, int y) {
    matrix->setCursor(x, y);
}

void MatrixDisplayManager::print(const char* text) {
    matrix->print(text);
}

void MatrixDisplayManager::print(const String& text) {
    matrix->print(text);
}

void MatrixDisplayManager::getTextBounds(const char* text, int x, int y, int16_t* x1, int16_t* y1,
                                         uint16_t* w, uint16_t* h) {
    matrix->getTextBounds(text, x, y, x1, y1, w, h);
}

// Color utilities
uint16_t MatrixDisplayManager::color565(uint8_t r, uint8_t g, uint8_t b) {
    return matrix->color565(r, g, b);
}

uint16_t MatrixDisplayManager::applyBrightness(uint16_t color) {
    float brightness = brightnessLevels[settings->getBrightnessIndex()];

    // Extract RGB components from RGB565
    uint8_t r = (color >> 11) & 0x1F;
    uint8_t g = (color >> 5) & 0x3F;
    uint8_t b = color & 0x1F;

    // Scale by brightness
    r = (uint8_t)(r * brightness);
    g = (uint8_t)(g * brightness);
    b = (uint8_t)(b * brightness);

    // Reassemble RGB565
    return (r << 11) | (g << 5) | b;
}

uint16_t MatrixDisplayManager::applyEffectBrightness(uint16_t color) {
    float brightness;

    // Use minimum brightness + 1 unless already at maximum
    if (settings->getBrightnessIndex() == BRIGHTNESS_LEVELS - 1) {
        brightness = brightnessLevels[settings->getBrightnessIndex()];  // Use max brightness
    } else {
        brightness = brightnessLevels[settings->getBrightnessIndex() + 1];  // Use brightness + 1
    }

    // Extract RGB components from RGB565
    uint8_t r = (color >> 11) & 0x1F;
    uint8_t g = (color >> 5) & 0x3F;
    uint8_t b = color & 0x1F;

    // Scale by brightness
    r = (uint8_t)(r * brightness);
    g = (uint8_t)(g * brightness);
    b = (uint8_t)(b * brightness);

    // Reassemble RGB565
    return (r << 11) | (g << 5) | b;
}

uint16_t MatrixDisplayManager::scaleBrightness(uint16_t color, float factor) {
    if (factor <= 0.0f)
        return 0;
    if (factor >= 1.0f)
        return color;

    // Extract RGB components from RGB565
    uint8_t r = (color >> 11) & 0x1F;  // 5 bits
    uint8_t g = (color >> 5) & 0x3F;   // 6 bits
    uint8_t b = color & 0x1F;          // 5 bits

    // Scale each component
    r = (uint8_t)(r * factor);
    g = (uint8_t)(g * factor);
    b = (uint8_t)(b * factor);

    // Reassemble RGB565
    return (r << 11) | (g << 5) | b;
}

uint16_t MatrixDisplayManager::scaledColor565(uint8_t r, uint8_t g, uint8_t b) {
    float brightness = brightnessLevels[settings->getBrightnessIndex()];

    // Scale by brightness
    r = (uint8_t)(r * brightness);
    g = (uint8_t)(g * brightness);
    b = (uint8_t)(b * brightness);

    return matrix->color565(r, g, b);
}

uint16_t MatrixDisplayManager::scaledEffectColor565(uint8_t r, uint8_t g, uint8_t b) {
    float brightness;

    // Use minimum brightness + 1 unless already at maximum
    if (settings->getBrightnessIndex() == BRIGHTNESS_LEVELS - 1) {
        brightness = brightnessLevels[settings->getBrightnessIndex()];  // Use max brightness
    } else {
        brightness = brightnessLevels[settings->getBrightnessIndex() + 1];  // Use brightness + 1
    }

    // Scale by brightness
    r = (uint8_t)(r * brightness);
    g = (uint8_t)(g * brightness);
    b = (uint8_t)(b * brightness);

    return matrix->color565(r, g, b);
}

uint16_t MatrixDisplayManager::getClockColor() {
    switch (settings->getClockColorMode()) {
        case CLOCK_WHITE:
            return applyBrightness(matrix->color565(255, 255, 255));
        case CLOCK_RED:
            return applyBrightness(matrix->color565(255, 0, 0));
        case CLOCK_GREEN:
            return applyBrightness(matrix->color565(0, 255, 0));
        case CLOCK_BLUE:
            return applyBrightness(matrix->color565(0, 0, 255));
        case CLOCK_YELLOW:
            return applyBrightness(matrix->color565(255, 255, 0));
        case CLOCK_CYAN:
            return applyBrightness(matrix->color565(0, 255, 255));
        case CLOCK_MAGENTA:
            return applyBrightness(matrix->color565(255, 0, 255));
        case CLOCK_ORANGE:
            return applyBrightness(matrix->color565(255, 69, 0));
        case CLOCK_PURPLE:
            return applyBrightness(matrix->color565(128, 0, 128));
        case CLOCK_PINK:
            return applyBrightness(matrix->color565(255, 20, 147));  // Deep pink - more vibrant
        case CLOCK_LIME:
            return applyBrightness(matrix->color565(50, 205, 50));
        case CLOCK_TEAL:
            return applyBrightness(matrix->color565(0, 128, 128));
        case CLOCK_INDIGO:
            return applyBrightness(matrix->color565(75, 0, 130));
        case CLOCK_GOLD:
            return applyBrightness(matrix->color565(255, 215, 0));
        case CLOCK_SILVER:
            return applyBrightness(matrix->color565(192, 192, 192));
        case CLOCK_RAINBOW: {
            // Rainbow effect based on time
            uint32_t time = millis();
            float hue = (time / 50.0f);  // Change color every 50ms
            hue = fmod(hue, 360.0f);

            // Convert HSV to RGB (simplified)
            float c = 1.0f;
            float x = c * (1.0f - fabs(fmod(hue / 60.0f, 2.0f) - 1.0f));
            float r = 0, g = 0, b = 0;

            if (hue < 60) {
                r = c;
                g = x;
                b = 0;
            } else if (hue < 120) {
                r = x;
                g = c;
                b = 0;
            } else if (hue < 180) {
                r = 0;
                g = c;
                b = x;
            } else if (hue < 240) {
                r = 0;
                g = x;
                b = c;
            } else if (hue < 300) {
                r = x;
                g = 0;
                b = c;
            } else {
                r = c;
                g = 0;
                b = x;
            }

            return applyBrightness(
                matrix->color565((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255)));
        }
        default:
            return applyBrightness(matrix->color565(255, 255, 255));
    }
}

// Text positioning and utility functions
int MatrixDisplayManager::getCenteredX(const char* text, int textSize) {
    int16_t x1, y1;
    uint16_t w, h;
    matrix->setTextSize(textSize);
    matrix->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    return (MATRIX_WIDTH - w) / 2 - x1;
}

int MatrixDisplayManager::getCenteredY(int textSize) {
    int textHeight = 8 * textSize;
    return (MATRIX_HEIGHT - textHeight) / 2;
}

void MatrixDisplayManager::drawCenteredText(const char* text, int textSize, uint16_t color, int y) {
    matrix->setTextSize(textSize);
    matrix->setTextColor(color);

    if (y == -1) {
        y = getCenteredY(textSize);
    }

    int x = getCenteredX(text, textSize);
    matrix->setCursor(x, y);
    matrix->print(text);
}

void MatrixDisplayManager::drawCenteredTextWithBox(const char* text, int textSize, uint16_t color,
                                                   uint16_t bgColor, int y) {
    matrix->setTextSize(textSize);

    if (y == -1) {
        y = getCenteredY(textSize);
    }

    int x = getCenteredX(text, textSize);

    // Calculate text dimensions for background box
    int textWidth = strlen(text) * 6 * textSize;  // Approximate width
    int textHeight = 8 * textSize;                // Standard character height

    // Add padding around text
    int padding = 2;
    int boxX = x - padding;
    int boxY = y - padding;
    int boxWidth = textWidth + (2 * padding);
    int boxHeight = textHeight + (2 * padding);

    // Ensure box stays within screen bounds
    if (boxX < 0)
        boxX = 0;
    if (boxY < 0)
        boxY = 0;
    if (boxX + boxWidth > MATRIX_WIDTH)
        boxWidth = MATRIX_WIDTH - boxX;
    if (boxY + boxHeight > MATRIX_HEIGHT)
        boxHeight = MATRIX_HEIGHT - boxY;

    // Draw background box
    matrix->fillRect(boxX, boxY, boxWidth, boxHeight, bgColor);

    // Draw text on top
    matrix->setTextColor(color);
    matrix->setCursor(x, y);
    matrix->print(text);
}

int MatrixDisplayManager::getTimeStringWidth(int textSize) {
    // 6 digits * 6 pixels + 2 colons with custom spacing (HH:MM:SS format)
    int digitWidth = 6 * textSize;
    int colonSpacing = 2 * textSize;  // Reduced spacing for colons
    return (6 * digitWidth) + (2 * colonSpacing);
}

// Text area management functions
TextAreaInfo MatrixDisplayManager::getTextAreaInfo(const char* text, int textSize) {
    TextAreaInfo info;

    int16_t x1, y1;
    uint16_t w, h;
    matrix->setTextSize(textSize);
    matrix->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

    info.width = w;
    info.height = h;
    info.boundingX = x1;
    info.boundingY = y1;
    info.centeredX = (MATRIX_WIDTH - w) / 2 - x1;
    info.centeredY = (MATRIX_HEIGHT - h) / 2 - y1;

    return info;
}

bool MatrixDisplayManager::doesTextFit(const char* text, int textSize) {
    TextAreaInfo info = getTextAreaInfo(text, textSize);
    return (info.width <= MATRIX_WIDTH && info.height <= MATRIX_HEIGHT);
}

void MatrixDisplayManager::displayTextWithMarquee(const char* text, int textSize, uint16_t color,
                                                  int& scrollX, int& scrollDirection,
                                                  unsigned long lastScrollTime, int scrollSpeed) {
    static unsigned long lastUpdate = 0;

    // Check if it's time to update scroll position
    if (millis() - lastUpdate >= scrollSpeed) {
        lastUpdate = millis();

        // Only scroll if text doesn't fit
        if (!doesTextFit(text, textSize)) {
            TextAreaInfo info = getTextAreaInfo(text, textSize);

            // Update scroll position
            if (scrollDirection == 1) {  // Moving right
                scrollX++;
                if (scrollX > MATRIX_WIDTH) {
                    scrollDirection = -1;  // Change direction
                }
            } else {  // Moving left
                scrollX--;
                if (scrollX < -(int)info.width) {
                    scrollDirection = 1;  // Change direction
                }
            }
        } else {
            // Text fits, center it
            scrollX = getCenteredX(text, textSize);
        }
    }

    // Draw the text at current scroll position
    matrix->setTextSize(textSize);
    matrix->setTextColor(color);
    matrix->setCursor(scrollX, getCenteredY(textSize));
    matrix->print(text);
}

// Display bounds and text area functions
void MatrixDisplayManager::getMainTextBounds(int& x1, int& y1, int& x2, int& y2) {
    getMainTextBounds(x1, y1, x2, y2, settings->getTextSize());
}

void MatrixDisplayManager::getMainTextBounds(int& x1, int& y1, int& x2, int& y2, int textSize) {
    int timeWidth = getTimeStringWidth(textSize);
    int timeHeight = 8 * textSize;

    x1 = (MATRIX_WIDTH - timeWidth) / 2 - 2;  // Add 2 pixel padding
    y1 = getCenteredY(textSize) - 2;
    x2 = x1 + timeWidth + 4;  // Add 4 pixels total padding
    y2 = y1 + timeHeight + 4;

    // Ensure bounds are within screen
    x1 = max(0, x1);
    y1 = max(0, y1);
    x2 = min(MATRIX_WIDTH - 1, x2);
    y2 = min(MATRIX_HEIGHT - 1, y2);
}

void MatrixDisplayManager::getAuxiliaryTextBounds(int& x1, int& y1, int& x2, int& y2) {
    if (settings->getUse24HourFormat()) {
        // No AM/PM in 24-hour format
        x1 = y1 = x2 = y2 = 0;
        return;
    }

    // AM/PM area in bottom right corner
    int ampmWidth = 2 * 6;  // 2 characters * 6 pixels each at size 1
    int ampmHeight = 8;     // 8 pixels height at size 1

    x1 = MATRIX_WIDTH - ampmWidth - 3;    // 3 pixel padding from right edge
    y1 = MATRIX_HEIGHT - ampmHeight - 1;  // 1 pixel padding from bottom
    x2 = MATRIX_WIDTH - 1;
    y2 = MATRIX_HEIGHT - 1;
}

bool MatrixDisplayManager::isInTextArea(int x, int y, bool hasText) {
    return isInTextArea(x, y, hasText, settings->getTextSize());
}

bool MatrixDisplayManager::isInTextArea(int x, int y, bool hasText, int textSize) {
    if (!hasText)
        return false;

    // Check main text area
    int x1, y1, x2, y2;
    getMainTextBounds(x1, y1, x2, y2, textSize);

    if (x >= x1 && x <= x2 && y >= y1 && y <= y2) {
        return true;
    }

    // Check auxiliary text area (AM/PM for clock) if in 12-hour format
    if (!settings->getUse24HourFormat()) {
        getAuxiliaryTextBounds(x1, y1, x2, y2);
        if (x >= x1 && x <= x2 && y >= y1 && y <= y2) {
            return true;
        }
    }

    return false;
}

// Color utility functions
uint16_t MatrixDisplayManager::randomVividColor() {
    uint8_t r = 0, g = 0, b = 0;
    uint8_t colorType = random(7);

    switch (colorType) {
        case 0:
            r = 255;
            break;
        case 1:
            g = 255;
            break;
        case 2:
            b = 255;
            break;
        case 3:
            r = 255;
            g = 255;
            break;
        case 4:
            r = 255;
            b = 255;
            break;
        case 5:
            g = 255;
            b = 255;
            break;
        case 6:
            r = g = b = 255;
            break;
    }

    // Add some randomization for non-white colors
    if (colorType != 6 && random(6) == 0) {
        uint8_t shade = random(64, 192);
        if (!r)
            r = shade;
        if (!g)
            g = shade;
        if (!b)
            b = shade;
    }

    return scaledColor565(r, g, b);
}

float MatrixDisplayManager::generateVelocity(float minSpeed, float maxSpeed, bool allowNegative) {
    float velocity;
    do {
        velocity = random(-maxSpeed * 100, maxSpeed * 100 + 1) / 100.0;
    } while (abs(velocity) < minSpeed);  // Ensure minimum movement

    if (!allowNegative && velocity < 0) {
        velocity = -velocity;
    }

    return velocity;
}

/**
 * Draw black background rectangles around the text areas
 */
void MatrixDisplayManager::drawTextBackground() {
    drawTextBackground(settings->getTextSize());
}

void MatrixDisplayManager::drawTextBackground(int textSize) {
    // Draw background for main text
    int x1, y1, x2, y2;
    getMainTextBounds(x1, y1, x2, y2, textSize);
    fillRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1, 0x0000);

    // Draw background for auxiliary text (AM/PM for clock) if in 12-hour format
    if (!settings->getUse24HourFormat()) {
        getAuxiliaryTextBounds(x1, y1, x2, y2);
        fillRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1, 0x0000);
    }
}

/**
 * Draw tight clock display that removes extra spacing for HH:MM:SS format
 */
void MatrixDisplayManager::drawTightClock(const char* timeStr, int textSize, uint16_t color,
                                          int y) {
    if (y == -1) {
        y = getCenteredY(textSize);
    }

    setTextSize(textSize);
    setTextColor(color);

    // Calculate spacing like the original
    int digitWidth = 6 * textSize;
    int colonWidth = 3 * textSize;
    int beforeColon = -2 * textSize;
    int afterColon = 1 * textSize;

    // Calculate total width and starting X
    int totalWidth = (6 * digitWidth) + (2 * (beforeColon + colonWidth + afterColon));
    int x = (MATRIX_WIDTH - totalWidth) / 2;

    // Draw each character with proper spacing
    for (int i = 0; i < 8 && timeStr[i] != '\0'; i++) {
        char c = timeStr[i];
        if (c == ':') {
            x += beforeColon;
            setCursor(x, y);
            print(":");
            x += colonWidth + afterColon;
        } else {
            setCursor(x, y);
            char charStr[2] = {c, '\0'};
            print(charStr);
            x += digitWidth;
        }
    }
}
