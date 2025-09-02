#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>

// EEPROM Settings
#define EEPROM_SIZE 128                       // Increased size for WiFi settings
#define EEPROM_MAGIC 0x42                     // Magic number to verify valid data
#define EEPROM_ADDR_MAGIC 0                   // Address for magic number
#define EEPROM_ADDR_TEXT_SIZE 1               // Address for text size
#define EEPROM_ADDR_BRIGHTNESS 2              // Address for brightness index
#define EEPROM_ADDR_EFFECT_MODE 3             // Address for effect mode
#define EEPROM_ADDR_TIME_FORMAT 4             // Address for time format (12/24 hour)
#define EEPROM_ADDR_CLOCK_COLOR 5             // Address for clock color mode
#define EEPROM_ADDR_WIFI_ENABLED 6            // Address for WiFi enabled flag
#define EEPROM_ADDR_WIFI_SSID 7               // Address for WiFi SSID (32 bytes)
#define EEPROM_ADDR_WIFI_PASSWORD 39          // Address for WiFi password (64 bytes)
#define EEPROM_ADDR_TIMEZONE_INDEX 103        // Address for timezone index (1 byte)
#define EEPROM_ADDR_MESSAGE_SCROLL_SPEED 104  // Address for message scroll speed (1 byte)

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

enum MessageScrollSpeed {
    MSG_SCROLL_SLOW,    // 51ms (current normal)
    MSG_SCROLL_MEDIUM,  // 34ms (current high priority)
    MSG_SCROLL_FAST     // 25ms (new fastest)
};

class SettingsManager {
   public:
    // Constructor
    SettingsManager();

    // Initialization
    void begin();

    // Settings getters
    int getTextSize() const {
        return textSize;
    }
    int getBrightnessIndex() const {
        return brightnessIndex;
    }
    EffectMode getEffectMode() const {
        return effectMode;
    }
    bool getUse24HourFormat() const {
        return use24HourFormat;
    }
    ClockColorMode getClockColorMode() const {
        return clockColorMode;
    }
    int getTimezoneIndex() const {
        return timezoneIndex;
    }
    MessageScrollSpeed getMessageScrollSpeed() const {
        return messageScrollSpeed;
    }

    // WiFi settings getters
    bool isWiFiEnabled() const {
        return wifiEnabled;
    }
    const char* getWiFiSSID() const {
        return wifiSSID;
    }
    const char* getWiFiPassword() const {
        return wifiPassword;
    }

    // OTA settings getters
    const char* getOTAPassword() const {
        return otaPassword;
    }

    // Settings setters
    void setTextSize(int size);
    void setBrightnessIndex(int index);
    void setEffectMode(EffectMode mode);
    void setUse24HourFormat(bool format);
    void setClockColorMode(ClockColorMode mode);
    void setTimezoneIndex(int index);
    void setMessageScrollSpeed(MessageScrollSpeed speed);

    // WiFi settings setters
    void setWiFiEnabled(bool enabled);
    void setWiFiCredentials(const char* ssid, const char* password);

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
    int timezoneIndex;
    MessageScrollSpeed messageScrollSpeed;

    // WiFi settings
    bool wifiEnabled;
    char wifiSSID[32];
    char wifiPassword[64];

    // OTA settings
    char otaPassword[32];  // Increased buffer size for static OTA password

    // Validation functions
    bool isValidTextSize(int size) const;
    bool isValidBrightnessIndex(int index) const;
    bool isValidEffectMode(int mode) const;
    bool isValidClockColorMode(int mode) const;
    bool isValidMessageScrollSpeed(int speed) const;
};

#endif  // SETTINGS_MANAGER_H
