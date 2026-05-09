#pragma once
#include <WiFiS3.h>
#include "Types.h"
#include "ScheduleEngine.h"
#include "WeatherService.h"

class AppWebServer {
public:
    AppWebServer(ScheduleEngine& engine, WeatherService& weather);

    void begin(const char* password);
    void tick();  // call every loop() iteration

private:
    WiFiServer    _server;
    ScheduleEngine& _engine;
    WeatherService& _weather;
    char          _password[32];
    bool          _started;

    void _handleClient(WiFiClient& client);
    void _serveDashboard(WiFiClient& client);
    void _serveStatus(WiFiClient& client);
    void _serveSettings(WiFiClient& client, const char* body, int bodyLen);
    bool _checkAuth(const char* headers);
    void _parseBody(const char* body, int len, const char* key, char* out, int outLen);

    void _sendHeader(WiFiClient& client, int code, const char* contentType);
    void _sendProgmem(WiFiClient& client, const char* pgm);
};
