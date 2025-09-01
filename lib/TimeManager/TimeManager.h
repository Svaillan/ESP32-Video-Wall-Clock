#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>

#include <RTClib.h>
#include <time.h>

class TimeManager {
   public:
    explicit TimeManager(RTC_DS3231* rtcInst, const char* ntpServer = "pool.ntp.org");
    void begin();
    void setTimezone(const char* tz);
    void setTimezoneAndUpdate(const char* tz);  // Set timezone and force time update
    void setTimezoneOffset(int utcOffsetHours, bool isDST = false, int dstOffsetHours = 0);
    bool isDSTActive(int month, int day, int utcOffsetHours);  // Simple DST calculation
    DateTime getLocalTime();  // Get current time with timezone offset applied
    bool syncTimeWithNTP(bool updateRTC = true);
    void periodicNTPSync(unsigned long intervalMs = 12UL * 60UL * 60UL * 1000UL);  // default 12h
    void updateRTCFromNTP();
    void updateRTCFromSystem();
    void updateSystemFromRTC();
    void setLastNTPSync(unsigned long ms);
    unsigned long getLastNTPSync() const;
    bool isNTPSyncRecent(unsigned long maxAgeMs = 24UL * 60UL * 60UL *
                                                  1000UL) const;  // default 24h

    // Non-blocking NTP sync methods
    void startNTPSync(bool updateRTC = true);
    bool updateNTPSync();  // Call this in main loop, returns true when complete
    bool isNTPSyncInProgress() const;
    bool wasLastNTPSyncSuccessful() const;
    bool checkAndClearNTPSyncCompletion();  // Returns true if just completed, then resets state

   private:
    enum NTPSyncState {
        NTP_IDLE,
        NTP_CONFIGURING,
        NTP_WAITING_FOR_TIME,
        NTP_COMPLETED_SUCCESS,
        NTP_COMPLETED_FAILURE
    };

    RTC_DS3231* rtc;
    const char* ntpServer;
    long gmtOffset_sec;
    int daylightOffset_sec;
    unsigned long lastNTPSync;
    unsigned long ntpSyncInterval;
    String timezoneString;

    // Simple timezone management
    int currentUTCOffset;
    bool supportsDST;
    int dstOffset;

    // Non-blocking NTP sync state
    NTPSyncState ntpState;
    bool ntpUpdateRTC;
    unsigned long ntpStartTime;
    bool ntpLastResult;
};

#endif  // TIME_MANAGER_H
