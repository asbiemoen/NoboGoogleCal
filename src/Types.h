#pragma once
#include <Arduino.h>

// ─── Capacity constants ───────────────────────────────────────────────────────
#define MAX_ZONES           8
#define MAX_EVENTS_PER_ZONE 20
#define MAX_NOBO_ZONES      16
#define MAX_NOBO_PROFILES   16
#define MAX_NOBO_OVERRIDES  16

// ─── Timing ───────────────────────────────────────────────────────────────────
#define SYNC_INTERVAL_MS    (60UL * 60UL * 1000UL)         // 1 hour
#define WEATHER_INTERVAL_MS (3UL  * 60UL * 60UL * 1000UL) // 3 hours
#define ENGINE_INTERVAL_MS  (60UL * 1000UL)                // 1 minute
#define KEEPALIVE_MS        (30UL * 1000UL)                // 30 seconds

// ─── Types ────────────────────────────────────────────────────────────────────

enum HeatingStatus : uint8_t {
    STATUS_NORMAL  = 0,
    STATUS_COMFORT = 1,
    STATUS_ECO     = 2,
    STATUS_AWAY    = 3
};

struct ZoneConfig {
    const char*   name;
    const char*   icsUrl;
    uint8_t       preheatHours;
    HeatingStatus defaultStatus;
};

struct CalEvent {
    time_t  start;
    time_t  end;
    char    summary[48];
    uint8_t zoneIndex;
    bool    valid;
};

struct ZoneState {
    HeatingStatus current;
    int           noboZoneId;    // discovered from hub; -1 if not found
    int           overrideId;    // active override ID; -1 if none
};

// ─── Helpers ──────────────────────────────────────────────────────────────────

inline const char* statusName(HeatingStatus s) {
    switch (s) {
        case STATUS_COMFORT: return "COMFORT";
        case STATUS_ECO:     return "ECO";
        case STATUS_AWAY:    return "AWAY";
        default:             return "NORMAL";
    }
}

// Norway timezone offset in seconds (CET=3600, CEST=7200).
// CEST: last Sunday in March (02:00) → last Sunday in October (03:00).
inline int norwayOffsetSeconds(const struct tm* t) {
    int mon  = t->tm_mon;   // 0=Jan
    int mday = t->tm_mday;
    int wday = t->tm_wday;  // 0=Sun

    if (mon < 2 || mon > 9) return 3600;   // Jan, Feb, Nov, Dec
    if (mon > 2 && mon < 9) return 7200;   // Apr – Sep

    // March (2) and October (9): compute day-of-month of last Sunday.
    // Both months have 31 days. wday of the 31st = (wday + (31 - mday)) % 7.
    int wdayOf31 = (wday + (31 - mday)) % 7;
    int lastSun  = 31 - wdayOf31;

    if (mon == 2) return (mday >= lastSun) ? 7200 : 3600;  // March
    return             (mday >= lastSun) ? 3600 : 7200;    // October
}
