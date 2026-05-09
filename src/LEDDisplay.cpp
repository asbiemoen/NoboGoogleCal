#include "LEDDisplay.h"
#include <Arduino.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// 5×7 font, ASCII 0x20 (space) – 0x5A (Z).
// Each entry: 5 column bytes. Bit 0 = top row (row 0), bit 6 = bottom row (row 6).
const uint8_t LEDDisplay::_font5x7[][5] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, // 0x20 ' '
    { 0x00, 0x00, 0x5F, 0x00, 0x00 }, // 0x21 '!'
    { 0x00, 0x07, 0x00, 0x07, 0x00 }, // 0x22 '"'
    { 0x14, 0x7F, 0x14, 0x7F, 0x14 }, // 0x23 '#'
    { 0x24, 0x2A, 0x7F, 0x2A, 0x12 }, // 0x24 '$'
    { 0x23, 0x13, 0x08, 0x64, 0x62 }, // 0x25 '%'
    { 0x36, 0x49, 0x55, 0x22, 0x50 }, // 0x26 '&'
    { 0x00, 0x05, 0x03, 0x00, 0x00 }, // 0x27 '\''
    { 0x00, 0x1C, 0x22, 0x41, 0x00 }, // 0x28 '('
    { 0x00, 0x41, 0x22, 0x1C, 0x00 }, // 0x29 ')'
    { 0x14, 0x08, 0x3E, 0x08, 0x14 }, // 0x2A '*'
    { 0x08, 0x08, 0x3E, 0x08, 0x08 }, // 0x2B '+'
    { 0x00, 0x50, 0x30, 0x00, 0x00 }, // 0x2C ','
    { 0x08, 0x08, 0x08, 0x08, 0x08 }, // 0x2D '-'
    { 0x00, 0x60, 0x60, 0x00, 0x00 }, // 0x2E '.'
    { 0x20, 0x10, 0x08, 0x04, 0x02 }, // 0x2F '/'
    { 0x3E, 0x51, 0x49, 0x45, 0x3E }, // 0x30 '0'
    { 0x00, 0x42, 0x7F, 0x40, 0x00 }, // 0x31 '1'
    { 0x42, 0x61, 0x51, 0x49, 0x46 }, // 0x32 '2'
    { 0x21, 0x41, 0x45, 0x4B, 0x31 }, // 0x33 '3'
    { 0x18, 0x14, 0x12, 0x7F, 0x10 }, // 0x34 '4'
    { 0x27, 0x45, 0x45, 0x45, 0x39 }, // 0x35 '5'
    { 0x3C, 0x4A, 0x49, 0x49, 0x30 }, // 0x36 '6'
    { 0x01, 0x71, 0x09, 0x05, 0x03 }, // 0x37 '7'
    { 0x36, 0x49, 0x49, 0x49, 0x36 }, // 0x38 '8'
    { 0x06, 0x49, 0x49, 0x29, 0x1E }, // 0x39 '9'
    { 0x00, 0x36, 0x36, 0x00, 0x00 }, // 0x3A ':'
    { 0x00, 0x56, 0x36, 0x00, 0x00 }, // 0x3B ';'
    { 0x08, 0x14, 0x22, 0x41, 0x00 }, // 0x3C '<'
    { 0x14, 0x14, 0x14, 0x14, 0x14 }, // 0x3D '='
    { 0x00, 0x41, 0x22, 0x14, 0x08 }, // 0x3E '>'
    { 0x02, 0x01, 0x51, 0x09, 0x06 }, // 0x3F '?'
    { 0x32, 0x49, 0x79, 0x41, 0x3E }, // 0x40 '@'
    { 0x7E, 0x11, 0x11, 0x11, 0x7E }, // 0x41 'A'
    { 0x7F, 0x49, 0x49, 0x49, 0x36 }, // 0x42 'B'
    { 0x3E, 0x41, 0x41, 0x41, 0x22 }, // 0x43 'C'
    { 0x7F, 0x41, 0x41, 0x22, 0x1C }, // 0x44 'D'
    { 0x7F, 0x49, 0x49, 0x49, 0x41 }, // 0x45 'E'
    { 0x7F, 0x09, 0x09, 0x09, 0x01 }, // 0x46 'F'
    { 0x3E, 0x41, 0x49, 0x49, 0x7A }, // 0x47 'G'
    { 0x7F, 0x08, 0x08, 0x08, 0x7F }, // 0x48 'H'
    { 0x00, 0x41, 0x7F, 0x41, 0x00 }, // 0x49 'I'
    { 0x20, 0x40, 0x41, 0x3F, 0x01 }, // 0x4A 'J'
    { 0x7F, 0x08, 0x14, 0x22, 0x41 }, // 0x4B 'K'
    { 0x7F, 0x40, 0x40, 0x40, 0x40 }, // 0x4C 'L'
    { 0x7F, 0x02, 0x0C, 0x02, 0x7F }, // 0x4D 'M'
    { 0x7F, 0x04, 0x08, 0x10, 0x7F }, // 0x4E 'N'
    { 0x3E, 0x41, 0x41, 0x41, 0x3E }, // 0x4F 'O'
    { 0x7F, 0x09, 0x09, 0x09, 0x06 }, // 0x50 'P'
    { 0x3E, 0x41, 0x51, 0x21, 0x5E }, // 0x51 'Q'
    { 0x7F, 0x09, 0x19, 0x29, 0x46 }, // 0x52 'R'
    { 0x46, 0x49, 0x49, 0x49, 0x31 }, // 0x53 'S'
    { 0x01, 0x01, 0x7F, 0x01, 0x01 }, // 0x54 'T'
    { 0x3F, 0x40, 0x40, 0x40, 0x3F }, // 0x55 'U'
    { 0x1F, 0x20, 0x40, 0x20, 0x1F }, // 0x56 'V'
    { 0x3F, 0x40, 0x38, 0x40, 0x3F }, // 0x57 'W'
    { 0x63, 0x14, 0x08, 0x14, 0x63 }, // 0x58 'X'
    { 0x07, 0x08, 0x70, 0x08, 0x07 }, // 0x59 'Y'
    { 0x61, 0x51, 0x49, 0x45, 0x43 }, // 0x5A 'Z'
};

