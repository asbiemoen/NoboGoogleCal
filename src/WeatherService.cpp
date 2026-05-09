// Stub — implemented in issue #6
#include "WeatherService.h"

void WeatherService::begin(const char*, const char*) {}
void WeatherService::tick()                          {}
bool WeatherService::_fetch()                        { return false; }
bool WeatherService::_summerFallback() const         { return false; }
