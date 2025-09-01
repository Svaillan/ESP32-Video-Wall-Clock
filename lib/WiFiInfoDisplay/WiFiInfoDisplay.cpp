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

    // Calculate vertical centering for text block
    // 3 lines of text (8px each) + 2 gaps (1px each) = 26px total
    // Center in 32px display: (32 - 26) / 2 = 3px offset
    const int TEXT_HEIGHT = 8;
    const int TEXT_SPACING = 1;
    const int START_Y = 3;

    // Draw connection status
    display->drawCenteredText("WiFi Connected", 1, display->applyBrightness(0x07E0),
                              START_Y);  // Green

    // Draw SSID (scrolling if too long)
    int ssidY = START_Y + TEXT_HEIGHT + TEXT_SPACING;
    if (ssid.length() > 16) {
        scrollText(ssid.c_str(), ssidY, display->applyBrightness(0xFFFF));  // White
    } else {
        display->drawCenteredText(ssid.c_str(), 1, display->applyBrightness(0xFFFF), ssidY);
    }

    // Draw IP address
    int ipY = ssidY + TEXT_HEIGHT + TEXT_SPACING;
    display->drawCenteredText(ip.c_str(), 1, display->applyBrightness(0x07FF), ipY);  // Cyan

    // Draw signal strength bar (keep this)
    drawSignalStrength(rssi);
}

void WiFiInfoDisplay::drawDisconnectedStatus() {
    display->fillScreen(0);

    // Calculate vertical centering for 2 lines of text
    // 2 lines of text (8px each) + 1 gap (1px) = 17px total
    // Center in 32px display: (32 - 17) / 2 = 7.5px ≈ 8px offset
    const int TEXT_HEIGHT = 8;
    const int TEXT_SPACING = 1;
    const int START_Y = 8;

    display->drawCenteredText("Disconnected", 1, display->applyBrightness(0xF800), START_Y);
    display->drawCenteredText("Enable in Menu", 1, display->applyBrightness(0x8410),
                              START_Y + TEXT_HEIGHT + TEXT_SPACING);
}

void WiFiInfoDisplay::drawNotConfiguredStatus() {
    display->fillScreen(0);

    // Calculate vertical centering for 2 lines of text
    // 2 lines of text (8px each) + 1 gap (1px) = 17px total
    // Center in 32px display: (32 - 17) / 2 = 7.5px ≈ 8px offset
    const int TEXT_HEIGHT = 8;
    const int TEXT_SPACING = 1;
    const int START_Y = 8;

    display->drawCenteredText("Not Configured", 1, display->applyBrightness(0xF800), START_Y);
    display->drawCenteredText("Set Up in Menu", 1, display->applyBrightness(0x8410),
                              START_Y + TEXT_HEIGHT + TEXT_SPACING);
}

void WiFiInfoDisplay::drawConnectingAnimation() {
    display->fillScreen(0);

    // Calculate vertical centering for 3 lines of text (connecting, dots, ssid)
    // 3 lines of text (8px each) + 2 gaps (1px each) = 26px total
    // Center in 32px display: (32 - 26) / 2 = 3px offset
    const int TEXT_HEIGHT = 8;
    const int TEXT_SPACING = 1;
    const int START_Y = 3;

    // Draw pulsing "Connecting..." text
    uint16_t color = display->applyBrightness(0xFFE0);  // Yellow
    display->drawCenteredText("Connecting...", 1, color, START_Y);

    // Draw animated dots
    int dotCount = (animationFrame / 5) % 4;
    String dots = "";
    for (int i = 0; i < dotCount; i++) {
        dots += ".";
    }
    int dotsY = START_Y + TEXT_HEIGHT + TEXT_SPACING;
    display->drawCenteredText(dots.c_str(), 1, color, dotsY);

    // Draw SSID being connected to
    String ssid = "SSID: " + String(settings->getWiFiSSID());
    int ssidY = dotsY + TEXT_HEIGHT + TEXT_SPACING;
    display->drawCenteredText(ssid.c_str(), 1, display->applyBrightness(0x8410), ssidY);
}

void WiFiInfoDisplay::drawSignalStrength(int rssi) {
    int strength = getSignalStrength(rssi);
    uint16_t color = getSignalColor(strength);

    // Position signal bars in bottom right corner
    // Each bar is 2px wide, spaced 3px apart: total width = 4*3 + 2 = 14px
    // Position so the rightmost bar ends 2px from right edge
    int startX = MATRIX_WIDTH - 16;  // 128 - 16 = 112, rightmost bar ends at x=126 (2px from edge)
    int startY = 30;                 // Below the text with proper spacing

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
