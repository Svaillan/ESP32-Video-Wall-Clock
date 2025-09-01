#include "WiFiManager.h"

#include "MatrixDisplayManager.h"
#include "SettingsManager.h"

WiFiManager::WiFiManager(SettingsManager* settings)
    : wifiConnected(false),
      otaInProgress(false),
      lastConnectionAttempt(0),
      otaProgress(0),
      displayManager(nullptr) {
    settingsManager = settings;
}

void WiFiManager::begin(const char* ssid, const char* password) {
    connectToWiFi(ssid, password);
}

void WiFiManager::reconnectWithNewCredentials(const char* ssid, const char* password) {
    // Disconnect from current network if connected
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect();
        delay(100);
    }

    // Connect with new credentials
    connectToWiFi(ssid, password);

    // If we successfully connected and OTA wasn't set up yet, set it up now
    if (wifiConnected && !ArduinoOTA.getHostname().length()) {
        setupOTA("matrix-clock", displayManager);
    }
}

void WiFiManager::disconnect() {
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect();
        wifiConnected = false;
    }
}

void WiFiManager::connectToWiFi(const char* ssid, const char* password) {
    WiFi.mode(WIFI_STA);

    if (password == nullptr || strlen(password) == 0) {
        WiFi.begin(ssid);
    } else {
        WiFi.begin(ssid, password);
    }

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        MDNS.begin("matrix-clock");
    } else {
        wifiConnected = false;
    }
}

void WiFiManager::setupOTA(const char* hostname, MatrixDisplayManager* display) {
    ArduinoOTA.setHostname(hostname);
    ArduinoOTA.setPassword(settingsManager->getOTAPassword());

    // Store the display manager for screen blanking during OTA
    displayManager = display;

    ArduinoOTA.onStart([this]() { onOTAStart(); });

    ArduinoOTA.onEnd([this]() { onOTAEnd(); });

    ArduinoOTA.onProgress(
        [this](unsigned int progress, unsigned int total) { onOTAProgress(progress, total); });

    ArduinoOTA.onError([this](ota_error_t error) { onOTAError(error); });

    ArduinoOTA.begin();
}

void WiFiManager::handleOTA() {
    if (WiFi.status() == WL_CONNECTED) {
        ArduinoOTA.handle();
        wifiConnected = true;
    } else {
        wifiConnected = false;

        if (millis() - lastConnectionAttempt > reconnectInterval) {
            lastConnectionAttempt = millis();
        }
    }
}

bool WiFiManager::isConnected() {
    return wifiConnected && (WiFi.status() == WL_CONNECTED);
}

String WiFiManager::getIPAddress() {
    if (isConnected()) {
        return WiFi.localIP().toString();
    }
    return "Not connected";
}

void WiFiManager::displayStatus(MatrixDisplayManager* display) {
    if (otaInProgress) {
        char progressStr[16];
        sprintf(progressStr, "UPDATE %u%%", otaProgress);
        display->drawCenteredTextWithBox(
            progressStr, 1, display->applyBrightness(0xFFE0));  // Yellow with brightness
    }
}

void WiFiManager::onOTAStart() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    otaInProgress = true;
    otaProgress = 0;

    if (displayManager != nullptr) {
        displayManager->clearScreen();
        displayManager->show();
    }
}

void WiFiManager::onOTAEnd() {
    otaInProgress = false;
    otaProgress = 100;
}

void WiFiManager::onOTAProgress(unsigned int progress, unsigned int total) {
    otaProgress = (progress / (total / 100));
}

void WiFiManager::onOTAError(ota_error_t error) {
    otaInProgress = false;
}

String WiFiManager::getOTAPassword() {
    const char* password = settingsManager->getOTAPassword();
    return String(password);
}
