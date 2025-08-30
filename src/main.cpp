#include <Arduino.h>
#include <Adafruit_Protomatter.h>
#include <Wire.h>
#include <RTClib.h>
#include <EEPROM.h>
#include "SettingsManager.h"
#include "ButtonManager.h"

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

// Confetti Settings
#define NUM_CONFETTI   40
#define CONFETTI_RAD   1

// Matrix Rain Settings
#define NUM_MATRIX_DROPS 12
#define MATRIX_CHAR_DELAY 80

// Torrent Settings
#define NUM_TORRENT_DROPS 30
#define TORRENT_CHAR_DELAY 60

// Star Settings
#define NUM_STARS 25
#define STAR_TWINKLE_CHANCE 5

// Shooting Star Settings
#define NUM_SHOOTING_STARS 2
#define SHOOTING_STAR_SPEED 0.8f
#define SHOOTING_STAR_TRAIL_LENGTH 8

// Sparkle Settings
#define NUM_SPARKLES 30
#define SPARKLE_DURATION 800

// Fireworks Settings
#define NUM_FIREWORKS 8
#define FIREWORK_PARTICLES 15
#define FIREWORK_LIFE 1500

// Tron Settings
#define NUM_TRON_TRAILS 12
#define TRON_MIN_LENGTH 8
#define TRON_MAX_LENGTH 20
#define TRON_MIN_SPEED 80
#define TRON_MAX_SPEED 200

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

struct Confetti { 
  float x, y, vx, vy; 
  uint16_t color; 
};

struct MatrixDrop {
  float x, y;
  uint8_t length;
  uint8_t speed;
  uint32_t lastUpdate;
};

struct Star {
  float x, y;  // Current position (can be off-screen)
  uint8_t brightness;
  uint8_t twinkleState;
  uint32_t lastTwinkle;
};

struct ShootingStar {
  float x, y;
  float speedX, speedY;
  bool active;
  uint8_t trailLength;
  uint32_t spawnTime;
};

struct Sparkle {
  uint8_t x, y;
  uint8_t brightness;
  uint16_t color;
  uint32_t startTime;
  uint16_t duration;
};

struct Firework {
  float x, y;
  float vx[FIREWORK_PARTICLES], vy[FIREWORK_PARTICLES];
  uint8_t px[FIREWORK_PARTICLES], py[FIREWORK_PARTICLES];
  uint16_t color;
  uint32_t startTime;
  bool active;
  bool exploded;
};

