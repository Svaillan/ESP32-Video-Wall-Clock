#include "TimeManager.h"

#include <sys/time.h>

TimeManager::TimeManager(RTC_DS3231* rtcInst, const char* ntpServer)
    : rtc(rtcInst),
      ntpServer(ntpServer),
      gmtOffset_sec(-7 * 3600),
      daylightOffset_sec(0),
      lastNTPSync(0),
      ntpSyncInterval(12UL * 60UL * 60UL * 1000UL),
      timezoneString("MST7"),
      currentUTCOffset(-7),
      supportsDST(false),
      dstOffset(0),
      ntpState(NTP_IDLE),
      ntpUpdateRTC(false),
      ntpStartTime(0),
      ntpLastResult(false) {}

void TimeManager::begin() {
    setTimezone(timezoneString.c_str());
}

void TimeManager::setTimezone(const char* tz) {
    timezoneString = tz;
    setenv("TZ", tz, 1);
    tzset();
    Serial.print("[TimeManager] Timezone set to: ");
    Serial.println(tz);
}

void TimeManager::setTimezoneAndUpdate(const char* tz) {
    setTimezone(tz);

    // Force time system to recalculate with new timezone
    // Get current time and re-set it to trigger timezone calculation
    time_t now = time(nullptr);
    if (now > 0) {
        struct timeval tv = {now, 0};
        settimeofday(&tv, nullptr);
        Serial.println("[TimeManager] System time updated for new timezone");
    }
}

void TimeManager::setTimezoneOffset(int utcOffsetHours, bool isDST, int dstOffsetHours) {
    currentUTCOffset = utcOffsetHours;
    supportsDST = isDST;
    dstOffset = dstOffsetHours;

    // Always sync with UTC (no offset to configTime)
    configTime(0, 0, ntpServer);

    Serial.printf("[TimeManager] Timezone set to UTC%+d (DST: %s, DST offset: %+d)\n",
                  utcOffsetHours, isDST ? "yes" : "no", dstOffsetHours);
}

bool TimeManager::isDSTActive(int month, int day, int utcOffsetHours) {
    // Simple DST rules for most regions:
    // US/Canada: Second Sunday in March to First Sunday in November
    // Europe: Last Sunday in March to Last Sunday in October
    // Southern Hemisphere: Opposite (October to March)

    if (utcOffsetHours >= 10) {
        // Southern hemisphere (Australia, New Zealand)
        return (month >= 10 || month <= 3);
    } else {
        // Northern hemisphere
        if (month < 3 || month > 11)
            return false;  // Jan, Feb, Dec
        if (month > 3 && month < 11)
            return true;  // Apr-Oct

        // March and November - simplified approximation
        if (month == 3)
            return day >= 14;  // Rough estimate for 2nd Sunday
        if (month == 11)
            return day <= 7;  // Rough estimate for 1st Sunday
    }

    return false;
}

DateTime TimeManager::getLocalTime() {
    // Get current UTC time from system
    time_t utcTime = time(nullptr);
    const struct tm* utcTm = gmtime(&utcTime);

    // Calculate local offset including DST
    int totalOffset = currentUTCOffset;
    if (supportsDST && isDSTActive(utcTm->tm_mon + 1, utcTm->tm_mday, currentUTCOffset)) {
        totalOffset += dstOffset;
    }

    // Apply offset to UTC time
    time_t localTime = utcTime + (totalOffset * 3600);  // Convert hours to seconds
    struct tm* localTm = gmtime(&localTime);

    return DateTime(localTm->tm_year + 1900, localTm->tm_mon + 1, localTm->tm_mday,
                    localTm->tm_hour, localTm->tm_min, localTm->tm_sec);
}

