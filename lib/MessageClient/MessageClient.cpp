#include "MessageClient.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WebServer.h>

MessageClient::MessageClient(SettingsManager* settings, MatrixDisplayManager* display)
    : settings(settings), display(display) {}

void MessageClient::begin() {
    // read poll interval from settings later if added; keep default for now
    // create web server
    webServer = new WebServer(80);
    webServer->on("/messages", HTTP_POST, [this]() { handlePostMessages(); });
    webServer->on("/status", HTTP_GET, [this]() { handleStatus(); });
    // do NOT call begin() here; start after WiFi is connected in loop
}

void MessageClient::loop() {
    // Only poll if WiFi is connected
    if (!WiFi.isConnected())
        return;

    // Start web server once after WiFi connects
    if (!serverStarted) {
        webServer->begin();
        serverStarted = true;
        Serial.println("MessageClient: HTTP server started on port 80");
    }

    webServer->handleClient();

    unsigned long now = millis();

    // Periodic memory monitoring (every 30 seconds)
    static unsigned long lastMemoryCheck = 0;
    if (now - lastMemoryCheck > 30000) {
        uint32_t freeHeap = ESP.getFreeHeap();
        if (freeHeap < LOW_MEMORY_THRESHOLD) {
            Serial.print("MessageClient: Low memory warning - ");
            Serial.print(freeHeap);
            Serial.println(" bytes free");

            // Could implement additional cleanup here if needed
            // For now, just log the warning
        }
        lastMemoryCheck = now;
    }

    // Process one pending request per loop pass to keep work spread out
    String pending;
    if (dequeuePending(pending)) {
        // process but don't block the HTTP response
        bool ok = processJson(pending);
        (void)ok;  // keep for potential future logging
    }

    if (now - lastPoll < pollIntervalMs)
        return;
    lastPoll = now;

    pollServer();
}

void MessageClient::pollServer() {
    IPAddress ip = WiFi.localIP();
    String serverUrl =
        String("http://") + ip.toString() + ":3000/messages";  // uses device local IP

    HTTPClient http;
    http.begin(serverUrl);
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        processJson(payload);
    } else {
        Serial.print("MessageClient: HTTP GET failed, code: ");
        Serial.println(httpCode);
    }
    http.end();
}

void MessageClient::handlePostMessages() {
    if (!webServer)
        return;

    // Check authentication first
    if (!checkAuthentication()) {
        webServer->send(401, "application/json", "{\"error\":\"unauthorized\"}");
        return;
    }

    // Check memory availability
    if (ESP.getFreeHeap() < LOW_MEMORY_THRESHOLD) {
        Serial.println("MessageClient: Low memory, rejecting message");
        webServer->send(507, "application/json", "{\"error\":\"insufficient memory\"}");
        return;
    }

    // Rate limiting check
    unsigned long now = millis();
    if (now - lastMessageTime < MIN_MESSAGE_INTERVAL) {
        webServer->send(429, "application/json", "{\"error\":\"rate limited\"}");
        return;
    }

    // Read body (non-blocking read is handled by WebServer; arg("plain") is safe)
    String body = webServer->arg("plain");
    if (body.length() == 0) {
        webServer->send(400, "application/json", "{\"error\":\"empty body\"}");
        return;
    }

    // Check message length
    if (body.length() > MAX_MESSAGE_LENGTH) {
        webServer->send(413, "application/json", "{\"error\":\"message too long\"}");
        return;
    }

    // Try to enqueue the raw body for async processing. If queue full, return 503.
    if (!enqueuePending(body)) {
        webServer->send(503, "application/json", "{\"error\":\"server busy\"}");
        return;
    }

    // Update rate limiting timestamp
    lastMessageTime = now;

    // Respond immediately to the client. Actual parsing/queuing to display occurs asynchronously in
    // loop().
    webServer->send(201, "application/json", "{\"status\":\"accepted\"}");
}

void MessageClient::handleStatus() {
    if (!webServer)
        return;

    // Check authentication for status endpoint too
    if (!checkAuthentication()) {
        webServer->send(401, "application/json", "{\"error\":\"unauthorized\"}");
        return;
    }

    IPAddress ip = WiFi.localIP();
    String payload;
    payload += "{\"ip\":\"" + ip.toString() + "\",";
    payload += "\"pending_queue\":" + String(pqCount) + ",";
    payload += "\"pending_capacity\":" + String(PENDING_QUEUE_SIZE) + ",";
    payload += "\"display_queue\":" + String(display->getQueueCount()) + ",";
    payload += "\"display_capacity\":" + String(8) + ",";
    payload += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    payload += "\"rate_limit_ms\":" + String(MIN_MESSAGE_INTERVAL) + ",";
    payload += "\"max_message_length\":" + String(MAX_MESSAGE_LENGTH) + ",";
    payload +=
        "\"auth_required\":" + String(strlen(MESSAGE_API_PASSWORD) > 0 ? "true" : "false") + "}";
    webServer->send(200, "application/json", payload);
}

bool MessageClient::processJson(const String& json) {
    // Try array first, then object. Use a conservative JSON document size.
    StaticJsonDocument<2048> doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.print("MessageClient: JSON parse error: ");
        Serial.println(err.c_str());
        return false;
    }

    if (doc.is<JsonArray>()) {
        for (JsonObject obj : doc.as<JsonArray>()) {
            const char* id = obj["id"] | "";
            const char* text = obj["text"] | "";
            const char* priority = obj["priority"] | "normal";

            if (strlen(text) == 0)
                continue;

            display->enqueueMessage(id, text, priority);
            Serial.print("MessageClient: queued message: ");
            Serial.println(text);
        }
        return true;
    }

    if (doc.is<JsonObject>()) {
        JsonObject obj = doc.as<JsonObject>();
        const char* id = obj["id"] | "";
        const char* text = obj["text"] | "";
        const char* priority = obj["priority"] | "normal";

        if (strlen(text) == 0)
            return false;

        display->enqueueMessage(id, text, priority);
        Serial.print("MessageClient: queued message: ");
        Serial.println(text);
        return true;
    }

    Serial.println("MessageClient: unexpected JSON type");
    return false;
}

bool MessageClient::enqueuePending(const String& body) {
    if (pqCount >= PENDING_QUEUE_SIZE)
        return false;
    pendingQueue[pqTail] = body;
    pqTail = (pqTail + 1) % PENDING_QUEUE_SIZE;
    pqCount++;
    return true;
}

bool MessageClient::dequeuePending(String& out) {
    if (pqCount == 0)
        return false;
    out = pendingQueue[pqHead];
    pqHead = (pqHead + 1) % PENDING_QUEUE_SIZE;
    pqCount--;
    return true;
}

bool MessageClient::checkAuthentication() {
    // If no password is configured, allow all requests
    if (strlen(MESSAGE_API_PASSWORD) == 0) {
        return true;
    }

    // Check for Authorization header
    if (!webServer->hasHeader("Authorization")) {
        return false;
    }

    String authHeader = webServer->header("Authorization");

    // Support both Bearer token and Basic auth formats
    String expectedBearer = "Bearer " + String(MESSAGE_API_PASSWORD);
    if (authHeader == expectedBearer) {
        return true;
    }

    // Also check for password in query parameter as fallback
    if (webServer->hasArg("password")) {
        String providedPassword = webServer->arg("password");
        if (providedPassword == String(MESSAGE_API_PASSWORD)) {
            return true;
        }
    }

    return false;
}
