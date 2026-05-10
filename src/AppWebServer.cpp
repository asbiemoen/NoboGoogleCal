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
<meta http-equiv="refresh" content="60">
<meta name="theme-color" content="#0f1117">
<title>Heating Controller</title>
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
.weather .temp-na{font-size:2.5rem;font-weight:700;color:#4b5563}
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
.status-AWAY{background:#1e3a5f;color:#60a5fa}
.status-NORMAL{background:#374151;color:#9ca3af}
.zone-body{padding:.75rem 1.25rem 1.25rem}
.zone-next{font-size:.82rem;color:#94a3b8;margin-bottom:.75rem;line-height:1.5}
.zone-next strong{color:#e2e8f0}
.timeline-wrap{overflow-x:auto}
.timeline{display:grid;grid-template-columns:repeat(7,1fr);gap:2px;min-width:336px}
.day-col{display:flex;flex-direction:column;gap:2px}
.day-label{font-size:.6rem;color:#64748b;text-align:center;margin-bottom:2px}
.day-hours{display:flex;height:8px;border-radius:2px;overflow:hidden}
.h-eco{flex:1;background:#15803d}
.h-comfort{flex:1;background:#dc2626}
.h-active{flex:1;background:#ef4444;box-shadow:inset 0 0 3px rgba(255,255,255,.25)}
.h-preheat{flex:1;background:#f59e0b}
.h-preheat-active{flex:1;background:#fbbf24;box-shadow:inset 0 0 3px rgba(255,255,255,.25)}
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
.nobo-warn{background:#450a0a;border-left:4px solid #dc2626;color:#fca5a5;padding:.6rem 1.5rem;font-size:.8rem}
.syncing-banner{background:#052e16;border-left:4px solid #4ade80;color:#4ade80;padding:.5rem 1.5rem;font-size:.8rem}
.badge-err{background:#450a0a;color:#fca5a5}
.badge-pend{background:#451a03;color:#fde68a;padding:.15rem .4rem}
.events{margin-bottom:.75rem}
.ev{display:flex;align-items:center;gap:.4rem;padding:.2rem 0;border-bottom:1px solid #1e293b}
.ev:last-child{border-bottom:none}
.ev-next{border-left:3px solid #f59e0b;padding-left:.35rem}
.ev-t{color:#64748b;font-size:.72rem;white-space:nowrap;min-width:100px}
.ev-s{color:#cbd5e1;font-size:.78rem;flex:1;min-width:0;overflow:hidden;white-space:nowrap;text-overflow:ellipsis}
.logbox{background:#0d1117;border:1px solid #1e293b;border-radius:.5rem;padding:.5rem 1rem;margin:0 1.5rem 1.5rem;font-family:monospace;font-size:.73rem;line-height:1.7}
.log-row{color:#4b5563}
.log-row:first-child{color:#9ca3af}
.log-row-err{color:#f87171!important}
.hero{background:#1a1f2e;border:1px solid #4c1d95;border-radius:.75rem;padding:.85rem 1.5rem;margin:1.5rem}
.hero-row{display:flex;align-items:center;gap:.75rem;padding:.15rem 0}
.hero-zone-name{font-weight:600;font-size:.88rem;color:#e2e8f0;min-width:160px;overflow:hidden;white-space:nowrap;text-overflow:ellipsis}
.hero-next{font-size:.78rem;color:#a78bfa;flex:1}
.hero-countdown{display:inline-block;margin-top:.5rem;font-size:.78rem;font-weight:600;color:#fb923c;background:#451a03;border-radius:.4rem;padding:.25rem .6rem}
.zone-warn{font-size:.73rem;color:#f87171;padding:.2rem 1.25rem;background:#450a0a}
.section-title{font-size:.72rem;font-weight:700;color:#4b5563;text-transform:uppercase;letter-spacing:.08em;padding:0 1.5rem .4rem}
@media(max-width:480px){header{flex-wrap:wrap;gap:.5rem}}
@media(max-width:400px){.ev-t{min-width:80px}.hero-zone-name{min-width:120px}}
@keyframes shake{0%,100%{transform:translateX(0)}20%,60%{transform:translateX(-4px)}40%,80%{transform:translateX(4px)}}.shake{animation:shake .35s ease}
.hero-override{padding:.1rem 1.25rem .6rem;gap:.5rem;flex-wrap:wrap}
.btn-ov{border:none;padding:.3rem .9rem;border-radius:9999px;font-size:.78rem;font-weight:600;cursor:pointer}
.btn-ov-boost{background:#15803d;color:#fff}
.btn-ov-mute{background:#dc2626;color:#fff}
.btn-ov-cancel{background:#f59e0b;color:#000}
</style>
</head>
<body>
)html";

static const char HTML_FOOT_SCRIPT[] PROGMEM = R"html(
<script>
var _pw=sessionStorage.getItem('nbc_pw')||'';
function $(i){return document.getElementById(i);}
function _showOv(show){document.querySelectorAll('.ov-btns').forEach(function(el){el.style.display=show?'flex':'none';});}
if(_pw){$('loginBtn').style.display='none';$('settingsBtn').style.display='';_showOv(true);}
function openLogin(){$('settingsView').style.display='none';$('loginView').style.display='';$('loginInput').value='';$('loginErr').style.display='none';$('mainModal').classList.add('open');}
function openSettings(){$('loginView').style.display='none';$('settingsView').style.display='';$('authInput').value=_pw;$('mainModal').classList.add('open');}
function closeModal(){$('mainModal').classList.remove('open');}
function doLogin(e){e.preventDefault();var p=$('loginInput').value;if(!p)return;fetch('/login',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'auth='+encodeURIComponent(p)}).then(function(r){if(r.ok){_pw=p;sessionStorage.setItem('nbc_pw',p);closeModal();$('loginBtn').style.display='none';$('settingsBtn').style.display='';_showOv(true);}else{$('loginErr').style.display='block';var inp=$('loginInput');inp.classList.add('shake');setTimeout(function(){inp.classList.remove('shake');},400);}});}
function doLogout(){sessionStorage.removeItem('nbc_pw');_pw='';$('settingsBtn').style.display='none';$('loginBtn').style.display='';_showOv(false);closeModal();}
function saveSettings(){var np=$('newPwInput').value;if(np){_pw=np;sessionStorage.setItem('nbc_pw',np);}$('authInput').value=_pw;$('settingsForm').action='/api/settings';$('settingsForm').submit();}
function doSync(){if(!_pw)return;$('pwHidden').value=_pw;$('settingsForm').action='/sync';$('settingsForm').submit();}
function doOverride(z,a){var h;if(a<0){h=window.prompt('Hours (e.g. 2 or 0.5):','2');if(h===null)return;h=parseFloat(h)||2;}else{h=a;}fetch('/api/override',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'auth='+encodeURIComponent(_pw)+'&zone='+z+'&hours='+h}).then(function(r){if(r.ok)location.reload();});}
</script>
</body></html>
)html";

// ─── Constructor / begin ──────────────────────────────────────────────────────

AppWebServer::AppWebServer(ScheduleEngine& e, WeatherService& w,
                           NoboController& n, CalendarManager& c)
    : _server(80), _engine(e), _weather(w), _nobo(n), _cal(c),
      _nvm(nullptr), _started(false), _rebootPending(false) {
    _password[0] = '\0';
}

void AppWebServer::begin(const char* pw, NVMConfig& nvm) {
    _nvm = &nvm;
    strncpy(_password, pw, sizeof(_password) - 1);
    _server.begin();
    _started = true;
    Serial.println(F("[Web] Server started on port 80"));
}

// ─── tick ─────────────────────────────────────────────────────────────────────

void AppWebServer::tick() {
    if (!_started) return;
    if (_rebootPending) {
        Serial.println(F("[Web] Rebooting to apply settings..."));
        delay(500);
        NVIC_SystemReset();
    }
    WiFiClient client = _server.available();
    if (!client) return;
    _handleClient(client);
}

// ─── Request routing ──────────────────────────────────────────────────────────

void AppWebServer::_handleClient(WiFiClient& client) {
    char   reqLine[128]  = {};
    char   headers[512]  = {};
    char   body[512]     = {};
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

    // Read headers (look for Content-Length)
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
    bool isDashboard = !isPost &&
        (strstr(reqLine, "GET / ") || strstr(reqLine, "GET /\r") || strstr(reqLine, "GET /?"));
    if (isDashboard) {
        bool syncing = strstr(reqLine, "syncing=1") != nullptr;
        _serveDashboard(client, syncing);
    } else if (strstr(reqLine, "GET /api/status")) {
        _serveStatus(client);
    } else if (isPost && strstr(reqLine, "POST /api/settings")) {
        _serveSettings(client, body, bodyLen);
    } else if (isPost && strstr(reqLine, "POST /sync")) {
        _serveSync(client, body, bodyLen);
    } else if (isPost && strstr(reqLine, "POST /login")) {
        _serveLogin(client, body, bodyLen);
    } else if (isPost && strstr(reqLine, "POST /api/override")) {
        _serveOverride(client, body, bodyLen);
    } else {
        _sendHeader(client, 404, "text/plain");
        client.print(F("Not found"));
    }

    client.stop();
}

// ─── Dashboard ────────────────────────────────────────────────────────────────

void AppWebServer::_serveDashboard(WiFiClient& client, bool syncing) {
    _sendHeader(client, 200, "text/html");
    _sendProgmem(client, HTML_HEAD);

    bool noboOk = _nobo.isConnected();

    // 1. Offline banner — topmost, above header
    if (!noboOk) {
        client.print(F("<div class=\"nobo-warn\">"));
        client.print(F("&#9888; Nob&oslash; hub not reachable &mdash; calendar events are fetched but heating is not being controlled"));
        client.print(F("</div>"));
    }

    // 2. Sync-in-progress banner
    if (syncing) {
        client.print(F("<div class=\"syncing-banner\">&#10003; Sync in progress &mdash; calendar will update shortly</div>"));
    }

    // Current board time (local)
    time_t now = time(nullptr);
    struct tm nowUtc = *gmtime(&now);
    int localOff = norwayOffsetSeconds(&nowUtc);
    time_t localNow = now + (time_t)localOff;
    struct tm lNow = *gmtime(&localNow);
    char boardTimeBuf[32];
    snprintf(boardTimeBuf, sizeof(boardTimeBuf), "%04d-%02d-%02d %02d:%02d:%02d UTC+%d",
             lNow.tm_year+1900, lNow.tm_mon+1, lNow.tm_mday,
             lNow.tm_hour, lNow.tm_min, lNow.tm_sec, localOff/3600);

    // 3. Header
    client.print(F("<header><h1>Heating Controller</h1><div class=\"badges\">"));
    client.print(F("<span class=\"badge badge-ok\">WiFi &#10003;</span>"));
    client.print(noboOk
        ? F("<span class=\"badge badge-ok\">Nob&oslash;: Online</span>")
        : F("<span class=\"badge badge-err\">Nob&oslash;: Offline</span>"));
    client.print(F("<span class=\"badge badge-info\">"));
    client.print(boardTimeBuf);
    client.print(F("</span>"));
    const char* lastSync = _cal.lastSyncTime();
    if (lastSync[0]) {
        client.print(F("<span class=\"badge badge-info\">Updated: "));
        client.print(lastSync);
        client.print(F("</span>"));
    }
    {
        IPAddress ip = WiFi.localIP();
        char ipBuf[16];
        snprintf(ipBuf, sizeof(ipBuf), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
        client.print(F("<span class=\"badge badge-info\">"));
        client.print(ipBuf);
        client.print(F("</span>"));
    }
    client.print(F("<button class=\"settings-btn\" id=\"loginBtn\" onclick=\"openLogin()\">&#128274; Login</button>"));
    client.print(F("<button class=\"settings-btn\" id=\"settingsBtn\" style=\"display:none\" onclick=\"openSettings()\">&#9881; Settings</button>"));
    client.print(F("</div></header>"));

    // 4. Hero — one row per zone + optional countdown badge
    client.print(F("<div class=\"hero\">"));

    // Find the soonest upcoming zone change within 30 minutes
    time_t soonestChange = 0;
    bool   soonestToComfort = false;
    for (int i = 0; i < _engine.zoneCount(); i++) {
        time_t changeAt; bool toComfort;
        if (_engine.nextChangeForZone(i, 1800, changeAt, toComfort)) {
            if (soonestChange == 0 || changeAt < soonestChange) {
                soonestChange    = changeAt;
                soonestToComfort = toComfort;
            }
        }
    }

    for (int i = 0; i < _engine.zoneCount(); i++) {
        const char* statusStr = statusName(_engine.zoneStatus(i));
        client.print(F("<div class=\"hero-row\">"));
        client.print(F("<span class=\"hero-zone-name\">"));
        client.print(_engine.zoneName(i));
        client.print(F("</span><span class=\"status status-"));
        client.print(statusStr);
        client.print(F("\">"));
        client.print(statusStr);
        client.print(F("</span></div>"));
        client.print(F("<div class=\"hero-override ov-btns\" style=\"display:none\">"));
        bool ovActive  = _engine.overrideActive(i);
        bool isComfort = (_engine.zoneStatus(i) == STATUS_COMFORT);
        if (ovActive) {
            char btnBuf[72];
            snprintf(btnBuf, sizeof(btnBuf),
                "<button class=\"btn-ov btn-ov-cancel\" onclick=\"doOverride(%d,0)\">Cancel override</button>", i);
            client.print(btnBuf);
        }
        {
            char btnBuf[80];
            if (isComfort) {
                snprintf(btnBuf, sizeof(btnBuf),
                    "<button class=\"btn-ov btn-ov-mute\" onclick=\"doOverride(%d,-1)\">Mute</button>", i);
            } else {
                snprintf(btnBuf, sizeof(btnBuf),
                    "<button class=\"btn-ov btn-ov-boost\" onclick=\"doOverride(%d,-1)\">Boost</button>", i);
            }
            client.print(btnBuf);
        }
        client.print(F("</div>"));
    }

    if (soonestChange > 0) {
        int minsLeft = (int)((soonestChange - now) / 60);
        if (minsLeft < 1) minsLeft = 1;
        char countBuf[48];
        snprintf(countBuf, sizeof(countBuf),
                 soonestToComfort ? "&#9889; Heating starts in %d min" : "&#9889; Heating ends in %d min",
                 minsLeft);
        client.print(F("<div class=\"hero-countdown\">"));
        client.print(countBuf);
        client.print(F("</div>"));
    }

    client.print(F("</div>"));

    // 5. Weather card
    client.print(F("<div class=\"weather\">"));
    bool weatherOk = _weather.isAvailable();
    if (weatherOk) {
        char tempBuf[16];
        dtostrf(_weather.currentTemp(), 4, 1, tempBuf);
        client.print(F("<div class=\"temp\">"));
        client.print(tempBuf);
        client.print(F(" &deg;C</div>"));
    } else {
        client.print(F("<div class=\"temp-na\">-- &deg;C</div>"));
    }
    client.print(F("<div class=\"info\">"));
    client.print(_weather.city());
    client.print(F("<br>"));
    if (!weatherOk) {
        client.print(F("Weather data unavailable &mdash; using seasonal fallback<br>"));
    }
    client.print(F("Comfort heating: "));
    client.print(_weather.comfortAllowed()
                 ? F("<span class=\"allowed\">Active</span>")
                 : F("<span class=\"suppressed\">Suppressed (too warm)</span>"));
    client.print(F("</div></div>"));

    // 7. Zone cards
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
        client.print(F("</span></div>"));
        if (noboOk && _engine.zoneNoboId(i) < 0) {
            client.print(F("<div class=\"zone-warn\">&#9888; Zone not found in Nob&oslash; hub</div>"));
        }
        client.print(F("<div class=\"zone-body\">"));
        _printZoneEvents(client, i, !noboOk);
        _printZoneTimeline(client, i);
        client.print(F("</div></div>"));
    }
    client.print(F("</div>"));

    // 8. Activity log
    client.print(F("<p class=\"section-title\">Activity log</p>"));
    client.print(F("<div class=\"logbox\">"));
    if (AppLog::count() == 0) {
        client.print(F("<div class=\"log-row\">No activity yet</div>"));
    } else {
        for (int i = AppLog::count() - 1; i >= 0; i--) {
            const char* e = AppLog::entry(i);
            bool isErr = strstr(e, "failed") || strstr(e, "timeout") || strstr(e, "TCP err");
            client.print(isErr ? F("<div class=\"log-row log-row-err\">")
                               : F("<div class=\"log-row\">"));
            client.print(e);
            client.print(F("</div>"));
        }
    }
    client.print(F("</div>"));

    client.print(F("<footer>Heating Controller &mdash; Arduino Uno R4 WiFi</footer>"));
    _serveSettingsModal(client);
    _sendProgmem(client, HTML_FOOT_SCRIPT);
}

// ─── Zone event list ──────────────────────────────────────────────────────────

void AppWebServer::_printZoneEvents(WiFiClient& client, int zoneIndex, bool pending) {
    time_t  now      = time(nullptr);
    int     shown    = 0;
    time_t  lastStart = 0;
    uint8_t preheatH = _engine.zonePreheatHours(zoneIndex);

    client.print(F("<div class=\"events\">"));

    for (int pass = 0; pass < 3; pass++) {
        time_t best    = 0;
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

        struct tm utcTm    = *gmtime(&ev.start);
        time_t localStart  = ev.start + norwayOffsetSeconds(&utcTm);
        struct tm lTm      = *gmtime(&localStart);

        struct tm utcEndTm = *gmtime(&ev.end);
        time_t localEnd    = ev.end + norwayOffsetSeconds(&utcEndTm);
        struct tm lEndTm   = *gmtime(&localEnd);

        char timeBuf[18];
        snprintf(timeBuf, sizeof(timeBuf), "%04d-%02d-%02d %02d:%02d",
                 lTm.tm_year + 1900, lTm.tm_mon + 1, lTm.tm_mday,
                 lTm.tm_hour, lTm.tm_min);
        char endBuf[6];
        snprintf(endBuf, sizeof(endBuf), "%02d:%02d", lEndTm.tm_hour, lEndTm.tm_min);

        const char* label = (strcmp(ev.summary, "Busy") == 0)
                            ? _engine.zoneEventLabel(zoneIndex)
                            : ev.summary;

        bool isFirst = (shown == 0);
        client.print(isFirst ? F("<div class=\"ev ev-next\">") : F("<div class=\"ev\">"));
        client.print(F("<span class=\"ev-t\">"));
        client.print(timeBuf);
        client.print(F("&ndash;"));
        client.print(endBuf);
        if (preheatH > 0) {
            client.print(F("&nbsp;<span class=\"badge badge-warn\" style=\"font-size:.62rem;padding:.1rem .3rem\">(+"));
            client.print(preheatH);
            client.print(F("h warmup)</span>"));
        }
        client.print(F("</span><span class=\"ev-s\">"));
        client.print(label);
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
    time_t now = time(nullptr);
    struct tm nowUtcTm = *gmtime(&now);
    int localOff = norwayOffsetSeconds(&nowUtcTm);
    time_t localNow = now + (time_t)localOff;
    time_t today = (localNow - (localNow % 86400UL)) - (time_t)localOff;

    uint8_t preheatH = _engine.zonePreheatHours(zoneIndex);

    client.print(F("<div class=\"timeline-wrap\"><div class=\"timeline\">"));
    for (int d = 0; d < 7; d++) {
        time_t dayStart      = today + (time_t)d * 86400L;
        time_t localDayStart = dayStart + (time_t)localOff;
        struct tm dTm        = *gmtime(&localDayStart);

        char dateBuf[6];
        snprintf(dateBuf, sizeof(dateBuf), "%02d.%02d", dTm.tm_mday, dTm.tm_mon + 1);
        client.print(F("<div class=\"day-col\"><div class=\"day-label\">"));
        client.print(dn[dTm.tm_wday]);
        client.print(F("<br>"));
        client.print(dateBuf);
        client.print(F("</div><div class=\"day-hours\">"));

        for (int h = 0; h < 24; h++) {
            time_t hStart  = dayStart + (time_t)h * 3600L;
            time_t hEnd    = hStart + 3600L;
            bool   isNow   = (now >= hStart && now < hEnd);
            bool   inEvent   = false;
            bool   inPreheat = false;

            for (int e = 0; e < MAX_EVENTS_PER_ZONE * MAX_ZONES; e++) {
                const CalEvent& ev = _cal.events()[e];
                if (!ev.valid || ev.zoneIndex != (uint8_t)zoneIndex) continue;
                if (ev.start < hEnd && ev.end > hStart) {
                    inEvent = true;
                } else if (preheatH > 0) {
                    time_t phFrom = ev.start - (time_t)preheatH * 3600L;
                    if (phFrom < hEnd && ev.start > hStart) inPreheat = true;
                }
            }

            if      (inEvent   && isNow) client.print(F("<div class=\"h-active\"></div>"));
            else if (inEvent)             client.print(F("<div class=\"h-comfort\"></div>"));
            else if (inPreheat && isNow) client.print(F("<div class=\"h-preheat-active\"></div>"));
            else if (inPreheat)           client.print(F("<div class=\"h-preheat\"></div>"));
            else                          client.print(F("<div class=\"h-eco\"></div>"));
        }

        client.print(F("</div></div>"));
    }
    client.print(F("</div></div>"));
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

    bool reboot = false;

    // Helper: parse field, update dst if non-empty and changed; set reboot if net field
    auto upd = [&](char* dst, int dstLen, const char* key, bool net) {
        char tmp[64] = {};
        _parseBody(body, len, key, tmp, (int)sizeof(tmp));
        if (!tmp[0]) return;
        if (strcmp(dst, tmp) != 0) {
            strncpy(dst, tmp, dstLen - 1);
            dst[dstLen - 1] = '\0';
            if (net) reboot = true;
        }
    };

    // Account
    char newPw[32] = {};
    _parseBody(body, len, "new_password", newPw, sizeof(newPw));
    if (newPw[0]) {
        strncpy(_password, newPw, sizeof(_password) - 1);
        if (_nvm) strncpy(_nvm->webPassword, newPw, sizeof(_nvm->webPassword) - 1);
        Serial.println(F("[Web] Password updated"));
    }

    if (_nvm) {
        // Network — changes require reboot
        upd(_nvm->wifiSsid,    sizeof(_nvm->wifiSsid),   "wifi_ssid",   true);
        upd(_nvm->wifiPass,    sizeof(_nvm->wifiPass),   "wifi_pass",   true);
        upd(_nvm->wifiSsid2,   sizeof(_nvm->wifiSsid2),  "wifi2_ssid",  true);
        upd(_nvm->wifiPass2,   sizeof(_nvm->wifiPass2),  "wifi2_pass",  true);
        upd(_nvm->noboIp,      sizeof(_nvm->noboIp),     "nobo_ip",     true);
        upd(_nvm->noboSerial,  sizeof(_nvm->noboSerial), "nobo_serial", true);

        // Weather city — instant apply
        char newCity[32] = {};
        _parseBody(body, len, "weather_city", newCity, sizeof(newCity));
        if (newCity[0]) {
            _weather.setCity(newCity);
            strncpy(_nvm->weatherCity, newCity, sizeof(_nvm->weatherCity) - 1);
            Serial.print(F("[Web] Weather city -> "));
            Serial.println(newCity);
        }

        // Notifications
        _nvm->emailEnabled = (strstr(body, "email_enabled=on") != nullptr);
        upd(_nvm->emailTime, sizeof(_nvm->emailTime), "email_time", false);

        char emailTo[64] = {};
        _parseBody(body, len, "email_to", emailTo, sizeof(emailTo));
        strncpy(_nvm->resendTo, emailTo, sizeof(_nvm->resendTo) - 1);

        upd(_nvm->resendKey,  sizeof(_nvm->resendKey),  "resend_key",  false);

        char resendFrom[64] = {};
        _parseBody(body, len, "resend_from", resendFrom, sizeof(resendFrom));
        strncpy(_nvm->resendFrom, resendFrom, sizeof(_nvm->resendFrom) - 1);

        nvmSave(*_nvm);
    }

    if (reboot) _rebootPending = true;
    client.print(F("HTTP/1.1 303 See Other\r\nLocation: /\r\nContent-Length: 0\r\n\r\n"));
}

// ─── Settings modal (dynamic — pre-fills current NVM values) ─────────────────

void AppWebServer::_serveSettingsModal(WiFiClient& client) {
    client.print(F("<div class=\"modal-overlay\" id=\"mainModal\"><div class=\"modal\">"));

    // Login view
    client.print(F("<div id=\"loginView\"><h2>Login</h2>"));
    client.print(F("<form onsubmit=\"doLogin(event)\"><label>Password</label>"));
    client.print(F("<input type=\"password\" id=\"loginInput\" placeholder=\"Enter password\" autocomplete=\"current-password\">"));
    client.print(F("<span id=\"loginErr\" style=\"display:none;color:#f87171;font-size:.75rem;margin-top:.4rem\">Wrong password</span>"));
    client.print(F("<div class=\"actions\">"));
    client.print(F("<button type=\"button\" class=\"btn-cancel\" onclick=\"closeModal()\">Cancel</button>"));
    client.print(F("<button type=\"submit\" class=\"btn-save\">Login</button>"));
    client.print(F("</div></form></div>"));

    // Settings view
    client.print(F("<div id=\"settingsView\" style=\"display:none\">"));
    client.print(F("<h2>Settings <button type=\"button\" class=\"btn-cancel\" style=\"float:right;padding:.25rem .5rem\" onclick=\"doLogout()\">Lock</button></h2>"));
    client.print(F("<form id=\"settingsForm\" method=\"POST\" action=\"/api/settings\">"));
    client.print(F("<input type=\"hidden\" name=\"pw\" id=\"pwHidden\">"));
    client.print(F("<input type=\"hidden\" name=\"auth\" id=\"authInput\">"));

    // Account
    client.print(F("<p class=\"section-title\" style=\"margin-top:.5rem\">Account</p>"));
    client.print(F("<label>New password (leave blank to keep current)</label>"));
    client.print(F("<input type=\"password\" id=\"newPwInput\" name=\"new_password\" autocomplete=\"new-password\">"));

    // Network
    client.print(F("<p class=\"section-title\" style=\"margin-top:.75rem\">Network</p>"));
    client.print(F("<label>WiFi SSID</label><input type=\"text\" name=\"wifi_ssid\" value=\""));
    if (_nvm) client.print(_nvm->wifiSsid);
    client.print(F("\" placeholder=\"(unchanged)\">"));
    client.print(F("<label>WiFi password</label>"));
    client.print(F("<input type=\"password\" name=\"wifi_pass\" autocomplete=\"new-password\" placeholder=\"(unchanged)\">"));
    client.print(F("<label>Nob&oslash; hub IP</label><input type=\"text\" name=\"nobo_ip\" value=\""));
    if (_nvm) client.print(_nvm->noboIp);
    client.print(F("\" placeholder=\"192.168.x.x\">"));
    client.print(F("<label>Nob&oslash; serial</label><input type=\"text\" name=\"nobo_serial\" value=\""));
    if (_nvm) client.print(_nvm->noboSerial);
    client.print(F("\" placeholder=\"123456789012\">"));
    client.print(F("<label>Weather city</label><input type=\"text\" name=\"weather_city\" value=\""));
    if (_nvm) client.print(_nvm->weatherCity);
    client.print(F("\" placeholder=\"Oslo\">"));

    bool hasSec = _nvm && _nvm->wifiSsid2[0];
    if (!hasSec) {
        client.print(F("<a href=\"#\" id=\"wifiSecLink\" style=\"font-size:.78rem;color:#a78bfa;display:block;margin-top:.5rem\""));
        client.print(F(" onclick=\"$('wifiSec').style.display='block';this.style.display='none';return false\">+ Add secondary network</a>"));
    }
    client.print(hasSec ? F("<div id=\"wifiSec\">") : F("<div id=\"wifiSec\" style=\"display:none\">"));
    client.print(F("<label>Secondary SSID</label><input type=\"text\" name=\"wifi2_ssid\" value=\""));
    if (_nvm) client.print(_nvm->wifiSsid2);
    client.print(F("\"><label>Secondary password</label>"));
    client.print(F("<input type=\"password\" name=\"wifi2_pass\" autocomplete=\"new-password\" placeholder=\"(unchanged)\"></div>"));
    client.print(F("<p style=\"font-size:.72rem;color:#94a3b8;margin-top:.5rem\">&#8505; WiFi and Nob&oslash; changes take effect after reboot.</p>"));

    // Notifications
    client.print(F("<p class=\"section-title\" style=\"margin-top:.75rem\">Notifications</p>"));
    bool emailOn = _nvm && _nvm->emailEnabled;
    client.print(F("<label><input type=\"checkbox\" name=\"email_enabled\" id=\"emailToggle\""));
    if (emailOn) client.print(F(" checked"));
    client.print(F(" onchange=\"$('emailSub').style.display=this.checked?'block':'none'\"> Daily summary email</label>"));
    client.print(emailOn ? F("<div id=\"emailSub\">") : F("<div id=\"emailSub\" style=\"display:none\">"));
    client.print(F("<label>Send at</label><input type=\"time\" name=\"email_time\" value=\""));
    client.print((_nvm && _nvm->emailTime[0]) ? _nvm->emailTime : "07:00");
    client.print(F("\"><label>Recipient</label><input type=\"email\" name=\"email_to\" value=\""));
    if (_nvm) client.print(_nvm->resendTo);
    client.print(F("\" placeholder=\"you@example.com\"></div>"));
    client.print(F("<label>Resend API key</label>"));
    client.print(F("<input type=\"password\" name=\"resend_key\" autocomplete=\"off\" placeholder=\""));
    client.print((_nvm && _nvm->resendKey[0]) ? F("(set — leave blank to keep)") : F("re_..."));
    client.print(F("\"><label>Resend from address</label>"));
    client.print(F("<input type=\"email\" name=\"resend_from\" value=\""));
    if (_nvm) client.print(_nvm->resendFrom);
    client.print(F("\" placeholder=\"noreply@example.com\">"));

    client.print(F("<div class=\"actions\">"));
    client.print(F("<button type=\"button\" class=\"btn-cancel\" onclick=\"closeModal()\">Cancel</button>"));
    client.print(F("<button type=\"button\" class=\"btn-cancel\" onclick=\"doSync()\">&#x21bb; Sync now</button>"));
    client.print(F("<button type=\"button\" class=\"btn-save\" onclick=\"saveSettings()\">Save</button>"));
    client.print(F("</div></form></div></div></div>"));
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
    char raw[128] = {};
    int i = 0;
    while (*p && *p != '&' && i < (int)sizeof(raw) - 1) raw[i++] = *p++;
    raw[i] = '\0';
    urlDecode(out, outLen, raw);
}

// ─── Manual sync POST ─────────────────────────────────────────────────────────

void AppWebServer::_serveSync(WiFiClient& client, const char* body, int len) {
    char pw[64] = {};
    _parseBody(body, len, "pw", pw, sizeof(pw));

    if (strncmp(pw, _password, sizeof(_password)) != 0) {
        _sendHeader(client, 401, "text/html");
        client.print(F("<html><body style='background:#0f1117;color:#f87171;font-family:monospace;padding:2rem'>"));
        client.print(F("Wrong password. <a href='/' style='color:#a78bfa'>Back</a></body></html>"));
        return;
    }

    // Redirect first — browser navigates while sync runs in background
    client.print(F("HTTP/1.1 303 See Other\r\nLocation: /?syncing=1\r\nContent-Length: 0\r\n\r\n"));
    client.flush();
    _cal.forceSyncAll();
}

// ─── Login POST ───────────────────────────────────────────────────────────────

void AppWebServer::_serveLogin(WiFiClient& client, const char* body, int len) {
    char auth[64] = {};
    _parseBody(body, len, "auth", auth, sizeof(auth));
    if (strncmp(auth, _password, sizeof(_password)) == 0) {
        _sendHeader(client, 200, "text/plain");
        client.print(F("OK"));
    } else {
        _sendHeader(client, 401, "text/plain");
        client.print(F("Unauthorized"));
    }
}

// ─── Override POST ────────────────────────────────────────────────────────────

void AppWebServer::_serveOverride(WiFiClient& client, const char* body, int len) {
    char auth[64] = {};
    _parseBody(body, len, "auth", auth, sizeof(auth));
    if (strncmp(auth, _password, sizeof(_password)) != 0) {
        _sendHeader(client, 401, "text/plain");
        client.print(F("Unauthorized"));
        return;
    }
    char zoneBuf[8]  = {};
    char hoursBuf[8] = {};
    _parseBody(body, len, "zone",  zoneBuf,  sizeof(zoneBuf));
    _parseBody(body, len, "hours", hoursBuf, sizeof(hoursBuf));
    int   zone  = atoi(zoneBuf);
    float hours = (float)atof(hoursBuf);
    _engine.setOverride(zone, hours);
    _sendHeader(client, 200, "text/plain");
    client.print(F("OK"));
}

bool AppWebServer::_checkAuth(const char* /*headers*/) { return false; }
