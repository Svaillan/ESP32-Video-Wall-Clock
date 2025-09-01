#ifndef CLOCK_DISPLAY_H
#define CLOCK_DISPLAY_H

#include <Arduino.h>

#include <RTClib.h>

#include "MatrixDisplayManager.h"
#include "SettingsManager.h"
#include "TimeManager.h"

class ClockDisplay {
   public:
    // Constructor
    ClockDisplay(MatrixDisplayManager* display, SettingsManager* settings, RTC_DS3231* rtc,
                 TimeManager* timeManager);

    // Initialization
    void begin();

    // Main clock display method
    void displayTime();
    void displayTimeWithDate();  // New method for the date/time mode

    // Utility methods for text area bounds (for effects integration)
    void getTimeDisplayBounds(int& x1, int& y1, int& x2, int& y2);
    void getAMPMDisplayBounds(int& x1, int& y1, int& x2, int& y2);
    bool isInTextArea(int x, int y);

   private:
    MatrixDisplayManager* display;
    SettingsManager* settings;
    RTC_DS3231* rtc;
    TimeManager* timeManager;

    // Helper methods
    String formatTime(DateTime now);
    String formatTimeWithAMPM(DateTime now);  // New method for time with AM/PM included
    String formatDateWithDay(DateTime now);   // New method for date with 3-char day code
    void displayAMPM(DateTime now);
};

#endif  // CLOCK_DISPLAY_H
