#pragma once
#include <WiFiS3.h>
#include "Types.h"
#include "NVMConfig.h"
#include "ScheduleEngine.h"
#include "WeatherService.h"
#include "NoboController.h"
#include "CalendarManager.h"

class AppWebServer {
public:
    AppWebServer(ScheduleEngine& engine, WeatherService& weather,
                 NoboController& nobo, CalendarManager& cal);

    void begin(const char* password, NVMConfig& nvm);
    void tick();
    bool rebootPending() const { return _rebootPending; }

private:
    WiFiServer       _server;
    ScheduleEngine&  _engine;
    WeatherService&  _weather;
    NoboController&  _nobo;
    CalendarManager& _cal;
    NVMConfig*       _nvm;
    char             _password[32];
    char             _sessionToken[17];
    bool             _reqAuth;
    bool             _started;
    bool             _rebootPending;

    void _handleClient(WiFiClient& client);
    void _serveDashboard(WiFiClient& client, bool syncing);
    void _serveLoginPage(WiFiClient& client, bool err);
    void _serveSettingsPage(WiFiClient& client);
    void _serveStatus(WiFiClient& client);
    void _serveSettings(WiFiClient& client, const char* body, int bodyLen);
    void _serveSync(WiFiClient& client, const char* body, int bodyLen);
    void _serveLogin(WiFiClient& client, const char* body, int bodyLen);
    void _serveLogout(WiFiClient& client);
    void _serveOverride(WiFiClient& client, const char* body, int bodyLen);
    bool _checkAuth(const char* headers);
    void _generateToken();
    void _parseBody(const char* body, int len, const char* key, char* out, int outLen);
    void _sendHeader(WiFiClient& client, int code, const char* contentType);
    void _sendProgmem(WiFiClient& client, const char* pgm);
    void _printZoneEvents(WiFiClient& client, int zoneIndex, bool pending);
    void _printZoneTimeline(WiFiClient& client, int zoneIndex);
};
