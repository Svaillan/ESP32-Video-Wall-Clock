#include <Arduino.h>
#include <Adafruit_Protomatter.h>
#include <Wire.h>
#include <RTClib.h>
#include <EEPROM.h>
#include "SettingsManager.h"
#include "ButtonManager.h"
#include "MatrixDisplayManager.h"
#include "EffectsEngine.h"

// ===============================================
// CONFIGURATION & CONSTANTS
// ===============================================

// Matrix Pin Configuration
uint8_t rgbPins[]  = {25, 27, 26, 14, 13, 12}; // {R1, B1, G1, R2, B2, G2}
uint8_t addrPins[] = {23, 19, 5, 17};
uint8_t clockPin   = 16;
uint8_t latchPin   = 4;
uint8_t oePin      = 15;

// Matrix Display Settings
#define MATRIX_WIDTH  128
#define MATRIX_HEIGHT 32
#define BIT_DEPTH     5

// Text Settings
#define TEXT_SIZE_MIN 1
#define TEXT_SIZE_MAX 3
#define BRIGHTNESS_LEVELS 10

// Timing Constants
#define MENU_DELAY 30               // Reduced from 50ms 
#define CLOCK_UPDATE_DELAY 10

// ===============================================
// DATA STRUCTURES & ENUMS
// ===============================================

enum AppState { 
  SHOW_TIME, 
  MENU, 
  EDIT_TEXT_SIZE, 
  EDIT_BRIGHTNESS, 
  EDIT_TIME_FORMAT,
  EDIT_CLOCK_COLOR,
  EDIT_EFFECTS, 
  TIME_SET 
};

enum SetClockStep { 
  NONE, 
  SET_HOUR, 
  SET_MINUTE, 
  SET_SECOND, 
  CONFIRM 
};

// ===============================================
// GLOBAL VARIABLES
// ===============================================

// Hardware Objects
Adafruit_Protomatter matrix(
  MATRIX_WIDTH, BIT_DEPTH, 1, rgbPins,
  4, addrPins, clockPin, latchPin, oePin, true);
RTC_DS3231 rtc;
SettingsManager settings;
ButtonManager buttons;
MatrixDisplayManager display(&matrix, &settings);
EffectsEngine effects(&display, &settings);

// Display Settings
uint16_t textColors[BRIGHTNESS_LEVELS] = { 0x2104, 0x4208, 0x630C, 0x8410, 0xA514, 0xC618, 0xE71C, 0xEF5D, 0xF79E, 0xFFFF };
float brightnessLevels[BRIGHTNESS_LEVELS] = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0 };

// State Variables
AppState appState = SHOW_TIME;
SetClockStep setStep = NONE;
// Button configuration
const int buttonPins[] = {19, 18, 5};  // UP, DOWN, ENTER
const int numButtons = 3;

// Menu Configuration
const char* menuItems[] = { "Text Size", "Brightness", "Time Format", "Clock Color", "Effects", "Set Clock", "Exit" };
const int MENU_ITEMS = sizeof(menuItems) / sizeof(menuItems[0]);
const char* effectNames[] = {"Confetti", "Acid", "Rain", "Torrent", "Stars", "Sparkles", "Fireworks", "Tron", "Off"};
const int EFFECT_OPTIONS = sizeof(effectNames) / sizeof(effectNames[0]);
const char* clockColorNames[] = {"White", "Red", "Green", "Blue", "Yellow", "Cyan", "Magenta", "Orange", "Purple", "Pink", "Lime", "Teal", "Indigo", "Gold", "Silver", "Rainbow"};
const int CLOCK_COLOR_OPTIONS = sizeof(clockColorNames) / sizeof(clockColorNames[0]);

// Menu State
int menuIndex = 0;
int effectMenuIndex = 0;
int clockColorMenuIndex = 0;

// Time Setting State
int setHour = 0, setMin = 0, setSec = 0;
bool inSetMode = false;
uint32_t timeSetEntryTime = 0;  // Time when we entered time setting mode
const uint32_t BUTTON_LOCK_DURATION = 1000;  // 1 second lock after entering
uint32_t lastEnterPress = 0;  // Track last ENTER button press
const uint32_t ENTER_COOLDOWN = 300;  // 300ms cooldown between ENTER presses
bool entryLockProcessed = false;  // Track if entry lock has been processed
char timeStr[16];

// ===============================================
// UTILITY FUNCTIONS
// ===============================================

/**
 * Calculate the Y position to center text vertically on the display
 */
int getCenteredY(int textSize) {
  int textHeight = 8 * textSize;  // Character height in pixels
  return (MATRIX_HEIGHT - textHeight) / 2;
}

/**
 * Calculate the X position to center text horizontally
 */