struct TronTrail {
  uint8_t x, y;
  uint8_t direction;  // 0=right, 1=down, 2=left, 3=up
  uint8_t trailPositions[TRON_MAX_LENGTH][2];  // Store trail segment positions
  uint8_t currentLength;
  uint16_t color;
  uint32_t lastMove;
  uint16_t speed;
  bool active;
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

// Effects
Confetti confetti[NUM_CONFETTI];
MatrixDrop matrixDrops[NUM_MATRIX_DROPS];
MatrixDrop torrentDrops[NUM_TORRENT_DROPS];
Star stars[NUM_STARS];
ShootingStar shootingStars[NUM_SHOOTING_STARS];
Sparkle sparkles[NUM_SPARKLES];
Firework fireworks[NUM_FIREWORKS];
TronTrail tronTrails[NUM_TRON_TRAILS];

// Shooting star timing variables
uint32_t lastShootingStarTime = 0;
bool waitingForSecondStar = false;
uint32_t secondStarTimer = 0;

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
void initializeConfetti() {
  int16_t centerY = MATRIX_HEIGHT / 2;
  int16_t yRange = 12;
  
  for (int i = 0; i < NUM_CONFETTI; i++) {
    confetti[i].x = random(0, MATRIX_WIDTH);
    confetti[i].y = random(centerY - yRange, centerY + yRange);
    
    // Ensure minimum velocity to prevent stuck particles
    confetti[i].vx = generateVelocity(0.1, 0.8);  // Min 0.1, max 0.8 pixels/frame
    confetti[i].vy = generateVelocity(0.05, 0.4); // Min 0.05, max 0.4 pixels/frame
    
    confetti[i].color = randomVividColor();
  }
}

/**
 * Reset a confetti particle to a new random position
 */
void resetConfettiParticle(int index) {
  int16_t centerY = MATRIX_HEIGHT / 2;
  int16_t yRange = 12;
  
  // Spawn from edges for more dynamic movement
  if (random(2) == 0) {
    // Spawn from left or right edge
    confetti[index].x = random(2) == 0 ? -CONFETTI_RAD : MATRIX_WIDTH + CONFETTI_RAD;
    confetti[index].y = random(centerY - yRange, centerY + yRange);
  } else {
    // Spawn from top or bottom edge
    confetti[index].x = random(0, MATRIX_WIDTH);
    confetti[index].y = random(2) == 0 ? -CONFETTI_RAD : MATRIX_HEIGHT + CONFETTI_RAD;
  }
  
  // Ensure minimum velocity to prevent stuck particles
  confetti[index].vx = generateVelocity(0.1, 0.8);
  confetti[index].vy = generateVelocity(0.05, 0.4);
  
  confetti[index].color = randomVividColor();
}

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
 * Update and draw confetti animation with text masking
 */
void updateConfetti() {
  // First pass: update positions
  for (int i = 0; i < NUM_CONFETTI; i++) {
    // Update position
    confetti[i].x += confetti[i].vx;
    confetti[i].y += confetti[i].vy;
    
    // Reset if particle goes off screen
    if (confetti[i].x < -CONFETTI_RAD * 2 || confetti[i].x > MATRIX_WIDTH + CONFETTI_RAD * 2 ||
        confetti[i].y < -CONFETTI_RAD * 2 || confetti[i].y > MATRIX_HEIGHT + CONFETTI_RAD * 2) {
      resetConfettiParticle(i);
    }
    
    // If particle has been moving too slowly for too long, reset it
    if (abs(confetti[i].vx) < 0.05 && abs(confetti[i].vy) < 0.05) {
      resetConfettiParticle(i);
    }
  }
  
  // Second pass: draw particles that are not behind text
  for (int i = 0; i < NUM_CONFETTI; i++) {
    int px = (int)confetti[i].x;
    int py = (int)confetti[i].y;
    
    // Only draw if not in text area
    if (!isInTextArea(px, py)) {
      uint16_t brightColor = applyEffectBrightness(confetti[i].color);
      matrix.fillCircle(px, py, CONFETTI_RAD, brightColor);
    }
  }
}

/**
 * Initialize matrix rain drops
 */
void initializeMatrixRain() {
  for (int i = 0; i < NUM_MATRIX_DROPS; i++) {
    matrixDrops[i].x = random(0, MATRIX_WIDTH);
    matrixDrops[i].y = random(-20, 0);  // Start above screen
    matrixDrops[i].length = random(3, 8);
    matrixDrops[i].speed = random(1, 4);
    matrixDrops[i].lastUpdate = millis();
  }
}

/**
 * Update matrix rain effect
 */
void updateMatrixRain() {
  uint32_t currentTime = millis();
  
  for (int i = 0; i < NUM_MATRIX_DROPS; i++) {
    if (currentTime - matrixDrops[i].lastUpdate > MATRIX_CHAR_DELAY / matrixDrops[i].speed) {
      matrixDrops[i].y += matrixDrops[i].speed;
      matrixDrops[i].lastUpdate = currentTime;
      
      // Reset when drop goes off screen
      if (matrixDrops[i].y > MATRIX_HEIGHT + matrixDrops[i].length) {
        matrixDrops[i].x = random(0, MATRIX_WIDTH);
        matrixDrops[i].y = random(-20, -5);
        matrixDrops[i].length = random(3, 8);
        matrixDrops[i].speed = random(1, 4);
      }
    }
    
    // Draw the drop trail
    for (int j = 0; j < matrixDrops[i].length && (matrixDrops[i].y - j) >= 0; j++) {
      int y = matrixDrops[i].y - j;
      if (y < MATRIX_HEIGHT && !isInTextArea(matrixDrops[i].x, y)) {
        uint8_t intensity = 255 - (j * 40);  // Fade as we go up
        if (intensity < 50) intensity = 50;
        uint16_t color = scaledEffectColor565(0, intensity, 0);  // Green rain with brightness scaling
        matrix.drawPixel(matrixDrops[i].x, y, color);
      }
    }
  }
}

/**
 * Initialize rain (blue raindrops)
 */
void initializeRain() {
  for (int i = 0; i < NUM_MATRIX_DROPS; i++) {
    matrixDrops[i].x = random(0, MATRIX_WIDTH);
    matrixDrops[i].y = random(-20, 0);  // Start above screen
    matrixDrops[i].length = random(3, 8);
    matrixDrops[i].speed = random(1, 4);
    matrixDrops[i].lastUpdate = millis();
  }
}

/**
 * Update rain effect (blue raindrops)
 */
void updateRain() {
  uint32_t currentTime = millis();
  
  for (int i = 0; i < NUM_MATRIX_DROPS; i++) {
    if (currentTime - matrixDrops[i].lastUpdate > MATRIX_CHAR_DELAY / matrixDrops[i].speed) {
      matrixDrops[i].y += matrixDrops[i].speed;
      matrixDrops[i].lastUpdate = currentTime;
      
      // Reset when drop goes off screen
      if (matrixDrops[i].y > MATRIX_HEIGHT + matrixDrops[i].length) {
        matrixDrops[i].x = random(0, MATRIX_WIDTH);
        matrixDrops[i].y = random(-20, -5);
        matrixDrops[i].length = random(3, 8);
        matrixDrops[i].speed = random(1, 4);
      }
    }
    
    // Draw the drop trail in blue
    for (int j = 0; j < matrixDrops[i].length && (matrixDrops[i].y - j) >= 0; j++) {
      int y = matrixDrops[i].y - j;
      if (y < MATRIX_HEIGHT && !isInTextArea(matrixDrops[i].x, y)) {
        uint8_t intensity = 255 - (j * 40);  // Fade as we go up
        if (intensity < 50) intensity = 50;
        uint16_t color = scaledEffectColor565(0, 0, intensity);  // Blue rain with brightness scaling
        matrix.drawPixel(matrixDrops[i].x, y, color);
      }
    }
  }
}

/**
 * Initialize torrent (many small raindrops)
 */
void initializeTorrent() {
  for (int i = 0; i < NUM_TORRENT_DROPS; i++) {
    torrentDrops[i].x = random(0, MATRIX_WIDTH);
    torrentDrops[i].y = random(-30, 0);  // Start above screen
    torrentDrops[i].length = random(1, 4);  // Smaller drops
    torrentDrops[i].speed = random(2, 6);   // Faster speed
    torrentDrops[i].lastUpdate = millis();
  }
}

/**
 * Update torrent effect (many small fast raindrops)
 */
void updateTorrent() {
  uint32_t currentTime = millis();
  
  for (int i = 0; i < NUM_TORRENT_DROPS; i++) {
    if (currentTime - torrentDrops[i].lastUpdate > TORRENT_CHAR_DELAY / torrentDrops[i].speed) {
      torrentDrops[i].y += torrentDrops[i].speed;
      torrentDrops[i].lastUpdate = currentTime;
      
      // Reset when drop goes off screen
      if (torrentDrops[i].y > MATRIX_HEIGHT + torrentDrops[i].length) {
        torrentDrops[i].x = random(0, MATRIX_WIDTH);
        torrentDrops[i].y = random(-30, -5);
        torrentDrops[i].length = random(1, 4);  // Smaller drops
        torrentDrops[i].speed = random(2, 6);   // Faster speed
      }
    }
    
    // Draw the drop trail in white/light blue
    for (int j = 0; j < torrentDrops[i].length && (torrentDrops[i].y - j) >= 0; j++) {
      int y = torrentDrops[i].y - j;
      if (y < MATRIX_HEIGHT && !isInTextArea(torrentDrops[i].x, y)) {
        uint8_t intensity = 255 - (j * 60);  // Faster fade for smaller drops
        if (intensity < 80) intensity = 80;
        uint16_t color = scaledEffectColor565(intensity/2, intensity/2, intensity);  // Light blue/white torrent
        matrix.drawPixel(torrentDrops[i].x, y, color);
      }
    }
  }
}

/**
 * Initialize stars
 */
void initializeStars() {
  for (int i = 0; i < NUM_STARS; i++) {
    stars[i].x = random(0, MATRIX_WIDTH * 100) / 100.0f;  // Sub-pixel precision
    stars[i].y = random(0, MATRIX_HEIGHT * 100) / 100.0f;
    stars[i].brightness = random(50, 255);
    stars[i].twinkleState = random(0, 2);
    stars[i].lastTwinkle = millis() + random(0, 2000);
  }
}

/**
 * Initialize shooting stars
 */
void initializeShootingStars() {
  for (int i = 0; i < NUM_SHOOTING_STARS; i++) {
    shootingStars[i].active = false;
  }
  lastShootingStarTime = millis();
}

/**
 * Spawn a new shooting star
 */
void spawnShootingStar(int index) {
  // Start from random position at the top or left edge
  if (random(0, 2) == 0) {
    // Start from top edge
    shootingStars[index].x = random(0, MATRIX_WIDTH);
    shootingStars[index].y = 0;
  } else {
    // Start from left edge
    shootingStars[index].x = 0;
    shootingStars[index].y = random(0, MATRIX_HEIGHT);
  }
  
  // Diagonal movement (northwest to southeast)
  shootingStars[index].speedX = SHOOTING_STAR_SPEED;
  shootingStars[index].speedY = SHOOTING_STAR_SPEED * 0.7f;  // Slightly less vertical speed
  shootingStars[index].active = true;
  shootingStars[index].trailLength = SHOOTING_STAR_TRAIL_LENGTH;
  shootingStars[index].spawnTime = millis();
}

/**
 * Update drifting starfield effect - stars slowly drift across the sky
 */
void updateStars() {
  uint32_t currentTime = millis();
  static uint32_t lastDriftUpdate = 0;
  static float globalDriftX = 0.005f;  // Slowed down to 10% of original speed (0.05f -> 0.005f)
  static float globalDriftY = 0.003f;  // Slowed down to 10% of original speed (0.03f -> 0.003f)
  
  // Slow drift movement - update every 150ms
  if (currentTime - lastDriftUpdate > 150) {
    // Use FIXED drift values for perfect group movement (no random variations during movement)
    float groupDriftX = globalDriftX;  // Completely fixed drift
    float groupDriftY = globalDriftY;
    
    // Apply the SAME movement to all stars as a unified group
    for (int i = 0; i < NUM_STARS; i++) {
      // All stars move together with the exact same drift
      stars[i].x += groupDriftX;
      stars[i].y += groupDriftY;
      
      // If star has drifted off the right or bottom edge, respawn it on the opposite edges
      if (stars[i].x > MATRIX_WIDTH + 2 || stars[i].y > MATRIX_HEIGHT + 2) {
        // Spawn from left or top edges with proper coordinates
        if (stars[i].x > MATRIX_WIDTH + 2) {
          // Respawn on left side
          stars[i].x = -1.5f;  // Fixed spawn position to avoid random() calls
          stars[i].y = (i * MATRIX_HEIGHT / NUM_STARS) + (i % 3 - 1);  // Distribute evenly with slight offset
        }
        if (stars[i].y > MATRIX_HEIGHT + 2) {
          // Respawn on top side
          stars[i].y = -1.5f;  // Fixed spawn position
          stars[i].x = (i * MATRIX_WIDTH / NUM_STARS) + (i % 3 - 1);   // Distribute evenly with slight offset
        }
        // Keep existing brightness to avoid random() call
      }
    }
    lastDriftUpdate = currentTime;
  }
  
  // Shooting star management
  // Check if it's time to spawn a shooting star (every 5-10 minutes)
  if (currentTime - lastShootingStarTime > random(300000, 600000)) {  // 5-10 minutes
  // if (currentTime - lastShootingStarTime > 5000) {  // TESTING: 5 seconds (COMMENTED OUT)
    // Find an inactive shooting star to spawn
    for (int i = 0; i < NUM_SHOOTING_STARS; i++) {
      if (!shootingStars[i].active) {
        spawnShootingStar(i);
        lastShootingStarTime = currentTime;
        
        // 30% chance for a second shooting star
        if (random(0, 100) < 30) {
          waitingForSecondStar = true;
          secondStarTimer = currentTime + random(2000, 5000);  // 2-5 seconds later
        }
        break;
      }
    }
  }
  
  // Check for second shooting star
  if (waitingForSecondStar && currentTime >= secondStarTimer) {
    for (int i = 0; i < NUM_SHOOTING_STARS; i++) {
      if (!shootingStars[i].active) {
        spawnShootingStar(i);
        waitingForSecondStar = false;
        break;
      }
    }
  }
  
  // Update shooting stars
  for (int i = 0; i < NUM_SHOOTING_STARS; i++) {
    if (shootingStars[i].active) {
      shootingStars[i].x += shootingStars[i].speedX;
      shootingStars[i].y += shootingStars[i].speedY;
      
      // Deactivate if off screen
      if (shootingStars[i].x > MATRIX_WIDTH + 10 || shootingStars[i].y > MATRIX_HEIGHT + 10) {
        shootingStars[i].active = false;
      } else {
        // Draw shooting star trail
        for (int t = 0; t < shootingStars[i].trailLength; t++) {
          int trailX = (int)(shootingStars[i].x - t * shootingStars[i].speedX * 0.5f);
          int trailY = (int)(shootingStars[i].y - t * shootingStars[i].speedY * 0.5f);
          
          if (trailX >= 0 && trailX < MATRIX_WIDTH && trailY >= 0 && trailY < MATRIX_HEIGHT) {
            if (!isInTextArea(trailX, trailY)) {
              uint8_t brightness = 255 - (t * 32);  // Fade trail
              uint16_t color = scaledEffectColor565(brightness, brightness, brightness);
              matrix.drawPixel(trailX, trailY, color);
            }
          }
        }
      }
    }
  }
  
  for (int i = 0; i < NUM_STARS; i++) {
    // Handle twinkling
    if (currentTime - stars[i].lastTwinkle > random(800, 2000)) {
      stars[i].twinkleState = !stars[i].twinkleState;
      stars[i].lastTwinkle = currentTime;
    }
    
    // Draw star if on screen and not in text area
    int pixelX = (int)round(stars[i].x);
    int pixelY = (int)round(stars[i].y);
    
    if (pixelX >= 0 && pixelX < MATRIX_WIDTH && pixelY >= 0 && pixelY < MATRIX_HEIGHT) {
      if (!isInTextArea(pixelX, pixelY)) {
        uint8_t brightness = stars[i].twinkleState ? stars[i].brightness : stars[i].brightness / 3;
        uint16_t color = scaledEffectColor565(brightness, brightness, brightness);
        matrix.drawPixel(pixelX, pixelY, color);
      }
    }
  }
}

/**
 * Initialize sparkles
 */
void initializeSparkles() {
  for (int i = 0; i < NUM_SPARKLES; i++) {
    sparkles[i].x = random(0, MATRIX_WIDTH);
    sparkles[i].y = random(0, MATRIX_HEIGHT);
    sparkles[i].brightness = 0;
    sparkles[i].color = randomVividColor();  // Assign random vivid color
    sparkles[i].startTime = millis() + random(0, 3000);
    sparkles[i].duration = random(400, SPARKLE_DURATION);
  }
}

/**
 * Update sparkle effect
 */
void updateSparkles() {
  uint32_t currentTime = millis();
  
  for (int i = 0; i < NUM_SPARKLES; i++) {
    uint32_t elapsed = currentTime - sparkles[i].startTime;
    
    if (elapsed < sparkles[i].duration) {
      // Calculate sparkle brightness (fade in and out)
      float progress = (float)elapsed / sparkles[i].duration;
      float brightness = sin(progress * PI) * 255;
      sparkles[i].brightness = (uint8_t)brightness;
      
      // Draw sparkle if not in text area
      if (!isInTextArea(sparkles[i].x, sparkles[i].y)) {
        // Scale the stored color by brightness and apply global brightness
        uint8_t r = ((sparkles[i].color >> 11) & 0x1F) * sparkles[i].brightness / 255;
        uint8_t g = ((sparkles[i].color >> 5) & 0x3F) * sparkles[i].brightness / 255;
        uint8_t b = (sparkles[i].color & 0x1F) * sparkles[i].brightness / 255;
        uint16_t scaledColor = scaledEffectColor565(r << 3, g << 2, b << 3);
        matrix.drawPixel(sparkles[i].x, sparkles[i].y, scaledColor);
      }
    } else {
      // Reset sparkle to new position with new color
      sparkles[i].x = random(0, MATRIX_WIDTH);
      sparkles[i].y = random(0, MATRIX_HEIGHT);
      sparkles[i].color = randomVividColor();  // New random color
      sparkles[i].startTime = currentTime + random(0, 2000);
      sparkles[i].duration = random(400, SPARKLE_DURATION);
    }
  }
}

/**
 * Initialize fireworks
 */
void initializeFireworks() {
  for (int i = 0; i < NUM_FIREWORKS; i++) {
    fireworks[i].active = false;
    fireworks[i].exploded = false;
    fireworks[i].startTime = millis() + random(0, 3000);  // Stagger initial launches
  }
}

/**
 * Update fireworks effect
 */
void updateFireworks() {
  uint32_t currentTime = millis();
  
  for (int i = 0; i < NUM_FIREWORKS; i++) {
    if (!fireworks[i].active) {
      // Launch new firework randomly
      if (currentTime > fireworks[i].startTime) {
        // Choose launch and explosion position to avoid text area
        int textCenterX = MATRIX_WIDTH / 2;
        int textCenterY = getCenteredY(settings.getTextSize());
        
        // Launch from sides or center, but explode away from text
        if (random(0, 2) == 0) {
          // Launch from left side, explode on left
          fireworks[i].x = random(5, MATRIX_WIDTH / 3);
        } else {
          // Launch from right side, explode on right  
          fireworks[i].x = random(2 * MATRIX_WIDTH / 3, MATRIX_WIDTH - 5);
        }
        
        fireworks[i].y = MATRIX_HEIGHT;  // Start from bottom
        fireworks[i].color = randomVividColor();  // Color for explosion
        fireworks[i].active = true;
        fireworks[i].exploded = false;
        fireworks[i].startTime = currentTime;
      }
    } else if (!fireworks[i].exploded) {
      // Rising phase - draw white rocket moving up
      uint32_t elapsed = currentTime - fireworks[i].startTime;
      float progress = elapsed / 800.0f;  // 800ms rise time
      
      if (progress >= 1.0f) {
        // Time to explode - initialize particles
        fireworks[i].exploded = true;
        fireworks[i].startTime = currentTime;
        
        for (int j = 0; j < FIREWORK_PARTICLES; j++) {
          float angle = (j * 2.0f * PI) / FIREWORK_PARTICLES;
          float speed = random(100, 250) / 100.0f;  // Increased speed for more dramatic spread
          fireworks[i].vx[j] = cos(angle) * speed;
          fireworks[i].vy[j] = sin(angle) * speed;
        }
      } else {
        // Draw rising white rocket
        int rocketY = fireworks[i].y - (progress * (MATRIX_HEIGHT / 2));
        if (rocketY >= 0 && rocketY < MATRIX_HEIGHT && !isInTextArea(fireworks[i].x, rocketY)) {
          // White rocket trail
          uint16_t whiteColor = applyEffectBrightness(matrix.color565(255, 255, 255));
          matrix.drawPixel(fireworks[i].x, rocketY, whiteColor);
        }
      }
    } else {
      // Explosion phase - draw expanding particles
      uint32_t elapsed = currentTime - fireworks[i].startTime;
      float progress = elapsed / 1000.0f;  // Reduced from 1500ms to 1000ms for faster fade
      
      if (progress >= 1.0f) {
        // Firework finished, reset for next launch
        fireworks[i].active = false;
        fireworks[i].startTime = currentTime + random(2000, 5000);  // Wait 2-5 seconds
      } else {
        // Draw explosion particles with more dramatic spread
        float centerX = fireworks[i].x;
        float centerY = fireworks[i].y - (MATRIX_HEIGHT / 2);
        
        for (int j = 0; j < FIREWORK_PARTICLES; j++) {
          // Increased spread speed and reduced gravity effect
          float px = centerX + (fireworks[i].vx[j] * elapsed / 50.0f);  // Increased from 100 to 50 for faster spread
          float py = centerY + (fireworks[i].vy[j] * elapsed / 50.0f) + (0.1f * elapsed * elapsed / 10000.0f);  // Much reduced gravity
          
          if (px >= 0 && px < MATRIX_WIDTH && py >= 0 && py < MATRIX_HEIGHT) {
            if (!isInTextArea(px, py)) {
              // More dramatic fade - particles disappear while still in sky
              float fade = (1.0f - progress) * (1.0f - progress);  // Exponential fade for more dramatic effect
              if (fade > 0.1f) {  // Only draw if fade is significant
                uint16_t fadedColor = applyEffectBrightness(scaleBrightness(fireworks[i].color, fade));
                matrix.drawPixel((int)px, (int)py, fadedColor);
              }
            }
          }
        }
      }
    }
  }
}

/**
 * Initialize Tron trails
 */
void initializeTron() {
  for (int i = 0; i < NUM_TRON_TRAILS; i++) {
    tronTrails[i].active = false;
    tronTrails[i].currentLength = 0;
    tronTrails[i].lastMove = millis() + random(0, 2000);  // Stagger initial starts
  }
}

/**
 * Update Tron effect - creates light trails across the grid
 */
void updateTron() {
  uint32_t currentTime = millis();
  
  for (int i = 0; i < NUM_TRON_TRAILS; i++) {
    if (!tronTrails[i].active) {
      // Start new trail
      if (currentTime > tronTrails[i].lastMove) {
        tronTrails[i].active = true;
        tronTrails[i].currentLength = 0;
        tronTrails[i].speed = random(TRON_MIN_SPEED, TRON_MAX_SPEED + 1);
        tronTrails[i].direction = random(0, 4);  // 0=right, 1=down, 2=left, 3=up
        
        // Choose starting position based on direction
        switch (tronTrails[i].direction) {
          case 0: // Moving right
            tronTrails[i].x = 0;
            tronTrails[i].y = random(0, MATRIX_HEIGHT);
            break;
          case 1: // Moving down
            tronTrails[i].x = random(0, MATRIX_WIDTH);
            tronTrails[i].y = 0;
            break;
          case 2: // Moving left
            tronTrails[i].x = MATRIX_WIDTH - 1;
            tronTrails[i].y = random(0, MATRIX_HEIGHT);
            break;
          case 3: // Moving up
            tronTrails[i].x = random(0, MATRIX_WIDTH);
            tronTrails[i].y = MATRIX_HEIGHT - 1;
            break;
        }
        
        // Choose Tron-like colors (cyan, blue, white)
        int colorChoice = random(0, 3);
        switch (colorChoice) {
          case 0: tronTrails[i].color = matrix.color565(0, 255, 255); break;    // Cyan
          case 1: tronTrails[i].color = matrix.color565(0, 150, 255); break;   // Light blue
          case 2: tronTrails[i].color = matrix.color565(255, 255, 255); break; // White
        }
        
        // Add starting position to trail
        tronTrails[i].trailPositions[0][0] = tronTrails[i].x;
        tronTrails[i].trailPositions[0][1] = tronTrails[i].y;
        tronTrails[i].currentLength = 1;
        tronTrails[i].lastMove = currentTime;
      }
    } else {
      // Update active trail
      if (currentTime - tronTrails[i].lastMove >= tronTrails[i].speed) {
        // Move trail head
        switch (tronTrails[i].direction) {
          case 0: tronTrails[i].x++; break; // Right
          case 1: tronTrails[i].y++; break; // Down
          case 2: tronTrails[i].x--; break; // Left
          case 3: tronTrails[i].y--; break; // Up
        }
        
        // Add new head position to trail if on screen
        if (tronTrails[i].x >= 0 && tronTrails[i].x < MATRIX_WIDTH && 
            tronTrails[i].y >= 0 && tronTrails[i].y < MATRIX_HEIGHT) {
          
          // Shift trail positions
          if (tronTrails[i].currentLength >= TRON_MAX_LENGTH) {
            // Remove tail
            for (int j = 0; j < TRON_MAX_LENGTH - 1; j++) {
              tronTrails[i].trailPositions[j][0] = tronTrails[i].trailPositions[j + 1][0];
              tronTrails[i].trailPositions[j][1] = tronTrails[i].trailPositions[j + 1][1];
            }
            tronTrails[i].currentLength = TRON_MAX_LENGTH - 1;
          }
          
          // Add new head
          tronTrails[i].trailPositions[tronTrails[i].currentLength][0] = tronTrails[i].x;
          tronTrails[i].trailPositions[tronTrails[i].currentLength][1] = tronTrails[i].y;
          tronTrails[i].currentLength++;
        }
        
        // Draw the trail
        for (int j = 0; j < tronTrails[i].currentLength; j++) {
          uint8_t segX = tronTrails[i].trailPositions[j][0];
          uint8_t segY = tronTrails[i].trailPositions[j][1];
          
          if (segX < MATRIX_WIDTH && segY < MATRIX_HEIGHT) {
            if (!isInTextArea(segX, segY)) {
              // Fade from dim at tail to bright at head
              float brightness = (float)(j + 1) / tronTrails[i].currentLength;
              uint16_t fadedColor = applyEffectBrightness(scaleBrightness(tronTrails[i].color, brightness));
              matrix.drawPixel(segX, segY, fadedColor);
            }
          }
        }
        
        // Check if trail is off screen
        if (tronTrails[i].x < -5 || tronTrails[i].x > MATRIX_WIDTH + 5 ||
            tronTrails[i].y < -5 || tronTrails[i].y > MATRIX_HEIGHT + 5) {
          tronTrails[i].active = false;
          tronTrails[i].lastMove = currentTime + random(1000, 3000);  // Wait before next trail
        } else {
          tronTrails[i].lastMove = currentTime;
        }
      } else {
        // Still draw existing trail even when not moving
        for (int j = 0; j < tronTrails[i].currentLength; j++) {
          uint8_t segX = tronTrails[i].trailPositions[j][0];
          uint8_t segY = tronTrails[i].trailPositions[j][1];
          
          if (segX < MATRIX_WIDTH && segY < MATRIX_HEIGHT) {
            if (!isInTextArea(segX, segY)) {
              // Fade from dim at tail to bright at head
              float brightness = (float)(j + 1) / tronTrails[i].currentLength;
              uint16_t fadedColor = applyEffectBrightness(scaleBrightness(tronTrails[i].color, brightness));
              matrix.drawPixel(segX, segY, fadedColor);
            }
          }
        }
      }
    }
  }
}

/**
 * Draw black background rectangles around the text areas
 */
void drawTextBackground() {
  if (appState != SHOW_TIME) return;
  
  // Draw background for main clock
  int x1, y1, x2, y2;
  getTimeDisplayBounds(x1, y1, x2, y2);
  matrix.fillRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1, 0x0000);
  
