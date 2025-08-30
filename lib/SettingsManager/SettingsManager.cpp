#include "SettingsManager.h"

SettingsManager::SettingsManager() {
    // Initialize with default values
    textSize = 2;
    brightnessIndex = 9;  // Default to brightness level 10 (100%)
    effectMode = EFFECT_CONFETTI;
    use24HourFormat = true;  // Default to 24-hour format
    clockColorMode = CLOCK_WHITE;
}

void SettingsManager::begin() {
    EEPROM.begin(EEPROM_SIZE);
    Serial.println("EEPROM initialized");
    loadSettings();
}

void SettingsManager::setTextSize(int size) {
    if (isValidTextSize(size)) {
        textSize = size;
    }
}

void SettingsManager::setBrightnessIndex(int index) {
    if (isValidBrightnessIndex(index)) {
        brightnessIndex = index;
    }
}

void SettingsManager::setEffectMode(EffectMode mode) {
    effectMode = mode;
}

void SettingsManager::setUse24HourFormat(bool format) {
    use24HourFormat = format;
}

void SettingsManager::setClockColorMode(ClockColorMode mode) {
    clockColorMode = mode;
}

void SettingsManager::saveSettings() {
    EEPROM.write(EEPROM_ADDR_MAGIC, EEPROM_MAGIC);
    EEPROM.write(EEPROM_ADDR_TEXT_SIZE, textSize);
    EEPROM.write(EEPROM_ADDR_BRIGHTNESS, brightnessIndex);
    EEPROM.write(EEPROM_ADDR_EFFECT_MODE, (uint8_t)effectMode);
    EEPROM.write(EEPROM_ADDR_TIME_FORMAT, use24HourFormat ? 1 : 0);
    EEPROM.write(EEPROM_ADDR_CLOCK_COLOR, (uint8_t)clockColorMode);
    EEPROM.commit();
    
    Serial.println("Settings saved to EEPROM");
    Serial.print("Text Size: "); Serial.println(textSize);
    Serial.print("Brightness: "); Serial.println(brightnessIndex + 1);
    Serial.print("Effect Mode: "); Serial.println((int)effectMode);
    Serial.print("Time Format: "); Serial.println(use24HourFormat ? "24H" : "12H");
    Serial.print("Clock Color: "); Serial.println((int)clockColorMode);
}

void SettingsManager::loadSettings() {
    uint8_t magic = EEPROM.read(EEPROM_ADDR_MAGIC);
    
    if (magic == EEPROM_MAGIC) {
        // Valid EEPROM data found, load settings
        uint8_t savedTextSize = EEPROM.read(EEPROM_ADDR_TEXT_SIZE);
        uint8_t savedBrightness = EEPROM.read(EEPROM_ADDR_BRIGHTNESS);
        uint8_t savedEffectMode = EEPROM.read(EEPROM_ADDR_EFFECT_MODE);
        uint8_t savedTimeFormat = EEPROM.read(EEPROM_ADDR_TIME_FORMAT);
        uint8_t savedClockColor = EEPROM.read(EEPROM_ADDR_CLOCK_COLOR);
        
        // Validate ranges before applying
        if (isValidTextSize(savedTextSize)) {
            textSize = savedTextSize;
        }
        
        if (isValidBrightnessIndex(savedBrightness)) {
            brightnessIndex = savedBrightness;
        }
        
        if (isValidEffectMode(savedEffectMode)) {
            effectMode = (EffectMode)savedEffectMode;
        }
        
        if (isValidClockColorMode(savedClockColor)) {
            clockColorMode = (ClockColorMode)savedClockColor;
        }
        
        use24HourFormat = (savedTimeFormat == 1);
        
        Serial.println("Settings loaded from EEPROM");
        Serial.print("Text Size: "); Serial.println(textSize);
        Serial.print("Brightness: "); Serial.println(brightnessIndex + 1);
        Serial.print("Effect Mode: "); Serial.println((int)effectMode);
        Serial.print("Time Format: "); Serial.println(use24HourFormat ? "24H" : "12H");
        Serial.print("Clock Color: "); Serial.println((int)clockColorMode);
    } else {
        Serial.println("No valid EEPROM data found, using defaults");
        // Save default settings to EEPROM for next time
        saveSettings();
    }
}

bool SettingsManager::isValidTextSize(int size) const {
    return (size >= TEXT_SIZE_MIN && size <= TEXT_SIZE_MAX);
}

bool SettingsManager::isValidBrightnessIndex(int index) const {
    return (index >= 0 && index < BRIGHTNESS_LEVELS);
}

bool SettingsManager::isValidEffectMode(int mode) const {
    return (mode >= 0 && mode <= EFFECT_OFF);
}

bool SettingsManager::isValidClockColorMode(int mode) const {
    return (mode >= CLOCK_WHITE && mode <= CLOCK_RAINBOW);
}