LEDDisplay::LEDDisplay(ScheduleEngine& e, WeatherService& w)
    : _engine(e), _weather(w), _frameIndex(0),
      _bootMs(0), _errorState(false),
      _scrollPos(0), _scrollTotal(12), _scrollStepMs(0) {
    memset(_scrollMsg, 0, sizeof(_scrollMsg));
    memset(_scrollBuf, 0, sizeof(_scrollBuf));
}

void LEDDisplay::begin() {
    _matrix.begin();
    _bootMs = millis();
    Serial.println(F("[LED] Matrix initialised"));
}

void LEDDisplay::tick() {
    uint32_t now = millis();

    if (_errorState) {
        if (now - _scrollStepMs >= 500UL) {
            _scrollStepMs = now;
            _showError();
        }
        return;
    }

    char msg[96] = {};
    _buildCurrentMsg(msg, sizeof(msg));

    if (strncmp(msg, _scrollMsg, sizeof(_scrollMsg)) != 0) {
        strncpy(_scrollMsg, msg, sizeof(_scrollMsg) - 1);
        _scrollMsg[sizeof(_scrollMsg) - 1] = '\0';
        _renderToScrollBuf(_scrollMsg);
        _scrollPos    = 0;
        _scrollStepMs = now;
    }

    if (now - _scrollStepMs >= LED_SCROLL_STEP_MS) {
        _scrollStepMs = now;
        if (++_scrollPos >= _scrollTotal) {
            _scrollPos = 0;
            if (now - _bootMs >= LED_BOOT_WINDOW_MS) {
                uint8_t totalFrames = (uint8_t)(_engine.zoneCount() + 2);
                if (++_frameIndex >= totalFrames) _frameIndex = 0;
            }
        }
    }

    uint8_t frame[8][12] = {};
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 12; c++) {
            int src = _scrollPos + c;
            if (src < LED_SCROLL_MAX_COLS) frame[r][c] = _scrollBuf[r][src];
        }
    }
    _matrix.renderBitmap(frame, 8, 12);
}

