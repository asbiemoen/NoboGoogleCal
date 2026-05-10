#pragma once
#include <Arduino.h>
#include <stdio.h>
#include <time.h>
#include "Types.h"

// Prints "HH:MM:SS " (Norway local time) or "[+Xs] " before NTP sync.
// Add before the first Serial.print in each log chain.
inline void serialTs() {
    time_t now = time(nullptr);
    char   buf[14];
    if (now < 1577836800UL) {
        snprintf(buf, sizeof(buf), "[+%lus] ", (unsigned long)(millis() / 1000UL));
    } else {
        extern int norwayOffsetSeconds(const struct tm*);
        struct tm utcTm = *gmtime(&now);
        int    off   = norwayOffsetSeconds(&utcTm);
        time_t local = now + (time_t)off;
        struct tm lTm = *gmtime(&local);
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d ", lTm.tm_hour, lTm.tm_min, lTm.tm_sec);
    }
    Serial.print(buf);
}

#define APP_LOG_SIZE  12
#define APP_LOG_WIDTH 60

class AppLog {
public:
    static void        add(const char* msg);
    static int         count()      { return _count; }
    // i=0 is oldest, i=count()-1 is newest
    static const char* entry(int i) { return _buf[(_head + i) % APP_LOG_SIZE]; }

private:
    static char _buf[APP_LOG_SIZE][APP_LOG_WIDTH];
    static int  _head;
    static int  _count;
};
