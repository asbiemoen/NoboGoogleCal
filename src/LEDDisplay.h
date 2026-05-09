#pragma once
#include <Arduino_LED_Matrix.h>
#include <ArduinoGraphics.h>
#include "Types.h"
#include "ScheduleEngine.h"
#include "WeatherService.h"

#define LED_FRAME_DURATION_MS 5000UL  // ms per message before rotating

class LEDDisplay {
public:
    LEDDisplay(ScheduleEngine& engine, WeatherService& weather);

    void begin();
    void tick();  // call every loop() iteration

private:
    ArduinoLEDMatrix _matrix;
    ScheduleEngine&  _engine;
    WeatherService&  _weather;

    uint8_t  _frameIndex;
    uint32_t _lastFrameMs;
    bool     _errorState;

    void _showFrame(uint8_t index);
    void _scrollText(const char* text);
    void _showError();
};
