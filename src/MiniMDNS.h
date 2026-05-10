#pragma once
#include <WiFiS3.h>
#include <WiFiUdp.h>

// Minimal mDNS responder — answers A-record queries for <hostname>.local
class MiniMDNS {
public:
    void begin(const char* hostname);
    void run();
private:
    WiFiUDP _udp;
    char    _hostname[32];
    void    _handlePacket(const uint8_t* buf, int len);
    void    _sendResponse(uint16_t txId);
};
