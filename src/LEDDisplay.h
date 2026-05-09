#pragma once
#include <Arduino_LED_Matrix.h>
#include <ArduinoGraphics.h>
#include <WiFiS3.h>
#include <time.h>
#include "Types.h"
#include "ScheduleEngine.h"
#include "WeatherService.h"

#define LED_FRAME_DURATION_MS   5000UL   // ms per message in normal rotation
#define LED_BOOT_WINDOW_MS      (3UL * 60UL * 1000UL)  // 3 min IP display at boot
#define LED_COUNTDOWN_SECS      60       // seconds before change to show countdown

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
    uint32_t _lastFrameMs;
    uint32_t _bootMs;
    bool     _errorState;

    void _showFrame(uint8_t index);
    void _showIP();
    void _showCountdown(int zoneIndex, time_t changeAt, bool toComfort);
    void _scrollText(const char* text);
    void _showError();
};
