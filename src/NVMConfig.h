#pragma once
#include <EEPROM.h>
#include <string.h>

// Runtime-editable settings persisted to EEPROM.
// Fields are empty strings by default; calling code falls back to
// compile-time config.h / arduino_secrets.h values when empty.

struct NVMConfig {
    uint16_t magic;
    char wifiSsid[33];
    char wifiPass[64];
    char wifiSsid2[33];
    char wifiPass2[64];
    char webPassword[32];
    char noboIp[16];
    char noboSerial[13];
    char weatherCity[32];
    char resendKey[64];
    char resendFrom[64];
    char resendTo[64];
    bool    emailEnabled;
    char    emailTime[6];       // "HH:MM\0"
    char    emailFrequency[8];  // "daily" or "weekly"
    uint8_t emailWeekday;       // 0=Sun,1=Mon,...,6=Sat — used when weekly
    char    mdnsName[32];
};

static const uint16_t NVM_MAGIC = 0xAB15;

inline void nvmLoad(NVMConfig& cfg) {
    EEPROM.get(0, cfg);
    if (cfg.magic != NVM_MAGIC) {
        memset(&cfg, 0, sizeof(cfg));
        cfg.magic = NVM_MAGIC;
        cfg.emailEnabled = true;
        strncpy(cfg.emailTime,      "07:00",  sizeof(cfg.emailTime)      - 1);
        strncpy(cfg.emailFrequency, "daily",  sizeof(cfg.emailFrequency) - 1);
        cfg.emailWeekday = 1; // Monday
    }
}

inline void nvmSave(const NVMConfig& cfg) {
    EEPROM.put(0, cfg);
}

// Returns s if non-empty, otherwise fallback.
inline const char* nvmOr(const char* s, const char* fallback) {
    return (s && s[0]) ? s : fallback;
}
