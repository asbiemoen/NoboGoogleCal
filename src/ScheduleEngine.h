#pragma once
#include "Types.h"
#include "NoboController.h"
#include "CalendarManager.h"
#include "WeatherService.h"

class ScheduleEngine {
public:
    ScheduleEngine(NoboController& nobo, CalendarManager& cal, WeatherService& weather);

    void begin(const ZoneConfig* zones, int zoneCount);
    void tick();

    int           zoneCount()             const { return _zoneCount; }
    const char*   zoneName(int i)         const;
    HeatingStatus zoneStatus(int i)       const;
    const char*   statusString()          const;
    const char*   nextEventString()       const;

private:
    NoboController& _nobo;
    CalendarManager& _cal;
    WeatherService&  _weather;

    const ZoneConfig* _zones;
    int               _zoneCount;
    ZoneState         _states[MAX_ZONES];
    uint32_t          _lastTickMs;
    char              _statusBuf[64];
    char              _nextEventBuf[64];

    void _evaluateZone(int i);
    HeatingStatus _desiredStatus(int zoneIndex) const;
    bool _comfortWindowActive(int zoneIndex, time_t& comfortUntil) const;
    void _buildStatusString();
    void _buildNextEventString();
};
