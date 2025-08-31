#ifndef CLOCK_DISPLAY_H
#define CLOCK_DISPLAY_H

#include <Arduino.h>
#include <RTClib.h>
#include "MatrixDisplayManager.h"
#include "SettingsManager.h"

class ClockDisplay {
public:
    // Constructor
    ClockDisplay(MatrixDisplayManager* display, SettingsManager* settings, RTC_DS3231* rtc);
    
    // Initialization
    void begin();
    
    // Main clock display method
    void displayTime();
    
    // Utility methods for text area bounds (for effects integration)
    void getTimeDisplayBounds(int &x1, int &y1, int &x2, int &y2);
    void getAMPMDisplayBounds(int &x1, int &y1, int &x2, int &y2);
    bool isInTextArea(int x, int y);
    
private:
    MatrixDisplayManager* display;
    SettingsManager* settings;
    RTC_DS3231* rtc;
    
    // Helper methods
    String formatTime(DateTime now);
    void displayAMPM(DateTime now);
};

#endif // CLOCK_DISPLAY_H
