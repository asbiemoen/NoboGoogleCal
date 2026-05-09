#include "LEDDisplay.h"
#include <Arduino.h>
#include <string.h>
#include <stdio.h>

LEDDisplay::LEDDisplay(ScheduleEngine& e, WeatherService& w)
    : _engine(e), _weather(w), _frameIndex(0), _lastFrameMs(0), _errorState(false) {}

void LEDDisplay::begin() {
    _matrix.begin();
    Serial.println(F("[LED] Matrix initialised"));
}

void LEDDisplay::tick() {
    if (millis() - _lastFrameMs < LED_FRAME_DURATION_MS) return;
    _lastFrameMs = millis();

    if (_errorState) {
        _showError();
        return;
    }

    _showFrame(_frameIndex);
    _frameIndex++;

    // Rotate through: zone 0, zone 1, weather, next event
    uint8_t totalFrames = 4;
    if (_frameIndex >= totalFrames) _frameIndex = 0;
}

void LEDDisplay::_showFrame(uint8_t index) {
    char msg[64];

    switch (index) {
        case 0:
        case 1: {
            // Zone status — one zone per frame
            int zi = index;  // frame 0 = zone 0, frame 1 = zone 1
            const char* st = statusName(_engine.zoneStatus(zi));
            snprintf(msg, sizeof(msg), "Z%d: %s", zi + 1, st);
            break;
        }
        case 2: {
            // Outside temperature
            char tempBuf[8];
            dtostrf(_weather.currentTemp(), 4, 1, tempBuf);
            snprintf(msg, sizeof(msg), "OUT: %s C", tempBuf);
            if (!_weather.comfortAllowed()) strncat(msg, " WARM", sizeof(msg) - strlen(msg) - 1);
            break;
        }
        case 3: {
            // Next event countdown
            strncpy(msg, _engine.nextEventString(), sizeof(msg) - 1);
            break;
        }
        default:
            strncpy(msg, "NoboGCal", sizeof(msg) - 1);
    }

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
    // Display a blinking "!" pattern
    _matrix.beginDraw();
    _matrix.stroke(0xFFFFFFFF);

    // Draw exclamation mark in centre
    uint8_t frame[8][12] = {};
    for (int r = 1; r <= 5; r++) frame[r][5] = 1;  // vertical bar
    frame[7][5] = 1;                                  // dot

    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 12; c++)
            if (frame[r][c]) _matrix.point(c, r);

    _matrix.endDraw();
}
