#include "WeatherService.h"
#include "AppLog.h"
#include <Arduino.h>
#include <WiFiSSLClient.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

static const char OWM_HOST[] = "api.openweathermap.org";

void WeatherService::begin(const char* city, const char* apiKey) {
    strncpy(_city,   city,   sizeof(_city)   - 1);
    strncpy(_apiKey, apiKey, sizeof(_apiKey) - 1);
    _currentTemp    = 0.0f;
    _comfortAllowed = !_summerFallback();
    _available      = false;
    _lastFetchMs    = 0;
}

void WeatherService::tick() {
    if (_lastFetchMs == 0 && millis() < 30000UL) return; // let system settle first
    if (_lastFetchMs != 0 && millis() - _lastFetchMs < WEATHER_INTERVAL_MS) return;
    _fetch();
}

bool WeatherService::_fetch() {
    _lastFetchMs = millis();

    char path[128];
    snprintf(path, sizeof(path),
             "/data/2.5/forecast?q=%s&appid=%s&units=metric&cnt=16",
             _city, _apiKey);

    WiFiSSLClient ssl;
    HttpClient    http(ssl, OWM_HOST, 443);
    http.setTimeout(8000);

    int err = http.get(path);
    if (err != 0) {
        serialTs(); Serial.println(F("[Weather] HTTP GET failed"));
        _available      = false;
        _comfortAllowed = !_summerFallback();
        AppLog::add("Weather: fetch failed");
        return false;
    }

    int code = http.responseStatusCode();
    if (code != 200) {
        serialTs(); Serial.print(F("[Weather] HTTP status: "));
        Serial.println(code);
        http.stop();
        _available      = false;
        _comfortAllowed = !_summerFallback();
        AppLog::add("Weather: HTTP error");
        return false;
    }

    http.skipResponseHeaders();

    // Filter to dt + main.temp across all list entries
    StaticJsonDocument<64>    filter;
    filter["list"][0]["dt"]           = true;
    filter["list"][0]["main"]["temp"] = true;
    StaticJsonDocument<1200>  doc;
    DeserializationError jerr = deserializeJson(doc, http,
                                                DeserializationOption::Filter(filter));
    http.stop();
    if (jerr) {
        serialTs(); Serial.print(F("[Weather] JSON error: "));
        Serial.println(jerr.c_str());
        _available      = false;
        _comfortAllowed = !_summerFallback();
        AppLog::add("Weather: JSON error");
        return false;
    }

    // Determine today's date in Norway local time
    time_t    now    = time(nullptr);
    struct tm nowUtc = *gmtime(&now);
    extern int norwayOffsetSeconds(const struct tm*);
    int    off      = norwayOffsetSeconds(&nowUtc);
    time_t localNow = now + (time_t)off;
    struct tm lNow  = *gmtime(&localNow);

    // Average forecast slots that fall on today; fall back to first slot if none
    float sum      = 0.0f;
    int   count    = 0;
    float firstTemp = 0.0f;
    bool  gotFirst  = false;

    for (JsonObject entry : doc["list"].as<JsonArray>()) {
        float t = entry["main"]["temp"] | 0.0f;
        if (!gotFirst) { firstTemp = t; gotFirst = true; }

        time_t dt       = (time_t)entry["dt"].as<long>();
        struct tm dUtc  = *gmtime(&dt);
        int dOff        = norwayOffsetSeconds(&dUtc);
        time_t localDt  = dt + (time_t)dOff;
        struct tm lDt   = *gmtime(&localDt);
        bool sameDay = (lDt.tm_mday == lNow.tm_mday && lDt.tm_mon  == lNow.tm_mon
                                                    && lDt.tm_year == lNow.tm_year);
        bool daytime = (lDt.tm_hour >= 6 && lDt.tm_hour < 18);
        if (sameDay && daytime) {
            sum += t;
            count++;
        }
    }

    _currentTemp    = (count > 0) ? (sum / (float)count) : (gotFirst ? firstTemp : 0.0f);
    _available      = gotFirst;
    _comfortAllowed = (_currentTemp <= 10.0f);

    serialTs(); Serial.print(F("[Weather] "));
    Serial.print(_city);
    Serial.print(F(": daytime avg "));
    Serial.print(_currentTemp);
    Serial.print(F("C ("));
    Serial.print(count);
    Serial.print(F(" slots 06-18) — comfort "));
    Serial.println(_comfortAllowed ? F("allowed") : F("suppressed"));

    char msg[APP_LOG_WIDTH];
    snprintf(msg, sizeof(msg), "Weather: %.1fC avg (%d slots)%s",
             _currentTemp, count, _comfortAllowed ? "" : " (warm)");
    AppLog::add(msg);
    return true;
}

bool WeatherService::_summerFallback() const {
    // Summer = June (5), July (6), August (7) in tm_mon (0-based)
    time_t    now = time(nullptr);
    struct tm* t  = gmtime(&now);
    return (t->tm_mon >= 5 && t->tm_mon <= 7);
}
