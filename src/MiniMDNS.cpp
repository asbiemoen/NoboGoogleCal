#include "MiniMDNS.h"
#include <Arduino.h>
#include <string.h>
#include <ctype.h>

static const IPAddress MDNS_ADDR(224, 0, 0, 251);
static const uint16_t  MDNS_PORT     = 5353;
static const uint32_t  ANNOUNCE_MS   = 60000UL;

void MiniMDNS::begin(const char* hostname) {
    strncpy(_hostname, hostname, sizeof(_hostname) - 1);
    _hostname[sizeof(_hostname) - 1] = '\0';
    for (int i = 0; _hostname[i]; i++)
        _hostname[i] = (char)tolower((unsigned char)_hostname[i]);
    _lastMs = 0; // triggers immediate announce on first run()
}

void MiniMDNS::run() {
    if (_lastMs != 0 && millis() - _lastMs < ANNOUNCE_MS) return;
    _lastMs = millis();
    _announce();
}

void MiniMDNS::_announce() {
    IPAddress myIp = WiFi.localIP();
    if (myIp == IPAddress(0, 0, 0, 0)) return;

    uint8_t resp[128];
    int     pos = 0;

    // DNS header: unsolicited response (txId=0), QR=1, AA=1, 1 answer
    resp[pos++] = 0x00; resp[pos++] = 0x00; // txId = 0
    resp[pos++] = 0x84; resp[pos++] = 0x00; // QR + AA
    resp[pos++] = 0x00; resp[pos++] = 0x00; // QDCount = 0
    resp[pos++] = 0x00; resp[pos++] = 0x01; // ANCount = 1
    resp[pos++] = 0x00; resp[pos++] = 0x00;
    resp[pos++] = 0x00; resp[pos++] = 0x00;

    // Name: <hostname>.local
    uint8_t hlen = (uint8_t)strlen(_hostname);
    resp[pos++] = hlen;
    memcpy(resp + pos, _hostname, hlen); pos += hlen;
    resp[pos++] = 5; memcpy(resp + pos, "local", 5); pos += 5;
    resp[pos++] = 0;

    // Type A (1), Class IN + cache-flush (0x8001)
    resp[pos++] = 0x00; resp[pos++] = 0x01;
    resp[pos++] = 0x80; resp[pos++] = 0x01;

    // TTL = 120 s
    resp[pos++] = 0x00; resp[pos++] = 0x00; resp[pos++] = 0x00; resp[pos++] = 120;

    // RDLENGTH = 4, then IP
    resp[pos++] = 0x00; resp[pos++] = 0x04;
    resp[pos++] = myIp[0]; resp[pos++] = myIp[1];
    resp[pos++] = myIp[2]; resp[pos++] = myIp[3];

    // Open, send, close — no persistent socket
    WiFiUDP udp;
    udp.beginPacket(MDNS_ADDR, MDNS_PORT);
    udp.write(resp, pos);
    udp.endPacket();
}
