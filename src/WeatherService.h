#pragma once
#include <WiFiS3.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include "Types.h"

class WeatherService {
public:
    void begin(const char* city, const char* apiKey);
    void tick();

    bool  comfortAllowed() const { return _comfortAllowed; }
    float currentTemp()    const { return _currentTemp; }
    bool  isAvailable()    const { return _available; }
    const char* city()     const { return _city; }

    void setCity(const char* city) {
        strncpy(_city, city, sizeof(_city) - 1);
        _lastFetchMs = 0;  // trigger immediate re-fetch on next tick()
    }

private:
    char     _city[32];
    char     _apiKey[48];
    float    _currentTemp;
    bool     _comfortAllowed;
    bool     _available;
    uint32_t _lastFetchMs;

    bool _fetch();
    bool _summerFallback() const;  // true if current month is June/July/August
};
