#include "NoboController.h"
#include "AppLog.h"
#include <Arduino.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

// Nobø Energy Hub TCP protocol v1.1
// Reference: https://github.com/torbjornnes/pynobo (Python implementation)
// Port: 27779
// Handshake: "HELLO 1.1 <first-3-of-serial> <epoch>\r\n"
// Hub sends all data then "HANDSHAKE\r\n"
// Override modes: 0=NORMAL 1=COMFORT 2=ECO 3=AWAY
// Override types: 0=constant 1=timed (has end epoch)
// Target types:   0=zone     1=component

bool NoboController::begin(const char* ip, const char* serial) {
    strncpy(_ip, ip, sizeof(_ip) - 1);
    strncpy(_serial, serial, sizeof(_serial) - 1);
    _connected            = false;
    _lastKeepAlive        = 0;
    _lastReconnectAttempt = 0;
    _nextOverrideId       = 1;
    _zoneCount            = 0;
    _profileCount         = 0;
    _overrideCount        = 0;

    return true;
}

void NoboController::tick() {
    if (!_connected) {
        uint32_t now = millis();
        if (_lastReconnectAttempt == 0 || now - _lastReconnectAttempt > 30000UL) {
            _lastReconnectAttempt = now;
            serialTs(); Serial.print(F("[Nobo] Connecting to "));
            Serial.println(_ip);
            _connect();
        }
        return;
    }

    // Send keep-alive every 30 s
    if (millis() - _lastKeepAlive > KEEPALIVE_MS) {
        _send("Y02");
        _lastKeepAlive = millis();
    }

    // Drain any incoming data (hub can push updates)
    char line[128];
    while (_client.available()) {
        if (_readLine(line, sizeof(line), 100)) {
            _parseLine(line);
        }
    }

    if (!_client.connected()) {
        serialTs(); Serial.println(F("[Nobo] Connection lost"));
        _disconnect();
    }
}

bool NoboController::getZone(int i, NoboZone& out) const {
    if (i < 0 || i >= _zoneCount) return false;
    out = _zones[i];
    return true;
}

bool NoboController::ensureProfileExists(const char* name) {
    if (!_connected) return false;
    if (_findProfileByName(name) >= 0) return true;  // already exists

    // Create a week profile: all hours ECO (mode 2)
    // A03 command: A03 <name> <color> <num_slots> <slot_list>
    // Slot list: "0 00:00 2" = day 0 (all days inherit), from 00:00, ECO
    // Color 0 = default
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "A03 %s 0 1 0 00:00 2", name);
    if (!_send(cmd)) return false;

    // Read confirmation (B03 <id> ...)
    char resp[128];
    if (!_readLine(resp, sizeof(resp))) return false;
    if (strncmp(resp, "B03", 3) != 0) return false;

    // Parse new profile ID from "B03 <id> <name> ..."
    int newId = atoi(resp + 4);
    if (_profileCount < MAX_NOBO_PROFILES) {
        _profiles[_profileCount].id = newId;
        strncpy(_profiles[_profileCount].name, name, sizeof(_profiles[0].name) - 1);
        _profileCount++;
    }
    serialTs(); Serial.print(F("[Nobo] Created profile: "));
    Serial.println(name);
    return true;
}

int NoboController::findZoneIdByProfileName(const char* profileName) {
    int profileId = _findProfileByName(profileName);
    if (profileId < 0) return -1;

    for (int i = 0; i < _zoneCount; i++) {
        if (_zones[i].weekProfileId == profileId) {
            return _zones[i].id;
        }
    }
    return -1;
}

bool NoboController::setZoneComfort(int zoneId, time_t from, time_t until, int& outOverrideId) {
    if (!_connected || zoneId < 0) return false;

    outOverrideId = _nextOverrideId++;
    char cmd[128];
    // A06 <id> <mode> <type> <end-epoch> <start-epoch> <target-type> <target-id>
    snprintf(cmd, sizeof(cmd),
             "A06 %d 1 1 %ld %ld 0 %d",
             outOverrideId, (long)until, (long)from, zoneId);

    if (!_send(cmd)) { outOverrideId = -1; return false; }

    char resp[64];
    if (!_readLine(resp, sizeof(resp))) { outOverrideId = -1; return false; }
    // Expected: "B06 <id>"
    if (strncmp(resp, "B06", 3) != 0) { outOverrideId = -1; return false; }

    if (_overrideCount < MAX_NOBO_OVERRIDES) {
        _overrides[_overrideCount++] = { outOverrideId, 1, from, until, 0, zoneId };
    }
    return true;
}

bool NoboController::clearOverride(int overrideId) {
    if (!_connected || overrideId < 0) return false;

    char cmd[32];
    snprintf(cmd, sizeof(cmd), "D06 %d", overrideId);
    if (!_send(cmd)) return false;

    char resp[64];
    _readLine(resp, sizeof(resp));  // E06 <id>

    // Remove from local list
    for (int i = 0; i < _overrideCount; i++) {
        if (_overrides[i].id == overrideId) {
            _overrides[i] = _overrides[--_overrideCount];
            break;
        }
    }
    return true;
}

bool NoboController::clearZoneOverrides(int zoneId) {
    bool ok = true;
    for (int i = _overrideCount - 1; i >= 0; i--) {
        if (_overrides[i].targetId == zoneId && _overrides[i].targetType == 0) {
            ok &= clearOverride(_overrides[i].id);
        }
    }
    return ok;
}