int getCenteredX(const char* text, int textSize) {
  int16_t x1, y1;
  uint16_t w, h;
  matrix.setTextSize(textSize);
  matrix.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  return (MATRIX_WIDTH - w) / 2 - x1;
}

/**
 * Display centered text at specified position
 */
void drawCenteredText(const char* text, int textSize, uint16_t color, int y = -1) {
  matrix.setTextSize(textSize);
  matrix.setTextColor(color);
  
  if (y == -1) {
    y = getCenteredY(textSize);
  }
  
  int x = getCenteredX(text, textSize);
  matrix.setCursor(x, y);
  matrix.print(text);
}

/**
 * Display centered text with a background box
 */
void drawCenteredTextWithBox(const char* text, int textSize, uint16_t color, uint16_t bgColor = 0x0000, int y = -1) {
  matrix.setTextSize(textSize);
  
  if (y == -1) {
    y = getCenteredY(textSize);
  }
  
  int x = getCenteredX(text, textSize);
  
  // Calculate text dimensions for background box
  int textWidth = strlen(text) * 6 * textSize;  // Approximate width
  int textHeight = 8 * textSize;  // Standard character height
  
  // Add padding around text
  int padding = 2;
  int boxX = x - padding;
  int boxY = y - padding;
  int boxWidth = textWidth + (2 * padding);
  int boxHeight = textHeight + (2 * padding);
  
  // Ensure box stays within screen bounds
  if (boxX < 0) boxX = 0;
  if (boxY < 0) boxY = 0;
  if (boxX + boxWidth > MATRIX_WIDTH) boxWidth = MATRIX_WIDTH - boxX;
  if (boxY + boxHeight > MATRIX_HEIGHT) boxHeight = MATRIX_HEIGHT - boxY;
  
  // Draw background box
  matrix.fillRect(boxX, boxY, boxWidth, boxHeight, bgColor);
  
  // Draw text on top
  matrix.setTextColor(color);
  matrix.setCursor(x, y);
  matrix.print(text);
}

/**
 * Calculate width of time string with tight spacing
 */
int getTimeStringWidth(int textSize) {
  // 6 digits * 6 pixels + 2 colons with custom spacing
  int digitWidth = 6 * textSize;
  int colonSpacing = 2 * textSize;  // Reduced spacing for colons
  return (6 * digitWidth) + (2 * colonSpacing);
}

/**
 * Apply global brightness scaling to any color
 */
