#pragma once
#include <WiFiS3.h>
#include "Types.h"
#include "ScheduleEngine.h"
#include "WeatherService.h"
#include "NoboController.h"
#include "CalendarManager.h"

class AppWebServer {
public:
    AppWebServer(ScheduleEngine& engine, WeatherService& weather,
                 NoboController& nobo, CalendarManager& cal);

    void begin(const char* password);
    void tick();

private:
    WiFiServer       _server;
    ScheduleEngine&  _engine;
    WeatherService&  _weather;
    NoboController&  _nobo;
    CalendarManager& _cal;
    char             _password[32];
    bool             _started;

    void _handleClient(WiFiClient& client);
    void _serveDashboard(WiFiClient& client, bool syncing);
    void _serveStatus(WiFiClient& client);
    void _serveSettings(WiFiClient& client, const char* body, int bodyLen);
    void _serveSync(WiFiClient& client, const char* body, int bodyLen);
    bool _checkAuth(const char* headers);
    void _parseBody(const char* body, int len, const char* key, char* out, int outLen);
    void _sendHeader(WiFiClient& client, int code, const char* contentType);
    void _sendProgmem(WiFiClient& client, const char* pgm);

    void _printZoneEvents(WiFiClient& client, int zoneIndex, bool pending);
    void _printZoneTimeline(WiFiClient& client, int zoneIndex);
};
