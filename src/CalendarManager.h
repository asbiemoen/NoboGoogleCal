#pragma once
#include <WiFiS3.h>
#include <ArduinoHttpClient.h>
#include "Types.h"

class CalendarManager {
public:
    void begin(const ZoneConfig* zones, int zoneCount);
    void tick();

    // Returns all valid events; caller should read until valid==false
    const CalEvent* events()      const { return _events; }
    int             eventCount()  const { return _eventCount; }
    const char*     lastSyncTime()const { return _lastSync; }

private:
    const ZoneConfig* _zones;
    int               _zoneCount;
    CalEvent          _events[MAX_EVENTS_PER_ZONE * MAX_ZONES];
    int               _eventCount;
    char              _lastSync[20]; // "YYYY-MM-DD HH:MM"

    uint32_t _lastSyncMs;
    int      _nextZoneToSync;  // staggered fetch: one zone per minute
    uint32_t _lastSyncPeriod[MAX_ZONES];

    void _syncZone(int zoneIndex);
    bool _fetchIcs(const char* url, int zoneIndex);

    // ICS line-by-line parser state
    bool   _inEvent;
    time_t _evStart;
    time_t _evEnd;
    char   _evSummary[48];
    bool   _hasRRule;
    char   _rRule[64];

    void   _resetEventState();
    void   _commitEvent(int zoneIndex);
    void   _processLine(const char* line, int zoneIndex);
    time_t _parseDateTime(const char* val);
    void   _expandRRule(time_t base, time_t baseEnd, int zoneIndex);
    bool   _inNext7Days(time_t t) const;
};
