#ifndef APP_STATE_H
#define APP_STATE_H

// Application states
enum AppState {
    SHOW_TIME,
    SHOW_TIME_WITH_DATE,  // New state for clock mode with date
    SHOW_WIFI_INFO,
    MENU,
    EDIT_TEXT_SIZE,
    EDIT_BRIGHTNESS,
    EDIT_TIME_FORMAT,
    EDIT_CLOCK_COLOR,
    EDIT_EFFECTS,
    EDIT_TIMEZONE,
    TIME_SET,
    SYNC_NTP,   // New state for NTP sync
    WIFI_MENU,  // New state for WiFi menu
    OTA_MENU    // New state for OTA menu
};

#endif  // APP_STATE_H
