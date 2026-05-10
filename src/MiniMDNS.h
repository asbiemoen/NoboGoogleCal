#pragma once
#include <WiFiS3.h>
#include <WiFiUdp.h>

// Minimal mDNS — sends periodic unsolicited announcements so that
// <hostname>.local resolves on the local network without keeping a
// persistent multicast socket open (which conflicts with WiFiServer on R4).
class MiniMDNS {
public:
    void begin(const char* hostname);
    void run();   // call from loop(); announces on boot and every 60 s
private:
    char     _hostname[32];
    uint32_t _lastMs;
    void     _announce();
};
