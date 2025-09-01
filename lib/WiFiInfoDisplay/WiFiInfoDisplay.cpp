#include "WiFiInfoDisplay.h"

#include <WiFi.h>

WiFiInfoDisplay::WiFiInfoDisplay(MatrixDisplayManager* displayManager, WiFiManager* wifiManager,
                                 SettingsManager* settingsManager) {
    display = displayManager;
    wifi = wifiManager;
    settings = settingsManager;
    lastUpdate = 0;
    animationFrame = 0;
    animationDirection = true;
}

void WiFiInfoDisplay::begin() {
    // Nothing specific to initialize
}

void WiFiInfoDisplay::updateDisplay() {
    uint32_t currentTime = millis();

    // Update animation frame every 100ms
    if (currentTime - lastUpdate > 100) {
        if (animationDirection) {
            animationFrame++;
            if (animationFrame > 20)
                animationDirection = false;
        } else {
            animationFrame--;
            if (animationFrame == 0)
                animationDirection = true;
        }
        lastUpdate = currentTime;
    }

    if (wifi->isConnected()) {
        drawConnectedStatus();
    } else if (strlen(settings->getWiFiSSID()) == 0) {
        // No WiFi credentials configured
        drawNotConfiguredStatus();
    } else if (settings->isWiFiEnabled()) {
        // Credentials exist and WiFi enabled but not connected
        drawConnectingAnimation();
    } else {
        // Credentials exist but WiFi disabled
        drawDisconnectedStatus();
    }
}

void WiFiInfoDisplay::drawConnectedStatus() {
    // Clear display
    display->fillScreen(0);

    // Get network info
    String ssid = WiFi.SSID();
    String ip = wifi->getIPAddress();
    int rssi = WiFi.RSSI();

    // Draw connection status
    display->drawCenteredText("WiFi Connected", 1, display->applyBrightness(0x07E0), 2);  // Green

    // Draw SSID (scrolling if too long)
    if (ssid.length() > 16) {
        scrollText(ssid.c_str(), 10, display->applyBrightness(0xFFFF));  // White
    } else {
        display->drawCenteredText(ssid.c_str(), 1, display->applyBrightness(0xFFFF), 10);
    }

    // Draw IP address
    display->drawCenteredText(ip.c_str(), 1, display->applyBrightness(0x07FF), 18);  // Cyan

    // Draw signal strength bar (keep this)
    drawSignalStrength(rssi);
}

void WiFiInfoDisplay::drawDisconnectedStatus() {
    display->fillScreen(0);

    // No symbol - just text
    display->drawCenteredText("Disconnected", 1, display->applyBrightness(0xF800), 2);
    display->drawCenteredText("Enable in Menu", 1, display->applyBrightness(0x8410), 20);
}

void WiFiInfoDisplay::drawNotConfiguredStatus() {
    display->fillScreen(0);

    // No symbol - just text
    display->drawCenteredText("Not Configured", 1, display->applyBrightness(0xF800), 2);
    display->drawCenteredText("Set Up in Menu", 1, display->applyBrightness(0x8410), 20);
}

void WiFiInfoDisplay::drawConnectingAnimation() {
    display->fillScreen(0);

    // Draw pulsing "Connecting..." text
    uint16_t color = display->applyBrightness(0xFFE0);  // Yellow

    display->drawCenteredText("Connecting...", 1, color, 2);

    // Draw animated dots
    int dotCount = (animationFrame / 5) % 4;
    String dots = "";
    for (int i = 0; i < dotCount; i++) {
        dots += ".";
    }
    display->drawCenteredText(dots.c_str(), 1, color, 10);

    // No WiFi wave symbols - just text
    String ssid = "SSID: " + String(settings->getWiFiSSID());
    display->drawCenteredText(ssid.c_str(), 1, display->applyBrightness(0x8410), 26);
}

void WiFiInfoDisplay::drawSignalStrength(int rssi) {
    int strength = getSignalStrength(rssi);
    uint16_t color = getSignalColor(strength);

    // Draw signal bars at bottom right
    int startX = MATRIX_WIDTH - 20;
    int startY = MATRIX_HEIGHT - 8;

    for (int i = 0; i < 5; i++) {
        if (i < strength) {
            // Draw filled bar
            for (int y = 0; y < (i + 1) * 2; y++) {
                for (int x = 0; x < 2; x++) {
                    display->drawPixel(startX + i * 3 + x, startY - y, color);
                }
            }
        } else {
            // Draw empty bar outline
            display->drawPixel(startX + i * 3, startY, display->applyBrightness(0x4208));
            display->drawPixel(startX + i * 3 + 1, startY, display->applyBrightness(0x4208));
        }
    }

    // dBm text removed - just show the signal bars
}

void WiFiInfoDisplay::scrollText(const char* text, int y, uint16_t color) {
    // Draw centered text
    display->drawCenteredText(text, 1, color, y);
}

int WiFiInfoDisplay::getSignalStrength(int rssi) {
    if (rssi >= -50)
        return 5;  // Excellent
    else if (rssi >= -60)
        return 4;  // Good
    else if (rssi >= -70)
        return 3;  // Fair
    else if (rssi >= -80)
        return 2;  // Weak
    else if (rssi >= -90)
        return 1;  // Very weak
    else
        return 0;  // No signal
}

uint16_t WiFiInfoDisplay::getSignalColor(int strength) {
    switch (strength) {
        case 5:
        case 4:
            return 0x07E0;  // Green
        case 3:
            return 0xFFE0;  // Yellow
        case 2:
            return 0xFD20;  // Orange
        case 1:
            return 0xF800;  // Red
        default:
            return 0x4208;  // Gray
    }
}
