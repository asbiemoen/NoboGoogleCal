#include "CalendarManager.h"
#include <Arduino.h>
#include <WiFiSSLClient.h>
#include <ArduinoHttpClient.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

void CalendarManager::begin(const ZoneConfig* zones, int zoneCount) {
    _zones          = zones;
    _zoneCount      = zoneCount;
    _eventCount     = 0;
    _lastSyncMs     = 0;
    _nextZoneToSync = 0;
    _lastSync[0]    = '\0';

    // Mark all events invalid
    for (int i = 0; i < MAX_EVENTS_PER_ZONE * MAX_ZONES; i++) {
        _events[i].valid = false;
    }
}

void CalendarManager::tick() {
    uint32_t now = millis();

    // Each zone is synced one minute apart within the hour window
    // Zone 0 at :00, zone 1 at :01, etc.
    // We use (now / 60000) % SYNC_INTERVAL_MS to stagger fetches.
    uint32_t minutesSinceBoot = now / 60000UL;
    uint32_t syncPeriod       = SYNC_INTERVAL_MS / 60000UL;  // e.g. 60 minutes

    // Find which zone should sync this minute
    int scheduledZone = (int)(minutesSinceBoot % syncPeriod);
    if (scheduledZone >= _zoneCount) return;  // no zone assigned to this minute

    // Check if we haven't already synced this zone in this period
    uint32_t periodId = minutesSinceBoot / syncPeriod;
    static uint32_t lastSyncPeriod[MAX_ZONES] = {};
    if (lastSyncPeriod[scheduledZone] == periodId) return;

    lastSyncPeriod[scheduledZone] = periodId;
    _syncZone(scheduledZone);

    // Update lastSync timestamp
    time_t now_t = time(nullptr);
    struct tm* t = gmtime(&now_t);
    snprintf(_lastSync, sizeof(_lastSync), "%04d-%02d-%02d %02d:%02d",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min);
}

void CalendarManager::_syncZone(int zoneIndex) {
    Serial.print(F("[Cal] Syncing zone "));
    Serial.println(_zones[zoneIndex].name);

    // Clear existing events for this zone
    int count = 0;
    for (int i = 0; i < MAX_EVENTS_PER_ZONE * MAX_ZONES; i++) {
        if (_events[i].valid && _events[i].zoneIndex == (uint8_t)zoneIndex) {
            _events[i].valid = false;
        }
    }

    _fetchIcs(_zones[zoneIndex].icsUrl, zoneIndex);

    // Recount
    _eventCount = 0;
    for (int i = 0; i < MAX_EVENTS_PER_ZONE * MAX_ZONES; i++) {
        if (_events[i].valid) _eventCount++;
    }
    Serial.print(F("[Cal] Total valid events: "));
    Serial.println(_eventCount);
}

bool CalendarManager::_fetchIcs(const char* url, int zoneIndex) {
    // Parse host and path from URL
    // Expected: https://calendar.google.com/calendar/ical/...
    char host[64] = "calendar.google.com";
    const char* pathStart = strstr(url, ".com");
    if (!pathStart) return false;
    pathStart += 4;  // skip ".com"

    WiFiSSLClient ssl;
    HttpClient http(ssl, host, 443);
    http.setTimeout(8000);

    int err = http.get(pathStart);
    if (err != 0) {
        Serial.println(F("[Cal] HTTP GET failed"));
        return false;
    }

    int statusCode = http.responseStatusCode();
    if (statusCode != 200) {
        Serial.print(F("[Cal] HTTP status: "));
        Serial.println(statusCode);
        http.stop();
        return false;
    }

    http.skipResponseHeaders();

    _resetEventState();

    char line[128];
    int  pos     = 0;
    bool timeout = false;
    uint32_t deadline = millis() + 15000UL;

    while (http.connected() || http.available()) {
        if (millis() > deadline) { timeout = true; break; }
        if (!http.available()) { delay(10); continue; }

        char c = (char)http.read();
        if (c == '\n') {
            if (pos > 0 && line[pos - 1] == '\r') pos--;
            line[pos] = '\0';
            _processLine(line, zoneIndex);
            pos = 0;
        } else if (pos < (int)sizeof(line) - 1) {
            line[pos++] = c;
        }
    }

    http.stop();
    if (timeout) Serial.println(F("[Cal] Fetch timed out"));
    return !timeout;
}

void CalendarManager::_resetEventState() {
    _inEvent    = false;
    _evStart    = 0;
    _evEnd      = 0;
    _evSummary[0] = '\0';
    _hasRRule   = false;
    _rRule[0]   = '\0';
}

void CalendarManager::_processLine(const char* line, int zoneIndex) {
    if (strcmp(line, "BEGIN:VEVENT") == 0) {
        _resetEventState();
        _inEvent = true;
        return;
    }
    if (strcmp(line, "END:VEVENT") == 0) {
        if (_inEvent) _commitEvent(zoneIndex);
        _inEvent = false;
        return;
    }
    if (!_inEvent) return;

    if (strncmp(line, "DTSTART", 7) == 0) {
        const char* val = strchr(line, ':');
        if (val) _evStart = _parseDateTime(val + 1);
    } else if (strncmp(line, "DTEND", 5) == 0) {
        const char* val = strchr(line, ':');
        if (val) _evEnd = _parseDateTime(val + 1);
    } else if (strncmp(line, "SUMMARY:", 8) == 0) {
        strncpy(_evSummary, line + 8, sizeof(_evSummary) - 1);
    } else if (strncmp(line, "RRULE:", 6) == 0) {
        strncpy(_rRule, line + 6, sizeof(_rRule) - 1);
        _hasRRule = true;
    }
}

