#pragma once
#include <Arduino.h>
#include <stdio.h>
#include <time.h>
#include "Types.h"

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
