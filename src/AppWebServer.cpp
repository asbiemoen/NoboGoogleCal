// Stub — implemented in issue #7
#include "AppWebServer.h"

AppWebServer::AppWebServer(ScheduleEngine& e, WeatherService& w)
    : _server(80), _engine(e), _weather(w), _started(false) {
    _password[0] = '\0';
}

void AppWebServer::begin(const char* pw) {
    strncpy(_password, pw, sizeof(_password) - 1);
    _server.begin();
    _started = true;
}

void AppWebServer::tick()                                          {}
void AppWebServer::_handleClient(WiFiClient&)                     {}
void AppWebServer::_serveDashboard(WiFiClient&)                   {}
void AppWebServer::_serveStatus(WiFiClient&)                      {}
void AppWebServer::_serveSettings(WiFiClient&, const char*, int)  {}
bool AppWebServer::_checkAuth(const char*)                        { return false; }
void AppWebServer::_parseBody(const char*, int, const char*, char*, int) {}
void AppWebServer::_sendHeader(WiFiClient&, int, const char*)     {}
void AppWebServer::_sendProgmem(WiFiClient&, const char*)         {}
