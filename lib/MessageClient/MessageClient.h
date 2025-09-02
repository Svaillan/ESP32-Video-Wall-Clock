#ifndef MESSAGE_CLIENT_H
#define MESSAGE_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>

#include <WebServer.h>

#include "MatrixDisplayManager.h"
#include "SettingsManager.h"

// Include message API credentials
#include "../../credentials/message_config.h"

struct MessageItem {
    String id;
    String text;
    String priority;
};

class MessageClient {
   public:
    MessageClient(SettingsManager* settings, MatrixDisplayManager* display);
    void begin();
    void loop();

    // HTTP server handler for incoming posts
    void handlePostMessages();
    void handleStatus();

   private:
    SettingsManager* settings;
    MatrixDisplayManager* display;
    unsigned long lastPoll = 0;
    unsigned long pollIntervalMs = 60000;  // default 60s
    // Web server
    WebServer* webServer = nullptr;
    bool serverStarted = false;

    // Incoming-body pending queue to avoid blocking the HTTP connection while processing
    static const int PENDING_QUEUE_SIZE = 6;
    String pendingQueue[PENDING_QUEUE_SIZE];
    int pqHead = 0;
    int pqTail = 0;
    int pqCount = 0;

    // Rate limiting
    static const unsigned long MIN_MESSAGE_INTERVAL = 500;    // 0.5 seconds
    static const int MAX_MESSAGE_LENGTH = 500;                // 500 characters
    static const unsigned long LOW_MEMORY_THRESHOLD = 50000;  // 50KB
    unsigned long lastMessageTime = 0;

    // Helpers for pending queue
    bool enqueuePending(const String& body);
    bool dequeuePending(String& out);

    // Authentication helper
    bool checkAuthentication();

    void pollServer();
    bool processJson(const String& json);
};

#endif
