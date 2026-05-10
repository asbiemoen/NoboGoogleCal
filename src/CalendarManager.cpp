#include "CalendarManager.h"
#include "AppLog.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <WiFiSSLClient.h>
#include <ArduinoHttpClient.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

// ─── EEPROM layout ────────────────────────────────────────────────────────────
// [0]   magic 0xCA
// [1]   magic 0x1E
// [2-3] uint16_t count
// [4+]  EepromEvent[] (34 bytes each)

static const uint8_t EEPROM_MAGIC_0 = 0xCA;
static const uint8_t EEPROM_MAGIC_1 = 0x1E;

struct EepromEvent {
    uint32_t start;
    uint32_t end;
    uint8_t  zoneIndex;
    char     summary[24];
    uint8_t  _pad;  // total: 34 bytes
};

void CalendarManager::begin(const ZoneConfig* zones, int zoneCount) {
    _zones              = zones;
    _zoneCount          = zoneCount;
    _eventCount         = 0;
    _lastSyncMs         = 0;
    _nextZoneToSync     = 0;
    _lastSync[0]        = '\0';
    _lastEepromSaveDay  = 0;

    for (int i = 0; i < MAX_ZONES; i++) { _lastSyncPeriod[i] = 0xFFFFFFFFUL; _syncing[i] = false; }

    for (int i = 0; i < MAX_EVENTS_PER_ZONE * MAX_ZONES; i++) {
        _events[i].valid = false;
    }

    _loadFromEeprom();
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
    if (_lastSyncPeriod[scheduledZone] == periodId) return;

    _lastSyncPeriod[scheduledZone] = periodId;
    _syncZone(scheduledZone);

    // Update lastSync timestamp
    time_t now_t = time(nullptr);
    struct tm utcTm = *gmtime(&now_t);
    time_t localNow = now_t + (time_t)norwayOffsetSeconds(&utcTm);
    struct tm lTm = *gmtime(&localNow);
    snprintf(_lastSync, sizeof(_lastSync), "%04d-%02d-%02d %02d:%02d:%02d",
             lTm.tm_year + 1900, lTm.tm_mon + 1, lTm.tm_mday, lTm.tm_hour, lTm.tm_min, lTm.tm_sec);
}

