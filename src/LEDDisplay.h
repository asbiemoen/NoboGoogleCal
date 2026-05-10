#pragma once
#include <Arduino_LED_Matrix.h>
#include <WiFiS3.h>

#define LED_SCROLL_STEP_MS   60UL
// hostname(31) + ".local "(7) + IP max "255.255.255.255"(15) + trailing "  "(2) = 55 chars
// 6 px per char (5 glyph + 1 gap) + 12 px blank tail so text scrolls fully off
#define LED_SCROLL_MAX_COLS  ((31 + 7 + 15 + 2) * 6 + 12)

class LEDDisplay {
public:
    LEDDisplay();
    void begin(const char* hostname);
    void tick();

private:
    ArduinoLEDMatrix _matrix;
    char     _hostname[32];
    char     _scrollMsg[64];
    uint8_t  _scrollBuf[8][LED_SCROLL_MAX_COLS];
    int16_t  _scrollPos;
    int16_t  _scrollTotal;
    uint32_t _scrollStepMs;

    void _buildMsg(char* buf, size_t len);
    void _renderToScrollBuf(const char* text);

    static const uint8_t _font5x7[][5];
};
