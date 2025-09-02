#include "SettingsManager.h"

// Try to include local OTA config, use default if not available
#if __has_include("../../credentials/ota_config.h")
#include "../../credentials/ota_config.h"
#endif

// Fallback password if no config file
#ifndef OTA_STATIC_PASSWORD
#define OTA_STATIC_PASSWORD "defaultOTA"
#endif

SettingsManager::SettingsManager() {
    // Initialize with default values
    textSize = 2;
    brightnessIndex = 9;  // Default to brightness level 10 (100%)
    effectMode = EFFECT_CONFETTI;
    use24HourFormat = true;  // Default to 24-hour format
    clockColorMode = CLOCK_WHITE;
    timezoneIndex = 0;                       // Default to Arizona (index 0)
    messageScrollSpeed = MSG_SCROLL_MEDIUM;  // Default to medium speed

    // WiFi defaults
    wifiEnabled = false;
    strcpy(wifiSSID, "");
    strcpy(wifiPassword, "");

    // OTA defaults - always use static password
    strcpy(otaPassword, OTA_STATIC_PASSWORD);
}

void SettingsManager::begin() {
    EEPROM.begin(EEPROM_SIZE);
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

void SettingsManager::setTimezoneIndex(int index) {
    timezoneIndex = index;
}

void SettingsManager::setMessageScrollSpeed(MessageScrollSpeed speed) {
    if (isValidMessageScrollSpeed((int)speed)) {
        messageScrollSpeed = speed;
    }
}

void SettingsManager::setWiFiEnabled(bool enabled) {
    wifiEnabled = enabled;
}

void SettingsManager::setWiFiCredentials(const char* ssid, const char* password) {
    strncpy(wifiSSID, ssid, sizeof(wifiSSID) - 1);
    wifiSSID[sizeof(wifiSSID) - 1] = '\0';

    strncpy(wifiPassword, password, sizeof(wifiPassword) - 1);
    wifiPassword[sizeof(wifiPassword) - 1] = '\0';

    wifiEnabled = (strlen(ssid) > 0);
}

void SettingsManager::saveSettings() {
    EEPROM.write(EEPROM_ADDR_MAGIC, EEPROM_MAGIC);
    EEPROM.write(EEPROM_ADDR_TEXT_SIZE, textSize);
    EEPROM.write(EEPROM_ADDR_BRIGHTNESS, brightnessIndex);
    EEPROM.write(EEPROM_ADDR_EFFECT_MODE, (uint8_t)effectMode);
    EEPROM.write(EEPROM_ADDR_TIME_FORMAT, use24HourFormat ? 1 : 0);
    EEPROM.write(EEPROM_ADDR_CLOCK_COLOR, (uint8_t)clockColorMode);
    EEPROM.write(EEPROM_ADDR_TIMEZONE_INDEX, (uint8_t)timezoneIndex);
    EEPROM.write(EEPROM_ADDR_MESSAGE_SCROLL_SPEED, (uint8_t)messageScrollSpeed);

    // Save WiFi settings
    EEPROM.write(EEPROM_ADDR_WIFI_ENABLED, wifiEnabled ? 1 : 0);

    // Save SSID
    for (int i = 0; i < 32; i++) {
        EEPROM.write(EEPROM_ADDR_WIFI_SSID + i, wifiSSID[i]);
    }

    // Save password
    for (int i = 0; i < 64; i++) {
        EEPROM.write(EEPROM_ADDR_WIFI_PASSWORD + i, wifiPassword[i]);
    }

    EEPROM.commit();

    Serial.println("Settings saved to EEPROM");
    Serial.print("Text Size: ");
    Serial.println(textSize);
    Serial.print("Brightness: ");
    Serial.println(brightnessIndex + 1);
    Serial.print("Effect Mode: ");
    Serial.println((int)effectMode);
    Serial.print("Time Format: ");
    Serial.println(use24HourFormat ? "24H" : "12H");
    Serial.print("Clock Color: ");
    Serial.println((int)clockColorMode);
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
        uint8_t savedTimezoneIndex = EEPROM.read(EEPROM_ADDR_TIMEZONE_INDEX);
        uint8_t savedMessageScrollSpeed = EEPROM.read(EEPROM_ADDR_MESSAGE_SCROLL_SPEED);

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

        // Validate and load timezone index (0-23 for 24 timezones)
        if (savedTimezoneIndex < 24) {
            timezoneIndex = savedTimezoneIndex;
        }

        if (isValidMessageScrollSpeed(savedMessageScrollSpeed)) {
            messageScrollSpeed = (MessageScrollSpeed)savedMessageScrollSpeed;
        }

        // Load WiFi settings
        wifiEnabled = EEPROM.read(EEPROM_ADDR_WIFI_ENABLED) == 1;

        // Load SSID
        for (int i = 0; i < 32; i++) {
            wifiSSID[i] = EEPROM.read(EEPROM_ADDR_WIFI_SSID + i);
        }
        wifiSSID[31] = '\0';  // Ensure null termination

        // Load password
        for (int i = 0; i < 64; i++) {
            wifiPassword[i] = EEPROM.read(EEPROM_ADDR_WIFI_PASSWORD + i);
        }
        wifiPassword[63] = '\0';  // Ensure null termination

        // Initialize OTA password - always use static password
        strcpy(otaPassword, OTA_STATIC_PASSWORD);

        use24HourFormat = (savedTimeFormat == 1);
    } else {
        // Use static OTA password for new device
        strcpy(otaPassword, OTA_STATIC_PASSWORD);
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

bool SettingsManager::isValidMessageScrollSpeed(int speed) const {
    return (speed >= MSG_SCROLL_SLOW && speed <= MSG_SCROLL_FAST);
}