void CalendarManager::_syncZone(int zoneIndex) {
    _syncing[zoneIndex] = true;
    serialTs(); Serial.print(F("[Cal] Syncing zone "));
    Serial.println(_zones[zoneIndex].name);
    char startMsg[APP_LOG_WIDTH];
    snprintf(startMsg, sizeof(startMsg), "Cal Z%d: syncing...", zoneIndex + 1);
    AppLog::add(startMsg);

    for (int i = 0; i < MAX_EVENTS_PER_ZONE * MAX_ZONES; i++) {
        if (_events[i].valid && _events[i].zoneIndex == (uint8_t)zoneIndex)
            _events[i].valid = false;
    }

    bool ok = _fetchIcs(_zones[zoneIndex].icsUrl, zoneIndex);

    _eventCount = 0;
    int zoneEvCount = 0;
    for (int i = 0; i < MAX_EVENTS_PER_ZONE * MAX_ZONES; i++) {
        if (!_events[i].valid) continue;
        _eventCount++;
        if (_events[i].zoneIndex == (uint8_t)zoneIndex) zoneEvCount++;
    }
    serialTs(); Serial.print(F("[Cal] Total valid events: "));
    Serial.println(_eventCount);

    char msg[APP_LOG_WIDTH];
    if (ok) {
        snprintf(msg, sizeof(msg), "Cal Z%d: %d event%s",
                 zoneIndex + 1, zoneEvCount, zoneEvCount == 1 ? "" : "s");
        AppLog::add(msg);

        uint32_t today = (uint32_t)(time(nullptr) / 86400UL);
        if (_lastEepromSaveDay != today) {
            _saveToEeprom();
            _lastEepromSaveDay = today;
        }
    }
    // failure already logged by _fetchIcs with specific reason
    _syncing[zoneIndex] = false;
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
        serialTs(); Serial.print(F("[Cal] TCP/SSL error: "));
        Serial.println(err);
        char msg[APP_LOG_WIDTH];
        snprintf(msg, sizeof(msg), "Cal Z%d: TCP err %d", zoneIndex + 1, err);
        AppLog::add(msg);
        return false;
    }

    int statusCode = http.responseStatusCode();
    if (statusCode != 200) {
        serialTs(); Serial.print(F("[Cal] HTTP status: "));
        Serial.println(statusCode);
        http.stop();
        char msg[APP_LOG_WIDTH];
        snprintf(msg, sizeof(msg), "Cal Z%d: HTTP %d", zoneIndex + 1, statusCode);
        AppLog::add(msg);
        return false;
    }

    http.skipResponseHeaders();

    _resetEventState();

    char line[128];
    int  pos     = 0;
    bool timeout = false;
    uint32_t deadline = millis() + 600000UL;  // 10 min — ICS can be large

    uint32_t lastDataMs = millis();
    while (http.connected() || http.available()) {
        if (millis() > deadline) { timeout = true; break; }
        if (!http.available()) {
            // If no new data for 15 s the connection has stalled or finished
            // without a clean close — treat as end of stream and continue with
            // whatever events we've parsed so far.
            if (millis() - lastDataMs > 15000UL) break;
            delay(1);
            continue;
        }
        lastDataMs = millis();
        char c = (char)http.read();
        if (c == '\n') {
            if (pos > 0 && line[pos - 1] == '\r') pos--;
            line[pos] = '\0';
            if (strcmp(line, "END:VCALENDAR") == 0) break;
            _processLine(line, zoneIndex);
            pos = 0;
        } else if (pos < (int)sizeof(line) - 1) {
            line[pos++] = c;
        }
    }

    http.stop();
    if (timeout) {
        serialTs(); Serial.println(F("[Cal] Fetch timed out"));
        char msg[APP_LOG_WIDTH];
        snprintf(msg, sizeof(msg), "Cal Z%d: timeout", zoneIndex + 1);
        AppLog::add(msg);
    }
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
    //           UNTIL=  and COUNT=  qualifiers
    time_t now   = time(nullptr);
    time_t limit = now + 7 * 24 * 3600;
    time_t dur   = baseEnd - base;

    const char* freq = strstr(_rRule, "FREQ=");
    if (!freq) return;
    freq += 5;

    bool weekly = (strncmp(freq, "WEEKLY", 6) == 0);
    bool daily  = (strncmp(freq, "DAILY",  5) == 0);
    if (!weekly && !daily) return;

    // Parse UNTIL — stop generating occurrences after this time
    time_t until = 0;
    const char* untilP = strstr(_rRule, "UNTIL=");
    if (untilP) until = _parseDateTime(untilP + 6);

    // Parse COUNT — stop after N occurrences
    int maxCount = INT_MAX;
    const char* countP = strstr(_rRule, "COUNT=");
    if (countP) maxCount = atoi(countP + 6);

    int step = daily ? 1 : 7;

    // BYDAY handling for weekly
    const char* byDay = strstr(_rRule, "BYDAY=");
    const char* days[7]  = {"MO","TU","WE","TH","FR","SA","SU"};
    int         isoWday[7] = {1,2,3,4,5,6,0};  // MO=1,...,SU=0 in tm_wday

    // gmtime() returns a pointer to a single static buffer — copy before second call
    struct tm baseTm = *gmtime(&base);

    int generated = 0;
    for (time_t t = now - 7 * 24 * 3600; t < limit && generated < maxCount; t += 86400) {
        struct tm tm_day = *gmtime(&t);  // copy out of static buffer immediately

        bool matches = false;
        if (daily) {
            matches = true;
        } else if (byDay) {
            for (int d = 0; d < 7; d++) {
                if (strstr(byDay + 6, days[d]) && tm_day.tm_wday == isoWday[d]) {
                    matches = true; break;
                }
            }
        } else {
            matches = (tm_day.tm_wday == baseTm.tm_wday);
        }

        if (!matches) continue;

        // Build event time: same H:M:S as base, but on this calendar date
        struct tm evTm = tm_day;
        evTm.tm_hour   = baseTm.tm_hour;
        evTm.tm_min    = baseTm.tm_min;
        evTm.tm_sec    = baseTm.tm_sec;
        time_t evStart = mktime(&evTm);
        time_t evEnd   = evStart + dur;

        if (until > 0 && evStart > until) break;
        if (!_inNext7Days(evStart)) { if (!daily) t += (step - 1) * 86400; continue; }

        for (int i = 0; i < MAX_EVENTS_PER_ZONE * MAX_ZONES; i++) {
            if (!_events[i].valid) {
                _events[i] = { evStart, evEnd, {}, (uint8_t)zoneIndex, true };
                strncpy(_events[i].summary, _evSummary, sizeof(_events[i].summary) - 1);
                generated++;
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

void CalendarManager::_saveToEeprom() {
    int maxCount = MAX_EVENTS_PER_ZONE * MAX_ZONES;
    uint16_t count = 0;

    for (int i = 0; i < maxCount; i++) {
        if (!_events[i].valid) continue;
        EepromEvent ee;
        ee.start     = (uint32_t)_events[i].start;
        ee.end       = (uint32_t)_events[i].end;
        ee.zoneIndex = _events[i].zoneIndex;
        ee._pad      = 0;
        strncpy(ee.summary, _events[i].summary, sizeof(ee.summary) - 1);
        ee.summary[sizeof(ee.summary) - 1] = '\0';
        EEPROM.put(4 + count * (int)sizeof(EepromEvent), ee);
        count++;
    }

    EEPROM.write(0, EEPROM_MAGIC_0);
    EEPROM.write(1, EEPROM_MAGIC_1);
    EEPROM.put(2, count);

    serialTs(); Serial.print(F("[Cal] EEPROM: saved "));
    Serial.print(count);
    Serial.println(F(" events"));
    char msg[APP_LOG_WIDTH];
    snprintf(msg, sizeof(msg), "Cal: saved %d to EEPROM", (int)count);
    AppLog::add(msg);
}

void CalendarManager::_loadFromEeprom() {
    if (EEPROM.read(0) != EEPROM_MAGIC_0 || EEPROM.read(1) != EEPROM_MAGIC_1) return;

    uint16_t count = 0;
    EEPROM.get(2, count);
    int maxCount = MAX_EVENTS_PER_ZONE * MAX_ZONES;
    if (count > (uint16_t)maxCount) count = (uint16_t)maxCount;

    time_t now = time(nullptr);
    int loaded = 0;
    EepromEvent ee;
    for (uint16_t i = 0; i < count; i++) {
        EEPROM.get(4 + i * (int)sizeof(EepromEvent), ee);
        if ((time_t)ee.end < now) continue;  // skip events already past
        for (int j = 0; j < maxCount; j++) {
            if (!_events[j].valid) {
                _events[j].start     = (time_t)ee.start;
                _events[j].end       = (time_t)ee.end;
                _events[j].zoneIndex = ee.zoneIndex;
                strncpy(_events[j].summary, ee.summary, sizeof(_events[j].summary) - 1);
                _events[j].valid = true;
                _eventCount++;
                loaded++;
                break;
            }
        }
    }

    if (loaded > 0) {
        serialTs(); Serial.print(F("[Cal] EEPROM: loaded "));
        Serial.print(loaded);
        Serial.println(F(" events"));
        char msg[APP_LOG_WIDTH];
        snprintf(msg, sizeof(msg), "Cal: loaded %d from EEPROM", loaded);
        AppLog::add(msg);
    }
}

void CalendarManager::forceSyncAll() {
    serialTs(); Serial.println(F("[Cal] Force sync all zones"));
    for (int i = 0; i < _zoneCount; i++) {
        _syncZone(i);
    }
    _saveToEeprom();
    _lastEepromSaveDay = (uint32_t)(time(nullptr) / 86400UL);

    // Prevent auto-sync from immediately re-syncing in this period
    uint32_t periodId = (millis() / 60000UL) / (SYNC_INTERVAL_MS / 60000UL);
    for (int i = 0; i < _zoneCount; i++) _lastSyncPeriod[i] = periodId;

    time_t now_t = time(nullptr);
    struct tm utcTm = *gmtime(&now_t);
    time_t localNow = now_t + (time_t)norwayOffsetSeconds(&utcTm);
    struct tm lTm = *gmtime(&localNow);
    snprintf(_lastSync, sizeof(_lastSync), "%04d-%02d-%02d %02d:%02d:%02d",
             lTm.tm_year + 1900, lTm.tm_mon + 1, lTm.tm_mday, lTm.tm_hour, lTm.tm_min, lTm.tm_sec);
}
