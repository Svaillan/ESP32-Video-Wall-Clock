#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>

// EEPROM Settings
#define EEPROM_SIZE 64              // Size in bytes
#define EEPROM_MAGIC 0x42           // Magic number to verify valid data
#define EEPROM_ADDR_MAGIC 0         // Address for magic number
#define EEPROM_ADDR_TEXT_SIZE 1     // Address for text size
#define EEPROM_ADDR_BRIGHTNESS 2    // Address for brightness index
#define EEPROM_ADDR_EFFECT_MODE 3   // Address for effect mode
#define EEPROM_ADDR_TIME_FORMAT 4   // Address for time format (12/24 hour)
#define EEPROM_ADDR_CLOCK_COLOR 5   // Address for clock color mode

// Constants
#define TEXT_SIZE_MIN 1
#define TEXT_SIZE_MAX 3
#define BRIGHTNESS_LEVELS 10

enum EffectMode { 
  EFFECT_CONFETTI, 
  EFFECT_ACID,
  EFFECT_RAIN,
  EFFECT_TORRENT,
  EFFECT_STARS,
  EFFECT_SPARKLES,
  EFFECT_FIREWORKS,
  EFFECT_TRON,
  EFFECT_OFF 
};

enum ClockColorMode {
  CLOCK_WHITE,
  CLOCK_RED,
  CLOCK_GREEN,
  CLOCK_BLUE,
  CLOCK_YELLOW,
  CLOCK_CYAN,
  CLOCK_MAGENTA,
  CLOCK_ORANGE,
  CLOCK_PURPLE,
  CLOCK_PINK,
  CLOCK_LIME,
  CLOCK_TEAL,
  CLOCK_INDIGO,
  CLOCK_GOLD,
  CLOCK_SILVER,
  CLOCK_RAINBOW
};

class SettingsManager {
public:
    // Constructor
    SettingsManager();
    
    // Initialization
    void begin();
    
    // Settings getters
    int getTextSize() const { return textSize; }
    int getBrightnessIndex() const { return brightnessIndex; }
    EffectMode getEffectMode() const { return effectMode; }
    bool getUse24HourFormat() const { return use24HourFormat; }
    ClockColorMode getClockColorMode() const { return clockColorMode; }
    
    // Settings setters
    void setTextSize(int size);
    void setBrightnessIndex(int index);
    void setEffectMode(EffectMode mode);
    void setUse24HourFormat(bool format);
    void setClockColorMode(ClockColorMode mode);
    
    // Save/Load operations
    void saveSettings();
    void loadSettings();
    
private:
    // Settings variables
    int textSize;
    int brightnessIndex;
    EffectMode effectMode;
    bool use24HourFormat;
    ClockColorMode clockColorMode;
    
    // Validation functions
    bool isValidTextSize(int size) const;
    bool isValidBrightnessIndex(int index) const;
    bool isValidEffectMode(int mode) const;
    bool isValidClockColorMode(int mode) const;
};

#endif // SETTINGS_MANAGER_H
