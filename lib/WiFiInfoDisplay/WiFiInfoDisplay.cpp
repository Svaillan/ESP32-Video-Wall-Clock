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
    } else if (settings->isWiFiEnabled()) {
        drawConnectingAnimation();
    } else {
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

    // Draw red X animation
    uint16_t color = display->applyBrightness(0xF800);  // Red

    // Draw X pattern
    for (int i = 0; i < 16; i++) {
        display->drawPixel(56 + i, 8 + i, color);
        display->drawPixel(56 + i, 23 - i, color);
        display->drawPixel(72 - i, 8 + i, color);
        display->drawPixel(72 - i, 23 - i, color);
    }

    display->drawCenteredText("WiFi Disabled", 1, display->applyBrightness(0xF800), 2);
    display->drawCenteredText("Enable in Menu", 1, display->applyBrightness(0x8410), 26);
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

    // Draw spinning WiFi waves
    drawWiFiWaves(64, 20, 3, animationFrame * 2);

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

void WiFiInfoDisplay::drawWiFiWaves(int centerX, int centerY, int strength, uint32_t frame) {
    uint16_t color = getSignalColor(strength);

    // Draw concentric arcs to represent WiFi waves
    for (int wave = 0; wave < strength; wave++) {
        int radius = 3 + wave * 3;

        // Draw arc points
        for (int angle = 45; angle <= 135; angle += 5) {
            float rad = angle * PI / 180.0;
            int x = centerX + radius * cos(rad);
            int y = centerY - radius * sin(rad);

            if (x >= 0 && x < MATRIX_WIDTH && y >= 0 && y < MATRIX_HEIGHT) {
                display->drawPixel(x, y, color);
            }
        }
    }

    // Draw center dot
    display->drawPixel(centerX, centerY, display->applyBrightness(0xFFFF));
    display->drawPixel(centerX - 1, centerY, display->applyBrightness(0xFFFF));
    display->drawPixel(centerX + 1, centerY, display->applyBrightness(0xFFFF));
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
