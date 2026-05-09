#include "AppLog.h"

char AppLog::_buf[APP_LOG_SIZE][APP_LOG_WIDTH];
int  AppLog::_head  = 0;
int  AppLog::_count = 0;

void AppLog::add(const char* msg) {
    time_t now = time(nullptr);
    struct tm t = *gmtime(&now);
    int off = norwayOffsetSeconds(&t);
    time_t local = now + (time_t)off;
    struct tm lt = *gmtime(&local);

    int slot = (_head + _count) % APP_LOG_SIZE;
    snprintf(_buf[slot], APP_LOG_WIDTH, "%02d:%02d %s", lt.tm_hour, lt.tm_min, msg);

    if (_count < APP_LOG_SIZE) {
        _count++;
    } else {
        _head = (_head + 1) % APP_LOG_SIZE;
    }
}