uint16_t applyBrightness(uint16_t color) {
  float brightness = brightnessLevels[settings.getBrightnessIndex()];
  
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

/**
 * Apply effect brightness scaling to any color (minimum brightness + 1)
 */
uint16_t applyEffectBrightness(uint16_t color) {
  float brightness;
  
  // Use minimum brightness + 1 unless already at maximum
  if (settings.getBrightnessIndex() == BRIGHTNESS_LEVELS - 1) {
    brightness = brightnessLevels[settings.getBrightnessIndex()];  // Use max brightness
  } else {
    brightness = brightnessLevels[settings.getBrightnessIndex() + 1];  // Use brightness + 1
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

/**
 * Scale color brightness by a factor (0.0 to 1.0)
 */
uint16_t scaleBrightness(uint16_t color, float factor) {
  if (factor <= 0.0f) return 0;
  if (factor >= 1.0f) return color;
  
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

/**
 * Apply brightness to RGB888 values and convert to RGB565
 */
uint16_t scaledColor565(uint8_t r, uint8_t g, uint8_t b) {
  float brightness = brightnessLevels[settings.getBrightnessIndex()];
  
  // Scale by brightness
  r = (uint8_t)(r * brightness);
  g = (uint8_t)(g * brightness);
  b = (uint8_t)(b * brightness);
  
  return matrix.color565(r, g, b);
}

/**
 * Apply brightness to RGB888 values for effects with minimum brightness
 */
uint16_t scaledEffectColor565(uint8_t r, uint8_t g, uint8_t b) {
  float brightness;
  
  // Use minimum brightness + 1 unless already at maximum
  if (settings.getBrightnessIndex() == BRIGHTNESS_LEVELS - 1) {
    brightness = brightnessLevels[settings.getBrightnessIndex()];  // Use max brightness
  } else {
    brightness = brightnessLevels[settings.getBrightnessIndex() + 1];  // Use brightness + 1
  }
  
  // Scale by brightness
  r = (uint8_t)(r * brightness);
  g = (uint8_t)(g * brightness);
  b = (uint8_t)(b * brightness);
  
  return matrix.color565(r, g, b);
}

/**
 * Get the current clock color based on clock color mode
 */
uint16_t getClockColor() {
  switch (settings.getClockColorMode()) {
    case CLOCK_WHITE:
      return applyBrightness(matrix.color565(255, 255, 255));
    case CLOCK_RED:
      return applyBrightness(matrix.color565(255, 0, 0));
    case CLOCK_GREEN:
      return applyBrightness(matrix.color565(0, 255, 0));
    case CLOCK_BLUE:
      return applyBrightness(matrix.color565(0, 0, 255));
    case CLOCK_YELLOW:
      return applyBrightness(matrix.color565(255, 255, 0));
    case CLOCK_CYAN:
      return applyBrightness(matrix.color565(0, 255, 255));
    case CLOCK_MAGENTA:
      return applyBrightness(matrix.color565(255, 0, 255));
    case CLOCK_ORANGE:
      return applyBrightness(matrix.color565(255, 69, 0));
    case CLOCK_PURPLE:
      return applyBrightness(matrix.color565(128, 0, 128));
    case CLOCK_PINK:
      return applyBrightness(matrix.color565(255, 20, 147));  // Deep pink - more vibrant
    case CLOCK_LIME:
      return applyBrightness(matrix.color565(50, 205, 50));
    case CLOCK_TEAL:
      return applyBrightness(matrix.color565(0, 128, 128));
    case CLOCK_INDIGO:
      return applyBrightness(matrix.color565(75, 0, 130));
    case CLOCK_GOLD:
      return applyBrightness(matrix.color565(255, 215, 0));
    case CLOCK_SILVER:
      return applyBrightness(matrix.color565(192, 192, 192));
    case CLOCK_RAINBOW:
      {
        // Rainbow effect based on time
        uint32_t time = millis();
        float hue = (time / 50.0f); // Change color every 50ms
        hue = fmod(hue, 360.0f);
        
        // Convert HSV to RGB (simplified)
        float c = 1.0f;
        float x = c * (1.0f - fabs(fmod(hue / 60.0f, 2.0f) - 1.0f));
        float r = 0, g = 0, b = 0;
        
        if (hue < 60) { r = c; g = x; b = 0; }
        else if (hue < 120) { r = x; g = c; b = 0; }
        else if (hue < 180) { r = 0; g = c; b = x; }
        else if (hue < 240) { r = 0; g = x; b = c; }
        else if (hue < 300) { r = x; g = 0; b = c; }
        else { r = c; g = 0; b = x; }
        
        return applyBrightness(matrix.color565((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255)));
      }
    default:
      return applyBrightness(matrix.color565(255, 255, 255));
  }
}

// ===============================================
// EFFECTS SYSTEM
// ===============================================

/**
 * Generate a random vivid color for confetti
 */
uint16_t randomVividColor() {
  uint8_t r = 0, g = 0, b = 0;
  uint8_t colorType = random(7);
  
  switch(colorType) {
    case 0: r = 255; break;
    case 1: g = 255; break;
    case 2: b = 255; break;
    case 3: r = 255; g = 255; break;
    case 4: r = 255; b = 255; break;
    case 5: g = 255; b = 255; break;
    case 6: r = g = b = 255; break;
  }
  
  // Add some randomization for non-white colors
  if (colorType != 6 && random(6) == 0) {
    uint8_t shade = random(64, 192);
    if (!r) r = shade;
    if (!g) g = shade;
    if (!b) b = shade;
  }
  
  return scaledColor565(r, g, b);
}

/**
 * Generate guaranteed non-zero velocity for confetti
 */
float generateVelocity(float minSpeed, float maxSpeed, bool allowNegative = true) {
  float velocity;
  do {
    velocity = random(-maxSpeed * 100, maxSpeed * 100 + 1) / 100.0;
  } while (abs(velocity) < minSpeed); // Ensure minimum movement
  
  if (!allowNegative && velocity < 0) {
    velocity = -velocity;
  }
  
  return velocity;
}

/**
 * Initialize confetti particles
 */



/**
 * Get the bounding box of the current time display (main clock only)
 */
void getTimeDisplayBounds(int &x1, int &y1, int &x2, int &y2) {
  int timeWidth = getTimeStringWidth(settings.getTextSize());
  int timeHeight = 8 * settings.getTextSize();
  
  x1 = (MATRIX_WIDTH - timeWidth) / 2 - 2;  // Add 2 pixel padding
  y1 = getCenteredY(settings.getTextSize()) - 2;
  x2 = x1 + timeWidth + 4;  // Add 4 pixels total padding
  y2 = y1 + timeHeight + 4;
  
  // Ensure bounds are within screen
  x1 = max(0, x1);
  y1 = max(0, y1);
  x2 = min(MATRIX_WIDTH - 1, x2);
  y2 = min(MATRIX_HEIGHT - 1, y2);
}

/**
 * Get the bounding box of the AM/PM indicator
 */
void getAMPMDisplayBounds(int &x1, int &y1, int &x2, int &y2) {
  if (settings.getUse24HourFormat()) {
    // No AM/PM in 24-hour format
    x1 = y1 = x2 = y2 = 0;
    return;
  }
  
  // AM/PM area in bottom right corner
  int ampmWidth = 2 * 6;  // 2 characters * 6 pixels each at size 1
  int ampmHeight = 8;     // 8 pixels height at size 1
  
  x1 = MATRIX_WIDTH - ampmWidth - 3;  // 3 pixel padding from right edge
  y1 = MATRIX_HEIGHT - ampmHeight - 1; // 1 pixel padding from bottom
  x2 = MATRIX_WIDTH - 1;
  y2 = MATRIX_HEIGHT - 1;
}

/**
 * Check if a point is within the text area (either main clock or AM/PM)
 */
bool isInTextArea(int x, int y) {
  if (appState != SHOW_TIME) return false;
  
  // Check main clock area
  int x1, y1, x2, y2;
  getTimeDisplayBounds(x1, y1, x2, y2);
  
  if (x >= x1 && x <= x2 && y >= y1 && y <= y2) {
    return true;
  }
  
  // Check AM/PM area if in 12-hour format
  if (!settings.getUse24HourFormat()) {
    getAMPMDisplayBounds(x1, y1, x2, y2);
    if (x >= x1 && x <= x2 && y >= y1 && y <= y2) {
      return true;
    }
  }
  
  return false;
}



/**
 * Draw black background rectangles around the text areas
 */
void drawTextBackground() {
  if (appState != SHOW_TIME) return;
  
  // Draw background for main clock
  int x1, y1, x2, y2;
  display.getTimeDisplayBounds(x1, y1, x2, y2);
  display.fillRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1, 0x0000);
  
  // Draw background for AM/PM if in 12-hour format
  if (!settings.getUse24HourFormat()) {
    display.getAMPMDisplayBounds(x1, y1, x2, y2);
    display.fillRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1, 0x0000);
  }
}

/**
 * Render background effects
 */
void renderEffects() {
  effects.updateEffects();
  drawTextBackground();  // Draw black background behind text for all effects except OFF
}

// ===============================================
// CLOCK DISPLAY
// ===============================================

/**
 * Draw time string with tight, centered spacing
 */
void drawTightClock(const char* timeStr, int textSize, uint16_t color, int y = -1) {
  if (y == -1) {
    y = getCenteredY(textSize);
  }
  
  matrix.setTextSize(textSize);
  matrix.setTextColor(color);
  
  // Calculate spacing
  int digitWidth = 6 * textSize;
  int colonWidth = 3 * textSize;
  int beforeColon = -2 * textSize;
  int afterColon = 1 * textSize;
  
  // Calculate total width and starting X
  int totalWidth = (6 * digitWidth) + (2 * (beforeColon + colonWidth + afterColon));
  int x = (MATRIX_WIDTH - totalWidth) / 2;
  
  // Draw each character
  for (int i = 0; timeStr[i] != '\0' && i < 8; i++) {
    char c = timeStr[i];
    if (c == ':') {
      x += beforeColon;
      matrix.setCursor(x, y);
      matrix.print(':');
      x += colonWidth + afterColon;
    } else {
      matrix.setCursor(x, y);
      matrix.print(c);
      x += digitWidth;
    }
  }
}

// ===============================================
// MENU SYSTEM
// ===============================================

/**
 * Handle menu entry logic
 */
void handleMenuEntry() {
  static bool blockMenuReentry = false;
  static uint32_t enterPressTime = 0;
  static bool wasPressed = false;
  
  if (appState == SHOW_TIME) {
    if (blockMenuReentry) {
      if (!buttons.isEnterPressed()) blockMenuReentry = false;
    } else {
      if (buttons.isEnterPressed() && !wasPressed) {
        enterPressTime = millis();
        wasPressed = true;
      }
      if (!buttons.isEnterPressed() && wasPressed) {
        if (millis() - enterPressTime < 1000) {
          appState = MENU;
          menuIndex = 0;
        }
        wasPressed = false;
        blockMenuReentry = true;
      }
    }
  }
}

/**
 * Display main menu
 */
void displayMainMenu() {
  char menuLine[32];
  strcpy(menuLine, menuItems[menuIndex]);
  
  // Add current values to menu items
  switch (menuIndex) {
    case 0: // Text Size
      sprintf(menuLine + strlen(menuLine), " (%d)", settings.getTextSize());
      break;
    case 1: // Brightness
      sprintf(menuLine + strlen(menuLine), " (%d)", settings.getBrightnessIndex() + 1);
      break;
    case 2: // Time Format
      sprintf(menuLine + strlen(menuLine), " (%s)", settings.getUse24HourFormat() ? "24H" : "12H");
      break;
    case 3: // Clock Color
      sprintf(menuLine + strlen(menuLine), " (%s)", clockColorNames[settings.getClockColorMode()]);
      break;
    case 4: // Effects
      sprintf(menuLine + strlen(menuLine), " (%s)", effectNames[settings.getEffectMode()]);
      break;
  }
  
  drawCenteredTextWithBox(menuLine, 1, applyBrightness(0xF81F));  // Purple menus with brightness scaling
}

/**
 * Handle main menu navigation
 */
void handleMainMenuInput() {
  if (buttons.isDownJustPressed()) {
    menuIndex = (menuIndex + 1) % MENU_ITEMS;
  }
  if (buttons.isUpJustPressed()) {
    menuIndex = (menuIndex - 1 + MENU_ITEMS) % MENU_ITEMS;
  }
  if (buttons.isEnterJustPressed() && !buttons.isEnterRepeating()) {
    switch (menuIndex) {
      case 0: 
        appState = EDIT_TEXT_SIZE; 
        break;
      case 1: 
        appState = EDIT_BRIGHTNESS; 
        break;
      case 2: 
        appState = EDIT_TIME_FORMAT; 
        break;
      case 3: 
        clockColorMenuIndex = settings.getClockColorMode();
        appState = EDIT_CLOCK_COLOR;
        break;
      case 4: 
        effectMenuIndex = settings.getEffectMode();
        appState = EDIT_EFFECTS;
        break;
      case 5: // Set Clock
        {
          DateTime now = rtc.now();
          setHour = now.hour();
          setMin = now.minute();
          setSec = now.second();
          setStep = SET_HOUR;  // Explicitly start at hour
          inSetMode = true;
          timeSetEntryTime = millis();  // Record entry time for button lock
          lastEnterPress = 0;  // Reset ENTER cooldown tracking
          entryLockProcessed = false;  // Reset entry lock flag
          appState = TIME_SET;
          
          // Debug output
          Serial.println("Entering time set mode");
          Serial.print("Starting at setStep: ");
          Serial.println((int)setStep);  // Cast to int for debugging
          Serial.print("SET_HOUR enum value: ");
          Serial.println((int)SET_HOUR);
          Serial.print("Current time: ");
          Serial.print(setHour); Serial.print(":");
          Serial.print(setMin); Serial.print(":");
          Serial.println(setSec);
          Serial.print("Entry time: ");
          Serial.println(timeSetEntryTime);
        }
        break;
      case 6: // Exit
        appState = SHOW_TIME;
        break;
    }
  }
}

/**
 * Display effects menu
 */
void displayEffectsMenu() {
  // Store current effect mode
  EffectMode originalMode = settings.getEffectMode();
  
  // Temporarily set effect mode to preview the selected effect
  settings.setEffectMode((EffectMode)effectMenuIndex);
  
  // Render the preview effect
  if (settings.getEffectMode() != EFFECT_OFF) {
    renderEffects();
  } else {
    // Clear screen for "off" effect
    matrix.fillScreen(0);
  }
  
  // Restore original effect mode
  settings.setEffectMode(originalMode);
  
  // Draw menu text on top
  char menuLine[32];
  strcpy(menuLine, effectNames[effectMenuIndex]);
  
  // Add indicator if this is the currently active effect
  if (effectMenuIndex == settings.getEffectMode()) {
    sprintf(menuLine + strlen(menuLine), " *");
  }
  
  drawCenteredTextWithBox(menuLine, 1, applyBrightness(0xF81F));  // Purple with brightness scaling
}

/**
 * Handle effects menu input
 */
void handleEffectsMenuInput() {
  if (buttons.isDownJustPressed()) {
    effectMenuIndex = (effectMenuIndex + 1) % EFFECT_OPTIONS;
  }
  if (buttons.isUpJustPressed()) {
    effectMenuIndex = (effectMenuIndex - 1 + EFFECT_OPTIONS) % EFFECT_OPTIONS;
  }
  if (buttons.isEnterJustPressed() && !buttons.isEnterRepeating()) {
    settings.setEffectMode((EffectMode)effectMenuIndex);
    settings.saveSettings();  // Save the new effect mode
    appState = MENU;
  }
}

/**
 * Display text size adjustment screen
 */
void displayTextSizeMenu() {
  char settingStr[24];
  sprintf(settingStr, "Text Size: %d", settings.getTextSize());
  drawCenteredTextWithBox(settingStr, 1, applyBrightness(0xF81F));  // Purple with brightness scaling
}

/**
 * Handle text size adjustment input
 */
void handleTextSizeInput() {
  bool settingsChanged = false;
  
  if (buttons.isUpJustPressed() && settings.getTextSize() < TEXT_SIZE_MAX) {
    settings.setTextSize(settings.getTextSize() + 1);
    settingsChanged = true;
  }
  if (buttons.isDownJustPressed() && settings.getTextSize() > TEXT_SIZE_MIN) {
    settings.setTextSize(settings.getTextSize() - 1);
    settingsChanged = true;
  }
  if (buttons.isEnterJustPressed() && !buttons.isEnterRepeating()) {
    appState = MENU;
  }
  
  // Save settings if they changed
  if (settingsChanged) {
    settings.saveSettings();
  }
}

/**
 * Display brightness adjustment screen
 */
void displayBrightnessMenu() {
  char settingStr[24];
  sprintf(settingStr, "Brightness: %d", settings.getBrightnessIndex() + 1);
  drawCenteredTextWithBox(settingStr, 1, applyBrightness(0xF81F));  // Purple with brightness scaling
}

/**
 * Display the time format selection menu
 */
void displayTimeFormatMenu() {
  const char* formatStr = settings.getUse24HourFormat() ? "24 Hour" : "12 Hour";
  char settingStr[24];
  sprintf(settingStr, "Format: %s", formatStr);
  drawCenteredTextWithBox(settingStr, 1, applyBrightness(0xF81F));  // Purple with brightness scaling
}

/**
 * Display the clock color selection menu with preview
 */
void displayClockColorMenu() {
  // Save current mode and temporarily set to preview mode
  ClockColorMode tempMode = settings.getClockColorMode();
  settings.setClockColorMode((ClockColorMode)clockColorMenuIndex);
  
  // Draw preview time with fixed text size 2
  uint16_t color = getClockColor();
  drawTightClock("12:34:56", 2, color);
  
  // Show the color name at the bottom
  int nameY = MATRIX_HEIGHT - 9;
  drawCenteredTextWithBox(clockColorNames[clockColorMenuIndex], 1, applyBrightness(0xF81F), 0x0000, nameY);  // Purple with black box
  
  // Restore original mode
  settings.setClockColorMode(tempMode);
}

/**
 * Handle brightness adjustment input
 */
void handleBrightnessInput() {
  bool settingsChanged = false;
  
  if (buttons.isUpJustPressed() && settings.getBrightnessIndex() < BRIGHTNESS_LEVELS - 1) {
    settings.setBrightnessIndex(settings.getBrightnessIndex() + 1);
    settingsChanged = true;
  }
  if (buttons.isDownJustPressed() && settings.getBrightnessIndex() > 0) {
    settings.setBrightnessIndex(settings.getBrightnessIndex() - 1);
    settingsChanged = true;
  }
  if (buttons.isEnterJustPressed() && !buttons.isEnterRepeating()) {
    appState = MENU;
  }
  
  // Save settings if they changed
  if (settingsChanged) {
    settings.saveSettings();
  }
}

/**
 * Handle time format adjustment input
 */
void handleTimeFormatInput() {
  bool settingsChanged = false;
  
  if ((buttons.isUpJustPressed() || buttons.isDownJustPressed()) && !(buttons.isUpRepeating() || buttons.isDownRepeating())) {
    settings.setUse24HourFormat(!settings.getUse24HourFormat());
    settingsChanged = true;
  }
  if (buttons.isEnterJustPressed() && !buttons.isEnterRepeating()) {
    appState = MENU;
  }
  
  // Save settings if they changed
  if (settingsChanged) {
    settings.saveSettings();
  }
}

/**
 * Handle clock color adjustment input
 */
void handleClockColorInput() {
  bool settingsChanged = false;
  
  if (buttons.isDownJustPressed()) {
    clockColorMenuIndex = (clockColorMenuIndex + 1) % CLOCK_COLOR_OPTIONS;
    settingsChanged = true;
  }
  if (buttons.isUpJustPressed()) {
    clockColorMenuIndex = (clockColorMenuIndex - 1 + CLOCK_COLOR_OPTIONS) % CLOCK_COLOR_OPTIONS;
    settingsChanged = true;
  }
  if (buttons.isEnterJustPressed() && !buttons.isEnterRepeating()) {
    settings.setClockColorMode((ClockColorMode)clockColorMenuIndex);
    settings.saveSettings();
    appState = MENU;
  }
  
  // Update the preview immediately
  if (settingsChanged) {
    // No need to save here since we only save on ENTER
  }
}

// ===============================================
// TIME SETTING SYSTEM
// ===============================================

/**
 * Handle time setting mode
 */
void handleTimeSettingMode() {
  static bool blinkState = true;
  static uint32_t lastBlink = 0;
  static bool debugPrinted = false;
  static SetClockStep lastSetStep = NONE;
  
  // Reset debug flag if step changed
  if (setStep != lastSetStep) {
    debugPrinted = false;
    lastSetStep = setStep;
  }
  
  // Debug output once when entering each step
  if (!debugPrinted) {
    Serial.println("=== TIME SETTING DEBUG ===");
    Serial.print("setStep = ");
    Serial.print((int)setStep);
    Serial.print(" (");
    switch(setStep) {
      case NONE: Serial.print("NONE"); break;
      case SET_HOUR: Serial.print("SET_HOUR"); break;
      case SET_MINUTE: Serial.print("SET_MINUTE"); break;
      case SET_SECOND: Serial.print("SET_SECOND"); break;
      case CONFIRM: Serial.print("CONFIRM"); break;
      default: Serial.print("UNKNOWN"); break;
    }
    Serial.println(")");
    debugPrinted = true;
  }
  
  // Handle blinking at 500ms intervals
  if (millis() - lastBlink > 500) {
    blinkState = !blinkState;
    lastBlink = millis();
  }
  
  // Simple button lock ONLY on initial entry to prevent menu bleed-through
  if (setStep == SET_HOUR && !entryLockProcessed && millis() - timeSetEntryTime < BUTTON_LOCK_DURATION) {
    // Just display during lock period, don't process buttons
    char displayStr[12];
    if (blinkState) {
      snprintf(displayStr, sizeof(displayStr), "%02d:%02d:%02d", setHour, setMin, setSec);
    } else {
      snprintf(displayStr, sizeof(displayStr), "  :%02d:%02d", setMin, setSec);
    }
    drawTightClock(displayStr, settings.getTextSize(), textColors[settings.getBrightnessIndex()]);
    return;
  }
  
  // Mark entry lock as processed after first check
  if (setStep == SET_HOUR && !entryLockProcessed) {
    entryLockProcessed = true;
  }
  
  // Handle button inputs - allow repeating for scrolling, but consume each press
  if (buttons.isUpJustPressed()) {
    buttons.clearUpJustPressed();  // Immediately consume the button press
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
  
  if (buttons.isDownJustPressed()) {
    buttons.clearDownJustPressed();  // Immediately consume the button press
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
  
  // Handle field progression - ENTER only on single press with cooldown
  if (buttons.isEnterJustPressed()) {
    Serial.print("ENTER justPressed=true, isRepeating=");
    Serial.print(buttons.isEnterRepeating());
    Serial.print(", Current setStep: ");
    Serial.print((int)setStep);
    Serial.print(", Time since last ENTER: ");
    Serial.print(millis() - lastEnterPress);
    Serial.print("ms");
    
    if (!buttons.isEnterRepeating() && (millis() - lastEnterPress) > ENTER_COOLDOWN) {
      buttons.clearEnterJustPressed();  // Immediately consume the button press
      Serial.print(" -> PROCESSING (consumed) -> ");
      lastEnterPress = millis();  // Update last press time
      
      switch (setStep) {
        case SET_HOUR:   
          setStep = SET_MINUTE;
          Serial.println("SET_MINUTE");
          break;
        case SET_MINUTE: 
          setStep = SET_SECOND;
          Serial.println("SET_SECOND");
          break;
        case SET_SECOND: 
          Serial.println("EXITING");
          // Set the RTC time and exit
          DateTime rtcNow = rtc.now();
          DateTime newTime = DateTime(rtcNow.year(), rtcNow.month(), rtcNow.day(), setHour, setMin, setSec);
          rtc.adjust(newTime);
          
          // Exit time setting mode
          inSetMode = false;
          appState = SHOW_TIME;
          setStep = NONE;
          return;
      }
      // Reset blink state when changing fields
      blinkState = true;
      lastBlink = millis();
    } else {
      if (buttons.isEnterRepeating()) {
        Serial.println(" -> IGNORING (isRepeating=true)");
      } else {
        Serial.println(" -> IGNORING (cooldown active)");
      }
    }
  }
  
  // Display time with appropriate blinking field
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
  
  drawTightClock(displayStr, settings.getTextSize(), textColors[settings.getBrightnessIndex()]);
}

// ===============================================
// MAIN PROGRAM LOGIC
// ===============================================

/**
 * Initialize hardware and system components
 */
void initializeSystem() {
  Serial.begin(9600);
  Serial.println("Matrix Sign Starting...");
  
  // Initialize settings manager
  settings.begin();
  
  // Initialize button manager
  buttons.begin();
  
  // Initialize matrix
  ProtomatterStatus status = matrix.begin();
  if (status != PROTOMATTER_OK) {
    Serial.print("Matrix initialization failed: ");
    Serial.println(status);
    while (true); // Halt on failure
  }
  Serial.println("Matrix initialized successfully");
  
  // Initialize display manager
  display.begin();
  
  // Initialize RTC
  Wire.begin();
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (true);
  }
  Serial.println("RTC initialized successfully");
  
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  // Initialize effects engine
  effects.begin();
  
  Serial.println("Setup complete!");
}

/**
 * Update the main display based on current state
 */
void updateDisplay() {
  // Clear screen
  matrix.fillScreen(0);
  
  // Render based on current state
  switch (appState) {
    case SHOW_TIME:
      buttons.setAllowButtonRepeat(false); // Disable repeating for normal clock display
      // Render background effects
      renderEffects();
      
      // Display current time
      {
        DateTime now = rtc.now();
        
        int hour = now.hour();
        bool isPM = false;
        String ampmStr = "";
        
        // Convert to 12-hour format if needed
        if (!settings.getUse24HourFormat()) {
          isPM = (hour >= 12);
          if (hour == 0) {
            hour = 12;  // Midnight is 12 AM
          } else if (hour > 12) {
            hour -= 12;  // Convert PM hours
          }
          // Use short form (A/P) for text size 3 to avoid corner collision
          if (settings.getTextSize() == 3) {
            ampmStr = isPM ? "P" : "A";
          } else {
            ampmStr = isPM ? "PM" : "AM";
          }
        }
        
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", 
                hour, now.minute(), now.second());
        drawTightClock(timeStr, settings.getTextSize(), getClockColor());
        
        // Display AM/PM in bottom right if using 12-hour format
        if (!settings.getUse24HourFormat()) {
          matrix.setTextSize(1);  // Small text for AM/PM
          
          // Position in bottom right corner
          int ampmX = MATRIX_WIDTH - (ampmStr.length() * 6) - 1;  // 6 pixels per char at size 1
          int ampmY = MATRIX_HEIGHT - 8;  // 8 pixels height at size 1
          
          uint16_t color = getClockColor();
          matrix.setCursor(ampmX, ampmY);
          matrix.setTextColor(color);
          matrix.print(ampmStr);
        }
      }
      break;
      
    case MENU:
      buttons.setAllowButtonRepeat(false); // Disable repeating for menu navigation
      displayMainMenu();
      break;
      
    case EDIT_TEXT_SIZE:
      buttons.setAllowButtonRepeat(false); // Disable repeating for text size menu
      displayTextSizeMenu();
      break;
      
    case EDIT_BRIGHTNESS:
      buttons.setAllowButtonRepeat(false); // Disable repeating for brightness menu
      displayBrightnessMenu();
      break;
      
    case EDIT_TIME_FORMAT:
      buttons.setAllowButtonRepeat(false); // Disable repeating for time format menu
      displayTimeFormatMenu();
      break;
      
    case EDIT_CLOCK_COLOR:
      buttons.setAllowButtonRepeat(false); // Disable repeating for clock color menu
      displayClockColorMenu();
      break;
      
    case EDIT_EFFECTS:
      buttons.setAllowButtonRepeat(false); // Disable repeating for effects menu
      displayEffectsMenu();
      break;
      
    case TIME_SET:
      buttons.setAllowButtonRepeat(true); // Enable repeating for time setting
      handleTimeSettingMode();
      break;
  }
  
  matrix.show();
}

/**
 * Handle user input based on current state
 */
void handleInput() {
  switch (appState) {
    case SHOW_TIME:
      handleMenuEntry();
      break;
      
    case MENU:
      handleMainMenuInput();
      break;
      
    case EDIT_TEXT_SIZE:
      handleTextSizeInput();
      break;
      
    case EDIT_BRIGHTNESS:
      handleBrightnessInput();
      break;
      
    case EDIT_TIME_FORMAT:
      handleTimeFormatInput();
      break;
      
    case EDIT_CLOCK_COLOR:
      handleClockColorInput();
      break;
      
    case EDIT_EFFECTS:
      handleEffectsMenuInput();
      break;
      
    case TIME_SET:
      handleTimeSettingMode();
      break;
  }
}

// ===============================================
// ARDUINO MAIN FUNCTIONS
// ===============================================

void setup() {
  initializeSystem();
}

void loop() {
  buttons.updateAll();
  handleInput();
  updateDisplay();
  
  // Small delay to prevent overwhelming the display
  if (appState == SHOW_TIME) {
    delay(CLOCK_UPDATE_DELAY);
  } else {
    delay(MENU_DELAY);
  }
}
