#include "ClockDisplay.h"

ClockDisplay::ClockDisplay(MatrixDisplayManager* display, SettingsManager* settings,
                           RTC_DS3231* rtc)
    : display(display), settings(settings), rtc(rtc) {}

void ClockDisplay::begin() {
    Serial.println("Clock Display initialized");
}

void ClockDisplay::displayTime() {
    if (!rtc || !display) {
        return;
    }

    DateTime now = rtc->now();
    String timeString = formatTime(now);

    // Draw text background to ensure readability over effects
    display->drawTextBackground();

    // Use drawTightClock for proper centering and text size
    display->drawTightClock(timeString.c_str(), settings->getTextSize(), display->getClockColor());

    // Handle AM/PM display for 12-hour format
    if (!settings->getUse24HourFormat()) {
        displayAMPM(now);
    }
}

String ClockDisplay::formatTime(DateTime now) {
    char timeStr[12];

    if (settings->getUse24HourFormat()) {
        // 24-hour format: HH:MM:SS
        sprintf(timeStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
    } else {
        // 12-hour format: HH:MM:SS (AM/PM displayed separately)
        int hour = now.hour();
        if (hour == 0)
            hour = 12;  // Midnight is 12 AM
        if (hour > 12)
            hour -= 12;  // Convert PM hours

        sprintf(timeStr, "%02d:%02d:%02d", hour, now.minute(), now.second());
    }

    return String(timeStr);
}

void ClockDisplay::displayAMPM(DateTime now) {
    int hour = now.hour();
    bool isPM = (hour >= 12);
    String ampmStr = "";

    // Use short form (A/P) for text size 3 to avoid corner collision
    int textSize = settings->getTextSize();
    if (textSize == 3) {
        ampmStr = isPM ? "P" : "A";
    } else {
        ampmStr = isPM ? "PM" : "AM";
    }

    // Position in bottom right corner
    display->setTextSize(1);

    int ampmX = 128 - (ampmStr.length() * 6) - 1;
    int ampmY = 32 - 8;

    uint16_t color = display->getClockColor();
    display->setCursor(ampmX, ampmY);
    display->setTextColor(color);
    display->print(ampmStr.c_str());
}

// Utility methods for effects integration
void ClockDisplay::getTimeDisplayBounds(int& x1, int& y1, int& x2, int& y2) {
    // Simple bounds for the time display area
    x1 = 0;
    y1 = 6;
    x2 = 127;
    y2 = 25;
}

void ClockDisplay::getAMPMDisplayBounds(int& x1, int& y1, int& x2, int& y2) {
    if (settings->getUse24HourFormat()) {
        // No AM/PM in 24-hour format
        x1 = y1 = x2 = y2 = 0;
        return;
    }

    // AM/PM area in bottom right
    x1 = 100;
    y1 = 6;
    x2 = 127;
    y2 = 25;
}

bool ClockDisplay::isInTextArea(int x, int y) {
    int x1, y1, x2, y2;
    getTimeDisplayBounds(x1, y1, x2, y2);
    return (x >= x1 && x <= x2 && y >= y1 && y <= y2);
}
