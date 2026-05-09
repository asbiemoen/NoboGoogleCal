// Stub — implemented in issue #4
#include "CalendarManager.h"

void CalendarManager::begin(const ZoneConfig*, int)  {}
void CalendarManager::tick()                         {}
void CalendarManager::_syncZone(int)                 {}
bool CalendarManager::_fetchIcs(const char*, int)    { return false; }
void CalendarManager::_resetEventState()             {}
void CalendarManager::_commitEvent(int)              {}
void CalendarManager::_processLine(const char*, int) {}
time_t CalendarManager::_parseDateTime(const char*)  { return 0; }
void CalendarManager::_expandRRule(time_t, time_t, int) {}
bool CalendarManager::_inNext7Days(time_t) const     { return false; }
