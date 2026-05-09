#pragma once
#include <WiFiS3.h>
#include "Types.h"

#define NOBO_PORT        27779
#define NOBO_API_VERSION "1.1"

struct NoboZone {
    int  id;
    char name[32];
    int  weekProfileId;
};

struct NoboProfile {
    int  id;
    char name[32];
};

struct NoboOverride {
    int    id;
    int    mode;       // HeatingStatus value
    time_t startTime;
    time_t endTime;    // 0 = constant (no end)
    int    targetType; // 0=zone, 1=component
    int    targetId;
};

class NoboController {
public:
    bool begin(const char* ip, const char* serial);
    void tick();

    bool isConnected() const { return _connected; }

    // Profile management (called at startup to provision zone programs)
    bool ensureProfileExists(const char* name);

    // Zone discovery — matches by assigned week profile name
    int  findZoneIdByProfileName(const char* profileName);

    // Override control
    bool setZoneComfort(int zoneId, time_t from, time_t until, int& outOverrideId);
    bool clearOverride(int overrideId);
    bool clearZoneOverrides(int zoneId);

    // Accessors for web/LED display
    int  zoneCount()                    const { return _zoneCount; }
    bool getZone(int i, NoboZone& out)  const;

private:
    WiFiClient _client;
    char       _ip[16];
    char       _serial[13];
    bool       _connected;
    uint32_t   _lastKeepAlive;
    uint32_t   _lastReconnectAttempt;
    int        _nextOverrideId;

    NoboZone     _zones[MAX_NOBO_ZONES];
    NoboProfile  _profiles[MAX_NOBO_PROFILES];
    NoboOverride _overrides[MAX_NOBO_OVERRIDES];
    int          _zoneCount;
    int          _profileCount;
    int          _overrideCount;

    bool _connect();
    void _disconnect();
    bool _send(const char* cmd);
    bool _readLine(char* buf, size_t len, uint32_t timeoutMs = 5000);
    bool _readAllData();
    void _parseLine(const char* line);
    void _parseH01(const char* data);
    void _parseH03(const char* data);
    void _parseH04(const char* data);
    int  _findProfileByName(const char* name) const;
};