  // Draw background for AM/PM if in 12-hour format
  if (!settings.getUse24HourFormat()) {
    getAMPMDisplayBounds(x1, y1, x2, y2);
    matrix.fillRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1, 0x0000);
  }
}

/**
 * Render background effects
 */
void renderEffects() {
  switch (settings.getEffectMode()) {
    case EFFECT_CONFETTI:
      updateConfetti();
      drawTextBackground();  // Draw black background behind text
      break;
      
    case EFFECT_ACID:
      updateMatrixRain();  // Green acid rain
      drawTextBackground();
      break;
      
    case EFFECT_RAIN:
      updateRain();  // Blue rain
      drawTextBackground();
      break;
      
    case EFFECT_TORRENT:
      updateTorrent();  // Heavy rain with many small drops
      drawTextBackground();
      break;
      
    case EFFECT_STARS:
      updateStars();
      drawTextBackground();
      break;
      
    case EFFECT_SPARKLES:
      updateSparkles();
      drawTextBackground();
      break;
      
    case EFFECT_FIREWORKS:
      updateFireworks();
      drawTextBackground();
      break;
      
    case EFFECT_TRON:
      updateTron();
      drawTextBackground();
      break;
      
    case EFFECT_OFF:
    default:
      // No effects, just black background
      break;
  }
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
  
  matrix.setTextWrap(false);
  matrix.setTextColor(textColors[settings.getBrightnessIndex()]);
  matrix.setTextSize(settings.getTextSize());
  
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
  
  // Initialize effects
  initializeConfetti();
  initializeMatrixRain();  // Acid rain (green)
  initializeRain();        // Blue rain
  initializeTorrent();     // Heavy rain
  initializeStars();
  initializeShootingStars();
  initializeSparkles();
  initializeFireworks();
  initializeTron();         // Tron light trails
  
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
