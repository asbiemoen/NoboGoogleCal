#pragma once
#include <Arduino_LED_Matrix.h>
#include <WiFiS3.h>
#include <time.h>
#include "Types.h"
#include "ScheduleEngine.h"
#include "WeatherService.h"

#define LED_BOOT_WINDOW_MS   (3UL * 60UL * 1000UL)
#define LED_COUNTDOWN_SECS   60
#define LED_SCROLL_STEP_MS   50UL    // ms per pixel when scrolling
#define LED_SCROLL_MAX_COLS  100     // max scroll buffer width (~16 chars)

class LEDDisplay {
public:
    LEDDisplay(ScheduleEngine& engine, WeatherService& weather);
    void begin();
    void tick();

private:
    ArduinoLEDMatrix _matrix;
    ScheduleEngine&  _engine;
    WeatherService&  _weather;

    uint8_t  _frameIndex;
    uint32_t _bootMs;
    bool     _errorState;

    char     _scrollMsg[96];
    uint8_t  _scrollBuf[8][LED_SCROLL_MAX_COLS];
    int16_t  _scrollPos;
    int16_t  _scrollTotal;
    uint32_t _scrollStepMs;

    void _buildCurrentMsg(char* buf, size_t len);
    void _renderToScrollBuf(const char* text);
    void _showError();

    // 5×7 font, ASCII 0x20–0x5A; bit 0 = top row, bit 6 = bottom row
    static const uint8_t _font5x7[][5];
};
