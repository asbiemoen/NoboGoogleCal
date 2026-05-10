#include "ScheduleEngine.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

ScheduleEngine::ScheduleEngine(NoboController& nobo, CalendarManager& cal, WeatherService& weather)
    : _nobo(nobo), _cal(cal), _weather(weather),
      _zones(nullptr), _zoneCount(0), _lastTickMs(0) {
    _statusBuf[0]    = '\0';
    _nextEventBuf[0] = '\0';
}

void ScheduleEngine::begin(const ZoneConfig* zones, int zoneCount) {
    _zones     = zones;
    _zoneCount = zoneCount;

    for (int i = 0; i < zoneCount && i < MAX_ZONES; i++) {
        _states[i]    = { zones[i].defaultStatus, -1, -1 };
        _overrides[i] = { false, 0, false };
    }
}

void ScheduleEngine::tick() {
    if (millis() - _lastTickMs < ENGINE_INTERVAL_MS) return;
    _lastTickMs = millis();

    // Expire overrides whose time has passed
    time_t now = time(nullptr);
    for (int i = 0; i < _zoneCount; i++) {
        if (_overrides[i].active && now >= _overrides[i].until)
            _overrides[i].active = false;
    }

    for (int i = 0; i < _zoneCount; i++) {
        _evaluateZone(i);
    }
    _buildStatusString();
    _buildNextEventString();
}

const char* ScheduleEngine::zoneName(int i) const {
    if (i < 0 || i >= _zoneCount) return "";
    return _zones[i].name;
}

HeatingStatus ScheduleEngine::zoneStatus(int i) const {
    if (i < 0 || i >= _zoneCount) return STATUS_ECO;
    return _states[i].current;
}

const char* ScheduleEngine::statusString()    const { return _statusBuf; }
const char* ScheduleEngine::nextEventString() const { return _nextEventBuf; }

const char* ScheduleEngine::zoneEventLabel(int i) const {
    if (i < 0 || i >= _zoneCount || !_zones[i].eventLabel) return "Event";
    return _zones[i].eventLabel;
}

uint8_t ScheduleEngine::zonePreheatHours(int i) const {
    if (i < 0 || i >= _zoneCount) return 0;
    return _zones[i].preheatHours;
}

bool ScheduleEngine::nextChangeForZone(int i, time_t withinSecs, time_t& changeAt, bool& toComfort) const {
    if (i < 0 || i >= _zoneCount) return false;
    if (!_weather.comfortAllowed()) return false;

    time_t now = time(nullptr);
    time_t comfortUntil;
    bool   inComfort = _comfortWindowActive(i, comfortUntil);

    if (inComfort) {
        time_t secsLeft = comfortUntil - now;
        if (secsLeft > 0 && secsLeft <= withinSecs) {
            changeAt  = comfortUntil;
            toComfort = false;
            return true;
        }
    } else {
        uint8_t preheat = _zones[i].preheatHours;
        const CalEvent* events = _cal.events();
        time_t nearest = 0;

        for (int j = 0; j < MAX_EVENTS_PER_ZONE * MAX_ZONES; j++) {
            const CalEvent& ev = events[j];
            if (!ev.valid || ev.zoneIndex != (uint8_t)i) continue;
            time_t preheatFrom = ev.start - (time_t)preheat * 3600;
            if (preheatFrom > now && preheatFrom - now <= withinSecs) {
                if (nearest == 0 || preheatFrom < nearest)
                    nearest = preheatFrom;
            }
        }

        if (nearest > 0) {
            changeAt  = nearest;
            toComfort = true;
            return true;
        }
    }
    return false;
}

// ─── Private ─────────────────────────────────────────────────────────────────

void ScheduleEngine::_evaluateZone(int i) {
    if (!_nobo.isConnected()) return;

    // Lazily discover Nobø zone ID if not yet known
    if (_states[i].noboZoneId < 0) {
        _states[i].noboZoneId = _nobo.findZoneIdByProfileName(_zones[i].name);
    }

    HeatingStatus desired = _desiredStatus(i);

    if (desired == _states[i].current) return;  // no change

    int zoneId = _states[i].noboZoneId;
    if (zoneId < 0) return;  // zone not yet matched to Nobø

    if (desired == STATUS_COMFORT) {
        // Find the end of the merged comfort window
        time_t comfortUntil;
        _comfortWindowActive(i, comfortUntil);

        // Clear any existing override first
        if (_states[i].overrideId >= 0) {
            _nobo.clearOverride(_states[i].overrideId);
            _states[i].overrideId = -1;
        }

        int newId;
        if (_nobo.setZoneComfort(zoneId, time(nullptr), comfortUntil, newId)) {
            _states[i].overrideId = newId;
            _states[i].current    = STATUS_COMFORT;
            Serial.print(F("[Engine] Zone COMFORT: "));
            Serial.println(_zones[i].name);
        }
    } else {
        // Return to default — clear override
        if (_states[i].overrideId >= 0) {
            _nobo.clearOverride(_states[i].overrideId);
            _states[i].overrideId = -1;
        }
        _states[i].current = desired;
        Serial.print(F("[Engine] Zone reset to "));
        Serial.print(statusName(desired));
        Serial.print(F(": "));
        Serial.println(_zones[i].name);
    }
}

