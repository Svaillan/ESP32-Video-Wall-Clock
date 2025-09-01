#ifndef WIFI_INFO_DISPLAY_H
#define WIFI_INFO_DISPLAY_H

#include <Arduino.h>

#include "MatrixDisplayManager.h"
#include "SettingsManager.h"
#include "WiFiManager.h"

class WiFiInfoDisplay {
   public:
    WiFiInfoDisplay(MatrixDisplayManager* displayManager, WiFiManager* wifiManager,
                    SettingsManager* settingsManager);

    void begin();
    void updateDisplay();

   private:
    MatrixDisplayManager* display;
    WiFiManager* wifi;
    SettingsManager* settings;

    // Animation state
    uint32_t lastUpdate;
    uint32_t animationFrame;
    bool animationDirection;

    // Display methods
    void drawConnectedStatus();
    void drawDisconnectedStatus();
    void drawConnectingAnimation();
    void drawSignalStrength(int rssi);
    void drawWiFiWaves(int centerX, int centerY, int strength, uint32_t frame);
    void scrollText(const char* text, int y, uint16_t color);

    // Utility methods
    int getSignalStrength(int rssi);
    uint16_t getSignalColor(int strength);
};

#endif
