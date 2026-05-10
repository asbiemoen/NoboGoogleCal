#pragma once
#include "NVMConfig.h"
#include "ScheduleEngine.h"
#include "CalendarManager.h"
#include "Types.h"

class EmailService {
public:
    void begin(NVMConfig& nvm, ScheduleEngine& engine, CalendarManager& cal);
    void tick();

private:
    NVMConfig*      _nvm;
    ScheduleEngine* _engine;
    CalendarManager* _cal;
    int             _lastSentDay;   // tm_yday of last sent email (-1 = never)

    bool _shouldSend() const;
    bool _send();
    int  _buildBody(char* buf, int len) const;
};