// ─── Private ─────────────────────────────────────────────────────────────────

bool NoboController::_connect() {
    _client.stop();
    _connected = false;

    if (!_client.connect(_ip, NOBO_PORT)) {
        serialTs(); Serial.println(F("[Nobo] TCP connect failed"));
        AppLog::add("Nobø: connect failed");
        return false;
    }

    // Handshake: send first 3 chars of serial + epoch
    char cmd[48];
    char serial3[4];
    strncpy(serial3, _serial, 3);
    serial3[3] = '\0';
    snprintf(cmd, sizeof(cmd), "HELLO %s %s %ld", NOBO_API_VERSION, serial3, (long)time(nullptr));

    if (!_send(cmd)) { _client.stop(); return false; }

    char resp[128];
    if (!_readLine(resp, sizeof(resp), 8000)) {
        serialTs(); Serial.println(F("[Nobo] Handshake timeout"));
        _client.stop();
        return false;
    }
    if (strncmp(resp, "HELLO", 5) != 0) {
        serialTs(); Serial.println(F("[Nobo] Unexpected handshake response"));
        _client.stop();
        return false;
    }

    serialTs(); Serial.println(F("[Nobo] Connected, loading data..."));
    AppLog::add("Nobø: connected");
    _connected      = true;
    _lastKeepAlive  = millis();

    // Hub sends full data snapshot after handshake
    if (!_readAllData()) {
        serialTs(); Serial.println(F("[Nobo] Failed to read initial data"));
    }

    // Start override IDs beyond any the hub already knows about
    _nextOverrideId = 1;
    for (int i = 0; i < _overrideCount; i++) {
        if (_overrides[i].id >= _nextOverrideId)
            _nextOverrideId = _overrides[i].id + 1;
    }
    return true;
}

void NoboController::_disconnect() {
    _client.stop();
    _connected = false;
    AppLog::add("Nobø: disconnected");
}

bool NoboController::_send(const char* cmd) {
    if (!_client.connected()) return false;
    _client.print(cmd);
    _client.print("\r\n");
    return true;
}

bool NoboController::_readLine(char* buf, size_t len, uint32_t timeoutMs) {
    uint32_t deadline = millis() + timeoutMs;
    size_t   pos      = 0;

    while (millis() < deadline) {
        if (_client.available()) {
            char c = _client.read();
            if (c == '\n') {
                if (pos > 0 && buf[pos - 1] == '\r') pos--;
                buf[pos] = '\0';
                return pos > 0;
            }
            if (pos < len - 1) buf[pos++] = c;
        } else {
            delay(1);  // yield to other work instead of spinning
        }
    }
    buf[pos] = '\0';
    return false;
}

bool NoboController::_readAllData() {
    char line[256];
    while (_connected) {
        if (!_readLine(line, sizeof(line), 5000)) break;
        if (strcmp(line, "HANDSHAKE") == 0) return true;
        _parseLine(line);
    }
    return false;
}

void NoboController::_parseLine(const char* line) {
    if      (strncmp(line, "H01", 3) == 0) _parseH01(line + 4);
    else if (strncmp(line, "H03", 3) == 0) _parseH03(line + 4);
    else if (strncmp(line, "H04", 3) == 0) _parseH04(line + 4);
    // H02 (components) and H05 (calendars) are not needed
}

void NoboController::_parseH01(const char* data) {
    // H01 <id> <name> <week-profile-id> <comfort-temp> <eco-temp> <allow-override>
    if (_zoneCount >= MAX_NOBO_ZONES) return;
    NoboZone& z = _zones[_zoneCount];
    char nameBuf[32];
    int  id, profileId;

    if (sscanf(data, "%d %31s %d", &id, nameBuf, &profileId) >= 2) {
        z.id            = id;
        z.weekProfileId = profileId;
        strncpy(z.name, nameBuf, sizeof(z.name) - 1);
        _zoneCount++;
    }
}

void NoboController::_parseH03(const char* data) {
    // H03 <id> <name> <color> ...
    if (_profileCount >= MAX_NOBO_PROFILES) return;
    NoboProfile& p = _profiles[_profileCount];
    char nameBuf[32];
    int  id;

    if (sscanf(data, "%d %31s", &id, nameBuf) == 2) {
        p.id = id;
        strncpy(p.name, nameBuf, sizeof(p.name) - 1);
        _profileCount++;
    }
}

void NoboController::_parseH04(const char* data) {
    // H04 <id> <mode> <type> <end-epoch> <start-epoch> <target-type> <target-id>
    if (_overrideCount >= MAX_NOBO_OVERRIDES) return;
    NoboOverride& o = _overrides[_overrideCount];
    int id, mode, type, targetType, targetId;
    long endEpoch, startEpoch;

    if (sscanf(data, "%d %d %d %ld %ld %d %d",
               &id, &mode, &type, &endEpoch, &startEpoch, &targetType, &targetId) == 7) {
        o = { id, mode, (time_t)startEpoch, (time_t)endEpoch, targetType, targetId };
        _overrideCount++;
    }
}

int NoboController::_findProfileByName(const char* name) const {
    for (int i = 0; i < _profileCount; i++) {
        if (strncmp(_profiles[i].name, name, sizeof(_profiles[0].name)) == 0)
            return _profiles[i].id;
    }
    return -1;
}
