#pragma once
#include <Arduino_LED_Matrix.h>
#include <WiFiS3.h>

#define LED_SCROLL_STEP_MS   60UL
#define LED_SCROLL_MAX_COLS  120

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
