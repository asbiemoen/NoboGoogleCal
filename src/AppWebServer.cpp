#include "AppWebServer.h"
#include "AppLog.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// ─── PROGMEM HTML ─────────────────────────────────────────────────────────────

static const char HTML_HEAD[] PROGMEM = R"html(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>NoboGoogleCal</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;background:#0f1117;color:#e2e8f0;min-height:100vh}
header{background:#1a1f2e;border-bottom:1px solid #2d3748;padding:1rem 1.5rem;display:flex;align-items:center;justify-content:space-between}
header h1{font-size:1.1rem;font-weight:600;color:#a78bfa;letter-spacing:.05em}
.badges{display:flex;gap:.5rem;align-items:center;flex-wrap:wrap}
.badge{font-size:.72rem;padding:.25rem .6rem;border-radius:9999px;font-weight:600}
.badge-ok{background:#14532d;color:#4ade80}
.badge-warn{background:#713f12;color:#fbbf24}
.badge-info{background:#1e3a5f;color:#60a5fa}
.weather{background:#1a1f2e;border:1px solid #2d3748;border-radius:.75rem;padding:1rem 1.5rem;margin:1.5rem;display:flex;align-items:center;gap:1rem}
.weather .temp{font-size:2.5rem;font-weight:700;color:#f9a825}
.weather .info{font-size:.85rem;color:#94a3b8;line-height:1.6}
.weather .suppressed{color:#f87171;font-weight:600}
.weather .allowed{color:#4ade80;font-weight:600}
.zones{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:1rem;padding:0 1.5rem 1.5rem}
.zone{background:#1a1f2e;border:1px solid #2d3748;border-radius:.75rem;overflow:hidden}
.zone-header{padding:1rem 1.25rem;display:flex;align-items:center;justify-content:space-between}
.zone-name{font-weight:600;font-size:.95rem}
.status{font-size:.8rem;font-weight:700;padding:.35rem .75rem;border-radius:9999px}
.status-COMFORT{background:#7c2d12;color:#fb923c}
.status-ECO{background:#0c4a6e;color:#38bdf8}
.status-AWAY{background:#374151;color:#9ca3af}
.status-NORMAL{background:#374151;color:#9ca3af}
.zone-body{padding:.75rem 1.25rem 1.25rem}
.zone-next{font-size:.82rem;color:#94a3b8;margin-bottom:.75rem;line-height:1.5}
.zone-next strong{color:#e2e8f0}
.timeline{display:grid;grid-template-columns:repeat(7,1fr);gap:2px}
.day-col{display:flex;flex-direction:column;gap:2px}
.day-label{font-size:.6rem;color:#64748b;text-align:center;margin-bottom:2px}
.day-block{height:8px;border-radius:2px;background:#2d3748}
.day-block.comfort{background:#ea580c}
.day-block.active{background:#fb923c;box-shadow:0 0 4px #fb923c80}
footer{text-align:center;padding:1.5rem;color:#4a5568;font-size:.78rem}
.settings-btn{background:#4c1d95;color:#c4b5fd;border:none;padding:.5rem 1rem;border-radius:.5rem;cursor:pointer;font-size:.82rem;font-weight:600}
.settings-btn:hover{background:#5b21b6}
.modal-overlay{display:none;position:fixed;inset:0;background:#00000090;z-index:50;align-items:center;justify-content:center}
.modal-overlay.open{display:flex}
.modal{background:#1a1f2e;border:1px solid #4c1d95;border-radius:.75rem;padding:1.5rem;width:90%;max-width:400px}
.modal h2{font-size:1rem;font-weight:600;color:#a78bfa;margin-bottom:1rem}
.modal label{display:block;font-size:.8rem;color:#94a3b8;margin-bottom:.25rem;margin-top:.75rem}
.modal input{width:100%;background:#0f1117;border:1px solid #2d3748;color:#e2e8f0;padding:.5rem .75rem;border-radius:.4rem;font-size:.875rem}
.modal .actions{display:flex;gap:.75rem;margin-top:1.25rem;justify-content:flex-end}
.btn-cancel{background:#374151;color:#d1d5db;border:none;padding:.5rem 1rem;border-radius:.4rem;cursor:pointer}
.btn-save{background:#4c1d95;color:#c4b5fd;border:none;padding:.5rem 1rem;border-radius:.4rem;cursor:pointer;font-weight:600}
.nobo-warn{background:#450a0a;border:1px solid #dc2626;color:#fca5a5;padding:.6rem 1.5rem;margin:0 1.5rem 1rem;border-radius:.5rem;font-size:.8rem}
.badge-err{background:#450a0a;color:#fca5a5}
.badge-pend{background:#451a03;color:#fde68a;padding:.15rem .4rem}
.events{margin-bottom:.75rem}
.ev{display:flex;align-items:center;gap:.4rem;padding:.2rem 0;border-bottom:1px solid #1e293b}
.ev:last-child{border-bottom:none}
.ev-t{color:#64748b;font-size:.72rem;white-space:nowrap;min-width:65px}
.ev-s{color:#cbd5e1;font-size:.78rem;flex:1;min-width:0;overflow:hidden;white-space:nowrap;text-overflow:ellipsis}
.logbox{background:#0d1117;border:1px solid #1e293b;border-radius:.5rem;padding:.5rem 1rem;margin:0 1.5rem 1.5rem;font-family:monospace;font-size:.73rem;line-height:1.7}
.log-row{color:#4b5563}
.log-row:last-child{color:#9ca3af}
</style>
</head>
<body>
)html";

static const char HTML_FOOT[] PROGMEM = R"html(
<div class="modal-overlay" id="settingsModal">
<div class="modal">
<h2>Settings</h2>
<form method="POST" action="/api/settings">
<label>Password to unlock settings</label>
<input type="password" name="auth" placeholder="Enter password" required>
<label>New web password (leave blank to keep current)</label>
<input type="text" name="new_password" autocomplete="new-password">
<label>Weather city</label>
<input type="text" name="weather_city" placeholder="Oslo">
<div class="actions">
<button type="button" class="btn-cancel" onclick="document.getElementById('settingsModal').classList.remove('open')">Cancel</button>
<button type="submit" class="btn-save">Save</button>
</div>
</form>
</div>
</div>
<script>
function openSettings(){document.getElementById('settingsModal').classList.add('open')}
</script>
</body></html>
)html";

// ─── Constructor / begin ──────────────────────────────────────────────────────

AppWebServer::AppWebServer(ScheduleEngine& e, WeatherService& w,
                           NoboController& n, CalendarManager& c)
    : _server(80), _engine(e), _weather(w), _nobo(n), _cal(c), _started(false) {
    _password[0] = '\0';
}

void AppWebServer::begin(const char* pw) {
    strncpy(_password, pw, sizeof(_password) - 1);
    _server.begin();
    _started = true;
    Serial.println(F("[Web] Server started on port 80"));
}

// ─── tick ─────────────────────────────────────────────────────────────────────

void AppWebServer::tick() {
    if (!_started) return;
    WiFiClient client = _server.available();
    if (!client) return;
    _handleClient(client);
}

// ─── Request routing ──────────────────────────────────────────────────────────

void AppWebServer::_handleClient(WiFiClient& client) {
    char   reqLine[128]  = {};
    char   headers[512]  = {};
    char   body[256]     = {};
    int    bodyLen       = 0;
    int    contentLength = 0;
    bool   isPost        = false;
    uint32_t dl          = millis() + 3000UL;

    // Read request line
    int pos = 0;
    while (client.connected() && millis() < dl) {
        if (!client.available()) continue;
        char c = client.read();
        if (c == '\n') break;
        if (pos < (int)sizeof(reqLine) - 1) reqLine[pos++] = c;
    }
    reqLine[pos] = '\0';
    isPost = (strncmp(reqLine, "POST", 4) == 0);

    // Read headers (look for Content-Length and Cookie)
    pos = 0;
    char headerLine[128];
    int hPos = 0;
    while (client.connected() && millis() < dl) {
        if (!client.available()) continue;
        char c = client.read();
        if (c == '\n') {
            headerLine[hPos] = '\0';
            if (hPos <= 1) break;  // blank line = end of headers
            if (strncmp(headerLine, "Content-Length:", 15) == 0)
                contentLength = atoi(headerLine + 16);
            // Accumulate for auth check
            if (pos + hPos < (int)sizeof(headers) - 1) {
                strncpy(headers + pos, headerLine, sizeof(headers) - pos - 1);
                pos += hPos;
            }
            hPos = 0;
        } else if (hPos < (int)sizeof(headerLine) - 1) {
            headerLine[hPos++] = c;
        }
    }

    // Read body for POST
    if (isPost && contentLength > 0) {
        int toRead = contentLength < (int)sizeof(body) - 1 ? contentLength : (int)sizeof(body) - 1;
        uint32_t bdl = millis() + 2000UL;
        while (bodyLen < toRead && millis() < bdl) {
            if (client.available()) body[bodyLen++] = client.read();
        }
        body[bodyLen] = '\0';
    }

    // Route
    if (strstr(reqLine, "GET / ") || strstr(reqLine, "GET /\r")) {
        _serveDashboard(client);
    } else if (strstr(reqLine, "GET /api/status")) {
        _serveStatus(client);
    } else if (isPost && strstr(reqLine, "POST /api/settings")) {
        _serveSettings(client, body, bodyLen);
    } else {
        _sendHeader(client, 404, "text/plain");
        client.print(F("Not found"));
    }

    client.stop();
}

// ─── Dashboard ────────────────────────────────────────────────────────────────

void AppWebServer::_serveDashboard(WiFiClient& client) {
    _sendHeader(client, 200, "text/html");
    _sendProgmem(client, HTML_HEAD);

    bool noboOk = _nobo.isConnected();

    // Header
    client.print(F("<header><h1>NoboGoogleCal</h1><div class=\"badges\">"));
    client.print(F("<span class=\"badge badge-ok\">WiFi ✓</span>"));
    client.print(noboOk
        ? F("<span class=\"badge badge-ok\">Nobø: Online</span>")
        : F("<span class=\"badge badge-err\">Nobø: Offline</span>"));
    const char* lastSync = _cal.lastSyncTime();
    if (lastSync[0]) {
        client.print(F("<span class=\"badge badge-info\">Sync: "));
        client.print(lastSync);
        client.print(F("</span>"));
    }
    client.print(F("<button class=\"settings-btn\" onclick=\"openSettings()\">&#9881; Settings</button>"));
    client.print(F("</div></header>"));

    // Nobø offline banner
    if (!noboOk) {
        client.print(F("<div class=\"nobo-warn\">"));
        client.print(F("&#9888; Nobø hub not reachable &mdash; calendar events are fetched but heating is not being controlled"));
        client.print(F("</div>"));
    }

    // Weather card
    client.print(F("<div class=\"weather\">"));
    char tempBuf[16];
    dtostrf(_weather.currentTemp(), 4, 1, tempBuf);
    client.print(F("<div class=\"temp\">"));
    client.print(tempBuf);
    client.print(F(" °C</div><div class=\"info\">"));
    if (!_weather.isAvailable()) {
        client.print(F("Weather API unavailable &mdash; using seasonal fallback<br>"));
    }
    client.print(F("Comfort heating: "));
    client.print(_weather.comfortAllowed()
                 ? F("<span class=\"allowed\">Active</span>")
                 : F("<span class=\"suppressed\">Suppressed (too warm)</span>"));
    client.print(F("</div></div>"));

    // Zone cards
    client.print(F("<div class=\"zones\">"));
    for (int i = 0; i < _engine.zoneCount(); i++) {
        const char* statusStr = statusName(_engine.zoneStatus(i));
        client.print(F("<div class=\"zone\"><div class=\"zone-header\">"));
        client.print(F("<span class=\"zone-name\">"));
        client.print(_engine.zoneName(i));
        client.print(F("</span><span class=\"status status-"));
        client.print(statusStr);
        client.print(F("\">"));
        client.print(statusStr);
        client.print(F("</span></div><div class=\"zone-body\">"));
        _printZoneEvents(client, i, !noboOk);
        _printZoneTimeline(client, i);
        client.print(F("</div></div>"));
    }
    client.print(F("</div>"));

    // Activity log
    client.print(F("<div class=\"logbox\">"));
    if (AppLog::count() == 0) {
        client.print(F("<div class=\"log-row\">No activity yet</div>"));
    } else {
        for (int i = AppLog::count() - 1; i >= 0; i--) {
            client.print(F("<div class=\"log-row\">"));
            client.print(AppLog::entry(i));
            client.print(F("</div>"));
        }
    }
    client.print(F("</div>"));

    client.print(F("<footer>NoboGoogleCal &mdash; Arduino Uno R4 WiFi</footer>"));
    _sendProgmem(client, HTML_FOOT);
}

// ─── Zone event list ──────────────────────────────────────────────────────────

void AppWebServer::_printZoneEvents(WiFiClient& client, int zoneIndex, bool pending) {
    static const char* dn[7] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    time_t now = time(nullptr);
    int shown = 0;
    time_t lastStart = 0;

    client.print(F("<div class=\"events\">"));

    for (int pass = 0; pass < 3; pass++) {
        time_t best = 0;
        int    bestIdx = -1;
        for (int e = 0; e < MAX_EVENTS_PER_ZONE * MAX_ZONES; e++) {
            const CalEvent& ev = _cal.events()[e];
            if (!ev.valid || ev.zoneIndex != (uint8_t)zoneIndex) continue;
            if (ev.end <= now) continue;
            if (ev.start <= lastStart && lastStart != 0) continue;
            if (best == 0 || ev.start < best) { best = ev.start; bestIdx = e; }
        }
        if (bestIdx < 0) break;

        const CalEvent& ev = _cal.events()[bestIdx];
        lastStart = ev.start;

        struct tm utcTm = *gmtime(&ev.start);
        time_t localStart = ev.start + norwayOffsetSeconds(&utcTm);
        struct tm lTm = *gmtime(&localStart);

        char timeBuf[12];
        snprintf(timeBuf, sizeof(timeBuf), "%s %02d:%02d",
                 dn[lTm.tm_wday], lTm.tm_hour, lTm.tm_min);

        client.print(F("<div class=\"ev\"><span class=\"ev-t\">"));
        client.print(timeBuf);
        client.print(F("</span><span class=\"ev-s\">"));
        client.print(ev.summary);
        client.print(F("</span>"));
        if (pending) client.print(F("<span class=\"badge badge-pend\">Pending</span>"));
        client.print(F("</div>"));
        shown++;
    }

    if (shown == 0) {
        client.print(F("<div class=\"ev\"><span class=\"ev-t\">—</span>"));
        client.print(F("<span class=\"ev-s\">No upcoming events</span></div>"));
    }

    client.print(F("</div>"));
}

// ─── 7-day timeline ───────────────────────────────────────────────────────────

void AppWebServer::_printZoneTimeline(WiFiClient& client, int zoneIndex) {
    static const char* dn[7] = {"Su","Mo","Tu","We","Th","Fr","Sa"};
    time_t now   = time(nullptr);
    time_t today = now - (now % 86400UL);

    client.print(F("<div class=\"timeline\">"));
    for (int d = 0; d < 7; d++) {
        time_t dayStart = today + (time_t)d * 86400L;
        time_t dayEnd   = dayStart + 86400L;
        struct tm dTm = *gmtime(&dayStart);

        bool hasEvent   = false;
        bool activeNow  = false;
        for (int e = 0; e < MAX_EVENTS_PER_ZONE * MAX_ZONES; e++) {
            const CalEvent& ev = _cal.events()[e];
            if (!ev.valid || ev.zoneIndex != (uint8_t)zoneIndex) continue;
            if (ev.start < dayEnd && ev.end > dayStart) {
                hasEvent = true;
                if (ev.start <= now && ev.end > now) activeNow = true;
            }
        }

        client.print(F("<div class=\"day-col\"><div class=\"day-label\">"));
        client.print(dn[dTm.tm_wday]);
        client.print(F("</div><div class=\"day-block"));
        if (activeNow)     client.print(F(" comfort active"));
        else if (hasEvent) client.print(F(" comfort"));
        client.print(F("\"></div></div>"));
    }
    client.print(F("</div>"));
}

// ─── JSON status ──────────────────────────────────────────────────────────────

void AppWebServer::_serveStatus(WiFiClient& client) {
    _sendHeader(client, 200, "application/json");

    client.print(F("{\"status\":\""));
    client.print(_engine.statusString());
    client.print(F("\",\"nextEvent\":\""));
    client.print(_engine.nextEventString());
    client.print(F("\",\"lastSync\":\""));
    client.print(_cal.lastSyncTime());
    client.print(F("\",\"noboConnected\":"));
    client.print(_nobo.isConnected() ? F("true") : F("false"));
    client.print(F(",\"temp\":"));

    char tempBuf[10];
    dtostrf(_weather.currentTemp(), 4, 1, tempBuf);
    client.print(tempBuf);

    client.print(F(",\"comfortAllowed\":"));
    client.print(_weather.comfortAllowed() ? F("true") : F("false"));
    client.print(F(",\"weatherAvailable\":"));
    client.print(_weather.isAvailable() ? F("true") : F("false"));
    client.print(F("}"));
}

// ─── Settings POST ────────────────────────────────────────────────────────────

void AppWebServer::_serveSettings(WiFiClient& client, const char* body, int len) {
    char auth[64] = {};
    _parseBody(body, len, "auth", auth, sizeof(auth));

    if (strncmp(auth, _password, sizeof(_password)) != 0) {
        _sendHeader(client, 401, "text/plain");
        client.print(F("Unauthorized"));
        return;
    }

    char newPw[32] = {};
    _parseBody(body, len, "new_password", newPw, sizeof(newPw));
    if (strlen(newPw) > 0) {
        strncpy(_password, newPw, sizeof(_password) - 1);
        Serial.println(F("[Web] Password updated"));
    }

    char newCity[32] = {};
    _parseBody(body, len, "weather_city", newCity, sizeof(newCity));
    if (strlen(newCity) > 0) {
        _weather.setCity(newCity);
        Serial.print(F("[Web] Weather city -> "));
        Serial.println(newCity);
    }

    // Redirect back to dashboard
    client.print(F("HTTP/1.1 303 See Other\r\nLocation: /\r\nContent-Length: 0\r\n\r\n"));
}

// ─── Helpers ──────────────────────────────────────────────────────────────────

void AppWebServer::_sendHeader(WiFiClient& client, int code, const char* ct) {
    client.print(F("HTTP/1.1 "));
    client.print(code);
    client.print(F(" OK\r\nContent-Type: "));
    client.print(ct);
    client.print(F("\r\nConnection: close\r\n\r\n"));
}

void AppWebServer::_sendProgmem(WiFiClient& client, const char* pgm) {
    char buf[64];
    size_t len = strlen_P(pgm);
    for (size_t i = 0; i < len; i += sizeof(buf) - 1) {
        size_t chunk = len - i;
        if (chunk > sizeof(buf) - 1) chunk = sizeof(buf) - 1;
        memcpy_P(buf, pgm + i, chunk);
        buf[chunk] = '\0';
        client.print(buf);
    }
}

// URL-decode a form field value (handles %XX and + = space)
static void urlDecode(char* out, int outLen, const char* in) {
    int o = 0;
    for (int i = 0; in[i] && o < outLen - 1; i++) {
        if (in[i] == '+') {
            out[o++] = ' ';
        } else if (in[i] == '%' && in[i+1] && in[i+2]) {
            char hex[3] = { in[i+1], in[i+2], '\0' };
            out[o++] = (char)strtol(hex, nullptr, 16);
            i += 2;
        } else {
            out[o++] = in[i];
        }
    }
    out[o] = '\0';
}

void AppWebServer::_parseBody(const char* body, int /*len*/, const char* key, char* out, int outLen) {
    char search[48];
    snprintf(search, sizeof(search), "%s=", key);
    const char* p = strstr(body, search);
    if (!p) { out[0] = '\0'; return; }
    p += strlen(search);
    // value ends at & or end of string
    char raw[128] = {};
    int i = 0;
    while (*p && *p != '&' && i < (int)sizeof(raw) - 1) raw[i++] = *p++;
    raw[i] = '\0';
    urlDecode(out, outLen, raw);
}

bool AppWebServer::_checkAuth(const char* /*headers*/) { return false; }