void LEDDisplay::_buildCurrentMsg(char* buf, size_t len) {
    if (millis() - _bootMs < LED_BOOT_WINDOW_MS) {
        IPAddress ip = WiFi.localIP();
        snprintf(buf, len, "IP:%d.%d.%d.%d ", ip[0], ip[1], ip[2], ip[3]);
        return;
    }

    time_t nearestAt   = 0;
    bool   nearestComf = false;
    int    nearestZone = -1;
    for (int i = 0; i < _engine.zoneCount(); i++) {
        time_t changeAt; bool toComfort;
        if (_engine.nextChangeForZone(i, LED_COUNTDOWN_SECS, changeAt, toComfort)) {
            if (nearestAt == 0 || changeAt < nearestAt) {
                nearestAt   = changeAt;
                nearestComf = toComfort;
                nearestZone = i;
            }
        }
    }
    if (nearestZone >= 0) {
        long secsLeft = (long)(nearestAt - time(nullptr));
        if (secsLeft < 0) secsLeft = 0;
        snprintf(buf, len, "Z%d %s %lds ", nearestZone + 1,
                 nearestComf ? "ON" : "OFF", secsLeft);
        return;
    }

    int zc = _engine.zoneCount();
    if (_frameIndex < (uint8_t)zc) {
        snprintf(buf, len, "Z%d:%s ", _frameIndex + 1,
                 statusName(_engine.zoneStatus(_frameIndex)));
    } else if (_frameIndex == (uint8_t)zc) {
        char tmp[8];
        dtostrf(_weather.currentTemp(), 4, 1, tmp);
        snprintf(buf, len, "OUT:%sC ", tmp);
    } else {
        snprintf(buf, len, "%s ", _engine.nextEventString());
    }
}

void LEDDisplay::_renderToScrollBuf(const char* text) {
    memset(_scrollBuf, 0, sizeof(_scrollBuf));
    int x = 0;
    for (int i = 0; text[i] && x < LED_SCROLL_MAX_COLS - 6; i++) {
        uint8_t ch = (uint8_t)text[i];
        if (ch >= 'a' && ch <= 'z') ch -= 32;   // uppercase
        if (ch < 0x20 || ch > 0x5A) ch = 0x20;  // unsupported → space
        const uint8_t* g = _font5x7[ch - 0x20];
        for (int col = 0; col < 5 && x < LED_SCROLL_MAX_COLS; col++, x++) {
            for (int row = 0; row < 7; row++) {
                if (g[col] & (1u << row)) _scrollBuf[row][x] = 1;
            }
        }
        if (x < LED_SCROLL_MAX_COLS) x++; // 1-pixel gap between chars
    }
    _scrollTotal = (int16_t)(x + 12); // trailing blank lets text scroll fully off
}

void LEDDisplay::_showError() {
    byte frame[8][12] = {
        { 0,0,0,0,0,0,0,0,0,0,0,0 },
        { 0,0,0,0,0,1,0,0,0,0,0,0 },
        { 0,0,0,0,0,1,0,0,0,0,0,0 },
        { 0,0,0,0,0,1,0,0,0,0,0,0 },
        { 0,0,0,0,0,1,0,0,0,0,0,0 },
        { 0,0,0,0,0,1,0,0,0,0,0,0 },
        { 0,0,0,0,0,0,0,0,0,0,0,0 },
        { 0,0,0,0,0,1,0,0,0,0,0,0 },
    };
    _matrix.renderBitmap(frame, 8, 12);
}