void CalendarManager::_commitEvent(int zoneIndex) {
    if (_evStart == 0 || _evEnd == 0) return;

    if (_hasRRule) {
        _expandRRule(_evStart, _evEnd, zoneIndex);
    } else {
        if (!_inNext7Days(_evStart)) return;
        // Find free slot
        for (int i = 0; i < MAX_EVENTS_PER_ZONE * MAX_ZONES; i++) {
            if (!_events[i].valid) {
                _events[i] = { _evStart, _evEnd, {}, (uint8_t)zoneIndex, true };
                strncpy(_events[i].summary, _evSummary, sizeof(_events[i].summary) - 1);
                break;
            }
        }
    }
}

// Parse ICS datetime: "20260510T100000Z", "20260510T100000", "20260510"
time_t CalendarManager::_parseDateTime(const char* val) {
    if (!val || strlen(val) < 8) return 0;

    struct tm t = {};
    char buf[5];

    // Year
    strncpy(buf, val, 4); buf[4] = '\0'; t.tm_year = atoi(buf) - 1900;
    // Month
    strncpy(buf, val + 4, 2); buf[2] = '\0'; t.tm_mon = atoi(buf) - 1;
    // Day
    strncpy(buf, val + 6, 2); buf[2] = '\0'; t.tm_mday = atoi(buf);

    bool isAllDay = (val[8] != 'T');

    if (!isAllDay && strlen(val) >= 15) {
        strncpy(buf, val + 9,  2); buf[2] = '\0'; t.tm_hour = atoi(buf);
        strncpy(buf, val + 11, 2); buf[2] = '\0'; t.tm_min  = atoi(buf);
        strncpy(buf, val + 13, 2); buf[2] = '\0'; t.tm_sec  = atoi(buf);
    }

    bool isUtc = (val[strlen(val) - 1] == 'Z');
    t.tm_isdst = -1;
    time_t epoch = mktime(&t);

    if (!isUtc && !isAllDay) {
        // Assume local time is Norwegian (CET/CEST); subtract offset to get UTC
        epoch -= norwayOffsetSeconds(&t);
    }
    return epoch;
}

void CalendarManager::_expandRRule(time_t base, time_t baseEnd, int zoneIndex) {
    // Supports: FREQ=WEEKLY;BYDAY=MO,TU,WE,TH,FR,SA,SU
    //           FREQ=DAILY
    //           FREQ=WEEKLY (no BYDAY = same weekday as DTSTART)
    time_t now   = time(nullptr);
    time_t limit = now + 7 * 24 * 3600;
    time_t dur   = baseEnd - base;

    const char* freq = strstr(_rRule, "FREQ=");
    if (!freq) return;
    freq += 5;

    bool weekly = (strncmp(freq, "WEEKLY", 6) == 0);
    bool daily  = (strncmp(freq, "DAILY",  5) == 0);
    if (!weekly && !daily) return;

    int step = daily ? 1 : 7;

    // BYDAY handling for weekly
    const char* byDay = strstr(_rRule, "BYDAY=");
    const char* days[7] = {"MO","TU","WE","TH","FR","SA","SU"};
    // struct tm wday: 0=Sun,1=Mon,...,6=Sat; ICS BYDAY: MO=Mon(1)...SU=Sun(0)
    int isoWday[7] = {1,2,3,4,5,6,0};  // MO=1,...,SU=0 in tm

    for (time_t t = now - 7 * 24 * 3600; t < limit; t += 86400) {
        struct tm* tm_t = gmtime(&t);

        bool matches = false;
        if (daily) {
            matches = true;
        } else if (byDay) {
            for (int d = 0; d < 7; d++) {
                if (strstr(byDay + 6, days[d]) && tm_t->tm_wday == isoWday[d]) {
                    matches = true; break;
                }
            }
        } else {
            // Same weekday as DTSTART
            struct tm* baseTm = gmtime(&base);
            matches = (tm_t->tm_wday == baseTm->tm_wday);
        }

        if (!matches) continue;

        // Build event time: same H:M:S as base, but on this date
        struct tm* baseTm = gmtime(&base);
        struct tm evTm    = *tm_t;
        evTm.tm_hour      = baseTm->tm_hour;
        evTm.tm_min       = baseTm->tm_min;
        evTm.tm_sec       = baseTm->tm_sec;
        time_t evStart    = mktime(&evTm);
        time_t evEnd      = evStart + dur;

        if (!_inNext7Days(evStart)) continue;

        for (int i = 0; i < MAX_EVENTS_PER_ZONE * MAX_ZONES; i++) {
            if (!_events[i].valid) {
                _events[i] = { evStart, evEnd, {}, (uint8_t)zoneIndex, true };
                strncpy(_events[i].summary, _evSummary, sizeof(_events[i].summary) - 1);
                break;
            }
        }
        if (!daily) t += (step - 1) * 86400;
    }
}

bool CalendarManager::_inNext7Days(time_t t) const {
    time_t now   = time(nullptr);
    time_t limit = now + 7 * 24 * 3600;
    return (t >= now - 24 * 3600) && (t < limit);
}
