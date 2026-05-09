#include "WeatherService.h"
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
             "/data/2.5/weather?q=%s&appid=%s&units=metric",
             _city, _apiKey);

    WiFiSSLClient ssl;
    HttpClient    http(ssl, OWM_HOST, 443);
    http.setTimeout(8000);

    int err = http.get(path);
    if (err != 0) {
        Serial.println(F("[Weather] HTTP GET failed"));
        _available      = false;
        _comfortAllowed = !_summerFallback();
        return false;
    }

    int code = http.responseStatusCode();
    if (code != 200) {
        Serial.print(F("[Weather] HTTP status: "));
        Serial.println(code);
        http.stop();
        _available      = false;
        _comfortAllowed = !_summerFallback();
        return false;
    }

    http.skipResponseHeaders();

    // Stream JSON directly — avoids a large stack buffer and handles any response size
    StaticJsonDocument<32>  filter;
    filter["main"]["temp"] = true;
    StaticJsonDocument<64>  doc;
    DeserializationError jerr = deserializeJson(doc, http,
                                                DeserializationOption::Filter(filter));
    http.stop();
    if (jerr) {
        Serial.print(F("[Weather] JSON error: "));
        Serial.println(jerr.c_str());
        _available      = false;
        _comfortAllowed = !_summerFallback();
        return false;
    }

    _currentTemp    = doc["main"]["temp"] | 0.0f;
    _available      = true;
    _comfortAllowed = (_currentTemp <= 10.0f);

    Serial.print(F("[Weather] "));
    Serial.print(_city);
    Serial.print(F(": "));
    Serial.print(_currentTemp);
    Serial.print(F(" C — comfort "));
    Serial.println(_comfortAllowed ? F("allowed") : F("suppressed"));
    return true;
}

bool WeatherService::_summerFallback() const {
    // Summer = June (5), July (6), August (7) in tm_mon (0-based)
    time_t    now = time(nullptr);
    struct tm* t  = gmtime(&now);
    return (t->tm_mon >= 5 && t->tm_mon <= 7);
}
