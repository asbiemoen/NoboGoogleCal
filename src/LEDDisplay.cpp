#include "LEDDisplay.h"
#include <Arduino.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

LEDDisplay::LEDDisplay(ScheduleEngine& e, WeatherService& w)
    : _engine(e), _weather(w), _frameIndex(0), _lastFrameMs(0),
      _bootMs(0), _errorState(false) {}

void LEDDisplay::begin() {
    _matrix.begin();
    _bootMs = millis();
    Serial.println(F("[LED] Matrix initialised"));
}

void LEDDisplay::tick() {
    if (millis() - _lastFrameMs < LED_FRAME_DURATION_MS) return;
    _lastFrameMs = millis();

    // Priority 1: Boot window — scroll IP address
    if (millis() - _bootMs < LED_BOOT_WINDOW_MS) {
        _showIP();
        return;
    }

    // Priority 2: Imminent heating change — countdown
    time_t nearestChangeAt  = 0;
    bool   nearestToComfort = false;
    int    nearestZone      = -1;

    for (int i = 0; i < _engine.zoneCount(); i++) {
        time_t changeAt;
        bool   toComfort;
        if (_engine.nextChangeForZone(i, LED_COUNTDOWN_SECS, changeAt, toComfort)) {
            if (nearestChangeAt == 0 || changeAt < nearestChangeAt) {
                nearestChangeAt  = changeAt;
                nearestToComfort = toComfort;
                nearestZone      = i;
            }
        }
    }

    if (nearestZone >= 0) {
        _showCountdown(nearestZone, nearestChangeAt, nearestToComfort);
        return;
    }

    // Priority 3: Normal frame rotation
    if (_errorState) {
        _showError();
        return;
    }

    _showFrame(_frameIndex);
    uint8_t totalFrames = (uint8_t)(_engine.zoneCount() + 2);  // zones + weather + next event
    if (++_frameIndex >= totalFrames) _frameIndex = 0;
}

void LEDDisplay::_showFrame(uint8_t index) {
    char msg[64];
    int  zc = _engine.zoneCount();

    if (index < (uint8_t)zc) {
        snprintf(msg, sizeof(msg), "Z%d:%s", index + 1, statusName(_engine.zoneStatus(index)));
    } else if (index == (uint8_t)zc) {
        char tempBuf[8];
        dtostrf(_weather.currentTemp(), 4, 1, tempBuf);
        snprintf(msg, sizeof(msg), "OUT:%sC", tempBuf);
        if (!_weather.comfortAllowed())
            strncat(msg, " WARM", sizeof(msg) - strlen(msg) - 1);
    } else {
        strncpy(msg, _engine.nextEventString(), sizeof(msg) - 1);
        msg[sizeof(msg) - 1] = '\0';
    }

    _scrollText(msg);
}

void LEDDisplay::_showIP() {
    IPAddress ip = WiFi.localIP();
    char msg[24];
    snprintf(msg, sizeof(msg), "IP:%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    _scrollText(msg);
}

void LEDDisplay::_showCountdown(int zoneIndex, time_t changeAt, bool toComfort) {
    long secsLeft = (long)(changeAt - time(nullptr));
    if (secsLeft < 0) secsLeft = 0;
    char msg[24];
    snprintf(msg, sizeof(msg), "Z%d %s %lds",
             zoneIndex + 1,
             toComfort ? "ON" : "OFF",
             secsLeft);
    _scrollText(msg);
}

void LEDDisplay::_scrollText(const char* text) {
    _matrix.beginDraw();
    _matrix.stroke(0xFFFFFFFF);
    _matrix.textScrollSpeed(50);
    _matrix.textFont(Font_4x6);
    _matrix.beginText(0, 1, 0xFFFFFF);
    _matrix.print(text);
    _matrix.endText(SCROLL_LEFT);
    _matrix.endDraw();
}

void LEDDisplay::_showError() {
    _matrix.beginDraw();
    _matrix.stroke(0xFFFFFFFF);

    uint8_t frame[8][12] = {};
    for (int r = 1; r <= 5; r++) frame[r][5] = 1;
    frame[7][5] = 1;

    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 12; c++)
            if (frame[r][c]) _matrix.point(c, r);

    _matrix.endDraw();
}
