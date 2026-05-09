// Stub — implemented in issue #3
#include "NoboController.h"

bool NoboController::begin(const char*, const char*)     { return false; }
void NoboController::tick()                              {}
bool NoboController::ensureProfileExists(const char*)    { return false; }
int  NoboController::findZoneIdByProfileName(const char*){ return -1; }
bool NoboController::setZoneComfort(int, time_t, time_t, int&) { return false; }
bool NoboController::clearOverride(int)                  { return false; }
bool NoboController::clearZoneOverrides(int)             { return false; }
bool NoboController::getZone(int, NoboZone&) const       { return false; }
bool NoboController::_connect()                          { return false; }
void NoboController::_disconnect()                       {}
bool NoboController::_send(const char*)                  { return false; }
bool NoboController::_readLine(char*, size_t, uint32_t)  { return false; }
bool NoboController::_readAllData()                      { return false; }
void NoboController::_parseLine(const char*)             {}
void NoboController::_parseH01(const char*)              {}
void NoboController::_parseH03(const char*)              {}
void NoboController::_parseH04(const char*)              {}
int  NoboController::_findProfileByName(const char*) const { return -1; }
