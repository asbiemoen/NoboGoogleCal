#include "EmailService.h"
#include "AppLog.h"
#include <Arduino.h>
#include <WiFiSSLClient.h>
#include <ArduinoHttpClient.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static const char RESEND_HOST[] = "api.resend.com";

void EmailService::begin(NVMConfig& nvm, ScheduleEngine& engine, CalendarManager& cal) {
    _nvm        = &nvm;
    _engine     = &engine;
    _cal        = &cal;
    _lastSentDay = -1;
}

void EmailService::tick() {
    if (!_shouldSend()) return;

    time_t now = time(nullptr);
    struct tm* utc = gmtime(&now);
    if (_lastSentDay == utc->tm_yday) return;

    if (_send()) {
        _lastSentDay = utc->tm_yday;
    }
}

bool EmailService::_shouldSend() const {
    if (!_nvm || !_nvm->emailEnabled) return false;
    if (!_nvm->resendKey[0])           return false;
    if (!_nvm->resendFrom[0])          return false;
    if (!_nvm->resendTo[0])            return false;

    // Parse send time HH:MM
    int hh = 7, mm = 0;
    if (_nvm->emailTime[0]) {
        sscanf(_nvm->emailTime, "%d:%d", &hh, &mm);
    }

    time_t now = time(nullptr);
    struct tm* utc = gmtime(&now);
    // Apply Norway offset for local time comparison
    extern int norwayOffsetSeconds(const struct tm*);
    int off = norwayOffsetSeconds(utc);
    time_t localNow = now + (time_t)off;
    struct tm lTm = *gmtime(&localNow);

    return (lTm.tm_hour == hh && lTm.tm_min >= mm && lTm.tm_min < mm + 5);
}

int EmailService::_buildBody(char* buf, int len) const {
    int pos = 0;
    pos += snprintf(buf + pos, len - pos, "<h2>Daily heating summary</h2>");

    time_t now = time(nullptr);

    for (int i = 0; i < _engine->zoneCount() && pos < len - 100; i++) {
        const char* statusStr = statusName(_engine->zoneStatus(i));
        pos += snprintf(buf + pos, len - pos,
                        "<h3>%s &mdash; %s</h3><ul>",
                        _engine->zoneName(i), statusStr);

        int shown = 0;
        time_t lastStart = 0;
        for (int pass = 0; pass < 5 && pos < len - 80; pass++) {
            time_t best = 0; int bestIdx = -1;
            for (int e = 0; e < MAX_EVENTS_PER_ZONE * MAX_ZONES; e++) {
                const CalEvent& ev = _cal->events()[e];
                if (!ev.valid || ev.zoneIndex != (uint8_t)i) continue;
                if (ev.end <= now) continue;
                if (ev.start <= lastStart && lastStart != 0) continue;
                if (best == 0 || ev.start < best) { best = ev.start; bestIdx = e; }
            }
            if (bestIdx < 0) break;
            const CalEvent& ev = _cal->events()[bestIdx];
            lastStart = ev.start;

            struct tm utcTm = *gmtime(&ev.start);
            extern int norwayOffsetSeconds(const struct tm*);
            time_t localStart = ev.start + (time_t)norwayOffsetSeconds(&utcTm);
            struct tm lTm = *gmtime(&localStart);
            struct tm utcEnd = *gmtime(&ev.end);
            time_t localEnd = ev.end + (time_t)norwayOffsetSeconds(&utcEnd);
            struct tm lEnd = *gmtime(&localEnd);

            pos += snprintf(buf + pos, len - pos,
                            "<li>%04d-%02d-%02d %02d:%02d &ndash; %02d:%02d</li>",
                            lTm.tm_year + 1900, lTm.tm_mon + 1, lTm.tm_mday,
                            lTm.tm_hour, lTm.tm_min, lEnd.tm_hour, lEnd.tm_min);
            shown++;
        }
        if (shown == 0) pos += snprintf(buf + pos, len - pos, "<li>No upcoming events</li>");
        pos += snprintf(buf + pos, len - pos, "</ul>");
    }
    return pos;
}

bool EmailService::_send() {
    char body[1536];
    char htmlBody[1024];
    _buildBody(htmlBody, sizeof(htmlBody));

    // Build JSON payload
    int blen = snprintf(body, sizeof(body),
        "{\"from\":\"%s\",\"to\":[\"%s\"],"
        "\"subject\":\"Heating schedule\","
        "\"html\":\"%s\"}",
        _nvm->resendFrom, _nvm->resendTo, htmlBody);

    if (blen <= 0 || blen >= (int)sizeof(body)) {
        AppLog::add("Email: body too large");
        return false;
    }

    char authHdr[80];
    snprintf(authHdr, sizeof(authHdr), "Bearer %s", _nvm->resendKey);

    WiFiSSLClient ssl;
    HttpClient    http(ssl, RESEND_HOST, 443);
    http.setTimeout(10000);

    http.beginRequest();
    http.post("/emails");
    http.sendHeader("Content-Type", "application/json");
    http.sendHeader("Authorization", authHdr);
    http.sendHeader("Content-Length", blen);
    http.beginBody();
    http.print(body);
    http.endRequest();

    int code = http.responseStatusCode();
    http.stop();

    if (code == 200 || code == 201) {
        serialTs(); Serial.println(F("[Email] Daily summary sent"));
        AppLog::add("Email: daily summary sent");
        return true;
    }

    serialTs(); Serial.print(F("[Email] HTTP error: "));
    Serial.println(code);
    char msg[APP_LOG_WIDTH];
    snprintf(msg, sizeof(msg), "Email: send failed (HTTP %d)", code);
    AppLog::add(msg);
    return false;
}
