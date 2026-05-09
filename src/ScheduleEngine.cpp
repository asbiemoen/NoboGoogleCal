// Stub — implemented in issue #5
#include "ScheduleEngine.h"

ScheduleEngine::ScheduleEngine(NoboController& n, CalendarManager& c, WeatherService& w)
    : _nobo(n), _cal(c), _weather(w), _zones(nullptr), _zoneCount(0), _lastTickMs(0) {
    _statusBuf[0]    = '\0';
    _nextEventBuf[0] = '\0';
}

void ScheduleEngine::begin(const ZoneConfig*, int) {}
void ScheduleEngine::tick()                        {}
HeatingStatus ScheduleEngine::zoneStatus(int) const { return STATUS_ECO; }
const char* ScheduleEngine::statusString()    const { return _statusBuf; }
const char* ScheduleEngine::nextEventString() const { return _nextEventBuf; }
void ScheduleEngine::_evaluateZone(int)             {}
HeatingStatus ScheduleEngine::_desiredStatus(int) const { return STATUS_ECO; }
bool ScheduleEngine::_comfortWindowActive(int, time_t&) const { return false; }
void ScheduleEngine::_buildStatusString()           {}
void ScheduleEngine::_buildNextEventString()        {}