void ScheduleEngine::setOverride(int zone, float hours) {
    if (zone < 0 || zone >= _zoneCount) return;
    if (hours <= 0.0f || _overrides[zone].active) {
        _overrides[zone].active = false;
        _evaluateZone(zone);
        return;
    }
    _overrides[zone].active  = true;
    _overrides[zone].until   = time(nullptr) + (time_t)(hours * 3600.0f + 0.5f);
    _overrides[zone].isBoost = (_states[zone].current != STATUS_COMFORT);
    Serial.print(F("[Engine] Override zone "));
    Serial.print(_zones[zone].name);
    Serial.println(_overrides[zone].isBoost ? F(" BOOST") : F(" MUTE"));
    _evaluateZone(zone);
}

HeatingStatus ScheduleEngine::_desiredStatus(int zoneIndex) const {
    const ZoneOverride& ov = _overrides[zoneIndex];
    if (ov.active) {
        if (ov.isBoost) return STATUS_COMFORT;
        // Mute: yield if calendar/preheat window is active
        time_t dummy;
        if (_comfortWindowActive(zoneIndex, dummy)) return STATUS_COMFORT;
        return _zones[zoneIndex].defaultStatus;
    }
    if (!_weather.comfortAllowed()) return _zones[zoneIndex].defaultStatus;
    time_t dummy;
    return _comfortWindowActive(zoneIndex, dummy) ? STATUS_COMFORT
                                                  : _zones[zoneIndex].defaultStatus;
}

bool ScheduleEngine::_comfortWindowActive(int zoneIndex, time_t& comfortUntil) const {
    time_t now = time(nullptr);
    uint8_t preheat = _zones[zoneIndex].preheatHours;

    // Collect all events for this zone and merge overlapping comfort windows
    time_t windowStart = 0;
    time_t windowEnd   = 0;

    const CalEvent* events = _cal.events();
    int count = _cal.eventCount();

    for (int i = 0; i < MAX_EVENTS_PER_ZONE * MAX_ZONES; i++) {
        const CalEvent& ev = events[i];
        if (!ev.valid || ev.zoneIndex != (uint8_t)zoneIndex) continue;

        time_t comfortFrom = ev.start - (time_t)preheat * 3600;
        time_t comfortTo   = ev.end;

        // Check if we're currently inside this comfort window
        if (now >= comfortFrom && now < comfortTo) {
            if (windowStart == 0) {
                windowStart = comfortFrom;
                windowEnd   = comfortTo;
            } else {
                // Merge overlapping windows
                if (comfortFrom <= windowEnd) {
                    if (comfortTo > windowEnd) windowEnd = comfortTo;
                } else {
                    // Gap — take the window we're currently in
                    if (now >= windowStart && now < windowEnd) break;
                    windowStart = comfortFrom;
                    windowEnd   = comfortTo;
                }
            }
        }
    }

    if (windowStart > 0) {
        comfortUntil = windowEnd;
        return true;
    }
    return false;
}

void ScheduleEngine::_buildStatusString() {
    _statusBuf[0] = '\0';
    for (int i = 0; i < _zoneCount; i++) {
        if (i > 0) strncat(_statusBuf, " | ", sizeof(_statusBuf) - strlen(_statusBuf) - 1);
        strncat(_statusBuf, _zones[i].name,           sizeof(_statusBuf) - strlen(_statusBuf) - 1);
        strncat(_statusBuf, ":",                       sizeof(_statusBuf) - strlen(_statusBuf) - 1);
        strncat(_statusBuf, statusName(_states[i].current),
                sizeof(_statusBuf) - strlen(_statusBuf) - 1);
    }
}

void ScheduleEngine::_buildNextEventString() {
    time_t now     = time(nullptr);
    time_t nearest = 0;
    char   summary[48] = {};
    int    nearestZone = -1;

    const CalEvent* events = _cal.events();
    for (int i = 0; i < MAX_EVENTS_PER_ZONE * MAX_ZONES; i++) {
        const CalEvent& ev = events[i];
        if (!ev.valid) continue;
        time_t preheatFrom = ev.start - (time_t)_zones[ev.zoneIndex].preheatHours * 3600;
        if (preheatFrom > now && (nearest == 0 || preheatFrom < nearest)) {
            nearest    = preheatFrom;
            nearestZone = ev.zoneIndex;
            strncpy(summary, ev.summary, sizeof(summary) - 1);
        }
    }

    if (nearest == 0) {
        strncpy(_nextEventBuf, "No upcoming events", sizeof(_nextEventBuf) - 1);
    } else {
        long minsUntil = (long)(nearest - now) / 60;
        snprintf(_nextEventBuf, sizeof(_nextEventBuf),
                 "%s in %ldh%ldm (%s)",
                 summary, minsUntil / 60, minsUntil % 60,
                 nearestZone >= 0 ? _zones[nearestZone].name : "");
    }
}
