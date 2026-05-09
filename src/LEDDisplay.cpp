// Stub — implemented in issue #8
#include "LEDDisplay.h"

LEDDisplay::LEDDisplay(ScheduleEngine& e, WeatherService& w)
    : _engine(e), _weather(w), _frameIndex(0), _lastFrameMs(0), _errorState(false) {}

void LEDDisplay::begin()              { _matrix.begin(); }
void LEDDisplay::tick()               {}
void LEDDisplay::_showFrame(uint8_t)  {}
void LEDDisplay::_scrollText(const char*) {}
void LEDDisplay::_showError()         {}