bool TimeManager::syncTimeWithNTP(bool updateRTC) {
    Serial.println("[TimeManager] Starting NTP sync...");
    // Use 0, 0 for offsets when using timezone strings - let TZ handle the conversion
    configTime(0, 0, ntpServer);
    struct tm timeinfo;
    if (::getLocalTime(&timeinfo, 10000)) {  // Use global getLocalTime function
        Serial.printf("[TimeManager] NTP Time: %04d-%02d-%02d %02d:%02d:%02d\n",
                      timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        if (updateRTC && rtc) {
            rtc->adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
            Serial.println("[TimeManager] RTC updated from NTP.");
        }
        lastNTPSync = millis();
        return true;
    } else {
        Serial.println("[TimeManager] Failed to get time from NTP server.");
        return false;
    }
}

void TimeManager::periodicNTPSync(unsigned long intervalMs) {
    // Only start a new sync if we're not already syncing and enough time has passed
    if (ntpState == NTP_IDLE && millis() - lastNTPSync > intervalMs) {
        Serial.println("[TimeManager] Starting periodic non-blocking NTP sync");
        startNTPSync(true);
    }
}

void TimeManager::updateRTCFromNTP() {
    // Only start if not already syncing
    if (ntpState == NTP_IDLE) {
        Serial.println("[TimeManager] Starting non-blocking RTC update from NTP");
        startNTPSync(true);
    }
}

void TimeManager::updateRTCFromSystem() {
    if (!rtc)
        return;
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    rtc->adjust(DateTime(timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
                         timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec));
    Serial.println("[TimeManager] RTC updated from system time.");
}

void TimeManager::updateSystemFromRTC() {
    if (!rtc)
        return;
    DateTime now = rtc->now();
    struct tm t;
    t.tm_year = now.year() - 1900;
    t.tm_mon = now.month() - 1;
    t.tm_mday = now.day();
    t.tm_hour = now.hour();
    t.tm_min = now.minute();
    t.tm_sec = now.second();
    time_t sysTime = mktime(&t);
    struct timeval tv = {sysTime, 0};
    settimeofday(&tv, nullptr);
    Serial.println("[TimeManager] System time updated from RTC.");
}

void TimeManager::setLastNTPSync(unsigned long ms) {
    lastNTPSync = ms;
}

unsigned long TimeManager::getLastNTPSync() const {
    return lastNTPSync;
}

bool TimeManager::isNTPSyncRecent(unsigned long maxAgeMs) const {
    if (lastNTPSync == 0)
        return false;
    return (millis() - lastNTPSync) <= maxAgeMs;
}

// ===================== NON-BLOCKING NTP SYNC METHODS =====================

void TimeManager::startNTPSync(bool updateRTC) {
    if (ntpState != NTP_IDLE) {
        Serial.println("[TimeManager] NTP sync already in progress");
        return;
    }

    Serial.println("[TimeManager] Starting non-blocking NTP sync...");
    ntpState = NTP_CONFIGURING;
    ntpUpdateRTC = updateRTC;
    ntpStartTime = millis();
    ntpLastResult = false;

    // Start the configuration - this might still block briefly but much less than getLocalTime
    configTime(0, 0, ntpServer);
    ntpState = NTP_WAITING_FOR_TIME;
}

bool TimeManager::updateNTPSync() {
    if (ntpState == NTP_IDLE)
        return false;
    if (ntpState == NTP_COMPLETED_SUCCESS || ntpState == NTP_COMPLETED_FAILURE) {
        return true;  // Already completed
    }

    if (ntpState == NTP_WAITING_FOR_TIME) {
        // Check if we've been waiting too long
        if (millis() - ntpStartTime > 10000) {  // 10 second timeout
            Serial.println("[TimeManager] NTP sync timed out");
            ntpState = NTP_COMPLETED_FAILURE;
            ntpLastResult = false;
            return true;
        }

        // Try to get time with very short timeout (non-blocking)
        struct tm timeinfo;
        if (::getLocalTime(&timeinfo, 50)) {  // 50ms timeout - much shorter!
            Serial.printf("[TimeManager] NTP Time: %04d-%02d-%02d %02d:%02d:%02d\n",
                          timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                          timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

            if (ntpUpdateRTC && rtc) {
                rtc->adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                                     timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
                Serial.println("[TimeManager] RTC updated from NTP.");
            }

            lastNTPSync = millis();
            ntpState = NTP_COMPLETED_SUCCESS;
            ntpLastResult = true;
            Serial.println("[TimeManager] Non-blocking NTP sync completed successfully");
            return true;
        }
        // If getLocalTime fails, we continue waiting (not completed yet)
    }

    return false;  // Still in progress
}

bool TimeManager::isNTPSyncInProgress() const {
    return (ntpState == NTP_CONFIGURING || ntpState == NTP_WAITING_FOR_TIME);
}

bool TimeManager::wasLastNTPSyncSuccessful() const {
    return ntpLastResult;
}

bool TimeManager::checkAndClearNTPSyncCompletion() {
    if (ntpState == NTP_COMPLETED_SUCCESS || ntpState == NTP_COMPLETED_FAILURE) {
        // Just completed - return true and reset to idle
        ntpState = NTP_IDLE;
        return true;
    }
    return false;
}
