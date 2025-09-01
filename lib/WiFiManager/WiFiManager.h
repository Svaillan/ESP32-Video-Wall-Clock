#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>

#include <ArduinoOTA.h>
#include <ESPmDNS.h>

class WiFiManager {
   public:
    explicit WiFiManager(class SettingsManager* settings);
    void begin(const char* ssid, const char* password);
    void reconnectWithNewCredentials(const char* ssid, const char* password);
    void disconnect();
    void handleOTA();
    bool isConnected();
    bool isOTAInProgress() {
        return otaInProgress;
    }
    String getIPAddress();
    void setupOTA(const char* hostname, class MatrixDisplayManager* display = nullptr);
    void displayStatus(class MatrixDisplayManager* display);
    String getOTAPassword();

   private:
    bool wifiConnected;
    bool otaInProgress;
    unsigned long lastConnectionAttempt;
    unsigned int otaProgress;
    class SettingsManager* settingsManager;
    class MatrixDisplayManager* displayManager;
    const unsigned long reconnectInterval = 30000;  // 30 seconds

    void connectToWiFi(const char* ssid, const char* password);
    void onOTAStart();
    void onOTAEnd();
    void onOTAProgress(unsigned int progress, unsigned int total);
    void onOTAError(ota_error_t error);
};

#endif
