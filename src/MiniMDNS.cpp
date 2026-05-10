#include "MiniMDNS.h"
#include <Arduino.h>
#include <string.h>
#include <ctype.h>

static const IPAddress MDNS_ADDR(224, 0, 0, 251);
static const uint16_t  MDNS_PORT = 5353;

void MiniMDNS::begin(const char* hostname) {
    strncpy(_hostname, hostname, sizeof(_hostname) - 1);
    _hostname[sizeof(_hostname) - 1] = '\0';
    for (int i = 0; _hostname[i]; i++)
        _hostname[i] = (char)tolower((unsigned char)_hostname[i]);
    _udp.beginMulticast(MDNS_ADDR, MDNS_PORT);
}

void MiniMDNS::run() {
    int pktLen = _udp.parsePacket();
    if (pktLen <= 0 || pktLen > 512) return;
    uint8_t buf[512];
    int len = _udp.read(buf, sizeof(buf));
    if (len < 12) return;
    _handlePacket(buf, len);
}

// Parses a DNS name at buf[pos] into out; returns position after the name.
static int parseDnsName(const uint8_t* buf, int len, int pos,
                        char* out, int outLen) {
    int written = 0;
    while (pos < len) {
        uint8_t lbl = buf[pos];
        if (lbl == 0) { pos++; break; }
        if ((lbl & 0xC0) == 0xC0) { pos += 2; break; } // compression pointer — skip
        pos++;
        if (written > 0 && written < outLen - 1) out[written++] = '.';
        for (int i = 0; i < (int)lbl && pos < len && written < outLen - 1; i++, pos++)
            out[written++] = (char)tolower((unsigned char)buf[pos]);
    }
    if (written < outLen) out[written] = '\0';
    return pos;
}

void MiniMDNS::_handlePacket(const uint8_t* buf, int len) {
    uint16_t flags = ((uint16_t)buf[2] << 8) | buf[3];
    if (flags & 0x8000) return; // response — ignore

    uint16_t qdCount = ((uint16_t)buf[4] << 8) | buf[5];
    if (qdCount == 0) return;

    uint16_t txId = ((uint16_t)buf[0] << 8) | buf[1];

    char expected[40];
    snprintf(expected, sizeof(expected), "%s.local", _hostname);

    int pos = 12;
    for (int q = 0; q < (int)qdCount && pos < len; q++) {
        char qname[64] = {};
        pos = parseDnsName(buf, len, pos, qname, sizeof(qname));
        if (pos + 4 > len) return;
        uint16_t qtype = ((uint16_t)buf[pos] << 8) | buf[pos + 1];
        pos += 4; // skip type + class

        if ((qtype == 1 || qtype == 255) && strcmp(qname, expected) == 0) {
            _sendResponse(txId);
            return;
        }
    }
}

void MiniMDNS::_sendResponse(uint16_t txId) {
    IPAddress myIp = WiFi.localIP();
    uint8_t   resp[128];
    int       pos = 0;

    // Header: QR=1, AA=1, 1 answer
    resp[pos++] = txId >> 8;  resp[pos++] = txId & 0xFF;
    resp[pos++] = 0x84;       resp[pos++] = 0x00;
    resp[pos++] = 0x00;       resp[pos++] = 0x00; // QDCount = 0
    resp[pos++] = 0x00;       resp[pos++] = 0x01; // ANCount = 1
    resp[pos++] = 0x00;       resp[pos++] = 0x00;
    resp[pos++] = 0x00;       resp[pos++] = 0x00;

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

    _udp.beginPacket(MDNS_ADDR, MDNS_PORT);
    _udp.write(resp, pos);
    _udp.endPacket();
}
