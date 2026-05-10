#include <time.h>      // must precede RTC.h — fully defines struct tm before wchar.h sees it
#include <WiFiS3.h>
#include <RTC.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "arduino_secrets.h"
#include "src/Types.h"
#include "src/NVMConfig.h"
#include "src/NoboController.h"
#include "src/CalendarManager.h"
#include "src/WeatherService.h"
#include "src/ScheduleEngine.h"
#include "src/AppWebServer.h"
#include "src/LEDDisplay.h"
#include "src/EmailService.h"
#include "src/MiniMDNS.h"
#include "src/AppLog.h"
#include "config.h"  // must come after Types.h

// ─── System clock ─────────────────────────────────────────────────────────────
// On R4, settimeofday() is not wired to time(). Override _gettimeofday (the
// newlib retargetable stub that time() calls internally) so that all code
// using time(nullptr) gets the real-world UTC epoch tracked via millis().
static time_t   _sysEpoch   = 0;
static uint32_t _sysEpochMs = 0;

static void setSysTime(time_t epoch) {
    _sysEpoch   = epoch;
    _sysEpochMs = millis();
}

extern "C" int _gettimeofday(struct timeval* tv, void*) {
    uint32_t elapsed = millis() - _sysEpochMs;
    tv->tv_sec  = _sysEpoch + (time_t)(elapsed / 1000UL);
    tv->tv_usec = (long)((elapsed % 1000UL) * 1000UL);
    return 0;
}

// ─── NTP ──────────────────────────────────────────────────────────────────────
static WiFiUDP   ntpUdp;
static NTPClient ntp(ntpUdp, "pool.ntp.org", 0);  // UTC only; display code applies Norway offset
static MiniMDNS  mdns;

// ─── Components ───────────────────────────────────────────────────────────────
static NoboController  nobo;
static CalendarManager calendar;
static WeatherService  weather;
static ScheduleEngine  engine(nobo, calendar, weather);
static AppWebServer    webServer(engine, weather, nobo, calendar);
static LEDDisplay      led(engine, weather);
static NVMConfig       nvm;
static EmailService    emailService;

// ─── Nobo post-connect profile setup ─────────────────────────────────────────
static bool _noboProfilesEnsured = false;

static void ensureNoboProfiles() {
    if (_noboProfilesEnsured || !nobo.isConnected()) return;
    for (int i = 0; i < ZONE_COUNT; i++) {
        nobo.ensureProfileExists(ZONES[i].name);
    }
    _noboProfilesEnsured = true;
}

// ─── WiFi ─────────────────────────────────────────────────────────────────────
static bool _tryWifi(const char* ssid, const char* pass) {
    serialTs(); Serial.print(F("Connecting to "));
    Serial.print(ssid);
    WiFi.begin(ssid, pass);
    for (uint8_t i = 0; i < 40; i++) {
        if (WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0)) break;
        delay(500);
        Serial.print('.');
    }
    Serial.println();
    if (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0, 0, 0, 0)) {
        serialTs(); Serial.println(F("WiFi connect failed"));
        WiFi.disconnect();
        return false;
    }
    serialTs(); Serial.print(F("IP: "));
    Serial.println(WiFi.localIP());
    return true;
}

static bool connectWifi() {
    // NVM overrides compile-time credentials when set
    const char* ssid1 = nvmOr(nvm.wifiSsid,  SECRET_SSID);
    const char* pass1 = nvmOr(nvm.wifiPass,  SECRET_PASS);
    if (_tryWifi(ssid1, pass1)) return true;

    // Secondary: NVM first, then compile-time fallback
    const char* ssid2 = nvm.wifiSsid2[0] ? nvm.wifiSsid2 : nullptr;
    const char* pass2 = nvm.wifiPass2[0] ? nvm.wifiPass2 : nullptr;
#ifdef SECRET_SSID2
    if (!ssid2) { ssid2 = SECRET_SSID2; pass2 = SECRET_PASS2; }
#endif
    if (ssid2) {
        serialTs(); Serial.println(F("Trying secondary WiFi"));
        if (_tryWifi(ssid2, pass2 ? pass2 : "")) return true;
    }
    return false;
}

// ─── setup / loop ─────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(1500);

    // Load NVM early so WiFi credentials can be overridden before connecting
    nvmLoad(nvm);

    while (!connectWifi()) {}

    RTC.begin();
    ntp.begin();

    // Retry until we get a valid epoch (> 2020-01-01). closes #41
    serialTs(); Serial.print(F("NTP sync"));
    time_t epoch = 0;
    for (int i = 0; i < 20 && epoch < 1577836800UL; i++) {
        if (ntp.forceUpdate()) epoch = (time_t)ntp.getEpochTime();
        if (epoch < 1577836800UL) { delay(500); Serial.print('.'); }
    }
    Serial.println(epoch >= 1577836800UL ? F(" OK") : F(" failed"));

    // Set system clock via our _gettimeofday override and also the hardware RTC
    setSysTime(epoch);
    RTCTime startTime(epoch);
    RTC.setTime(startTime);

    // Verify time() now returns a sane value
    {
        time_t now = time(nullptr);
        struct tm* utc = gmtime(&now);
        int off = norwayOffsetSeconds(utc);
        time_t localNow = now + off;
        struct tm* loc = gmtime(&localNow);
        char tbuf[48];
        snprintf(tbuf, sizeof(tbuf), "NTP epoch:    %lu", (unsigned long)epoch);
        Serial.println(tbuf);
        snprintf(tbuf, sizeof(tbuf), "time() epoch: %lu", (unsigned long)now);
        Serial.println(tbuf);
        snprintf(tbuf, sizeof(tbuf), "UTC:   %04d-%02d-%02d %02d:%02d:%02d",
                 utc->tm_year+1900, utc->tm_mon+1, utc->tm_mday,
                 utc->tm_hour, utc->tm_min, utc->tm_sec);
        Serial.println(tbuf);
        snprintf(tbuf, sizeof(tbuf), "Local: %04d-%02d-%02d %02d:%02d:%02d (UTC+%d)",
                 loc->tm_year+1900, loc->tm_mon+1, loc->tm_mday,
                 loc->tm_hour, loc->tm_min, loc->tm_sec, off / 3600);
        Serial.println(tbuf);
    }

    {
        const char* hostname = nvmOr(nvm.mdnsName, MDNS_NAME);
        mdns.begin(hostname);
        serialTs(); Serial.print(F("mDNS: "));
        Serial.print(hostname);
        Serial.println(F(".local"));
    }

    engine.begin(ZONES, ZONE_COUNT);
    webServer.begin(nvmOr(nvm.webPassword, WEB_PASSWORD), nvm);
    led.begin();

    calendar.begin(ZONES, ZONE_COUNT);
    weather.begin(nvmOr(nvm.weatherCity, WEATHER_CITY), WEATHER_API_KEY);
    nobo.begin(nvmOr(nvm.noboIp, NOBO_HUB_IP), nvmOr(nvm.noboSerial, NOBO_HUB_SERIAL));

    // Apply compile-time Resend defaults to in-memory NVM where not already set via GUI
#ifdef RESEND_API_KEY
    if (!nvm.resendKey[0])  strncpy(nvm.resendKey,  RESEND_API_KEY,   sizeof(nvm.resendKey)  - 1);
#endif
#ifdef RESEND_FROM
    if (!nvm.resendFrom[0]) strncpy(nvm.resendFrom, RESEND_FROM,      sizeof(nvm.resendFrom) - 1);
#endif
#ifdef RESEND_TO
    if (!nvm.resendTo[0])   strncpy(nvm.resendTo,   RESEND_TO,        sizeof(nvm.resendTo)   - 1);
#endif
#ifdef EMAIL_DAILY_TIME
    if (!nvm.emailTime[0])  strncpy(nvm.emailTime,  EMAIL_DAILY_TIME, sizeof(nvm.emailTime)  - 1);
#endif

    emailService.begin(nvm, engine, calendar);

    serialTs(); Serial.println(F("NoboGoogleCal ready."));
}

// ─── Cooperative multitasking ─────────────────────────────────────────────────
// Arduino's delay() calls yield() on every iteration. By overriding yield() we
// let the web server handle requests while CalendarManager is blocked waiting
// for ICS data over SSL — without threads or modem concurrency issues.
// Throttled to 50 ms so we don't spam the modem with AT commands.
void yield() {
    static uint32_t _lastYieldMs = 0;
    uint32_t now = millis();
    if (now - _lastYieldMs < 50) return;
    _lastYieldMs = now;
    webServer.tick();
}

void loop() {
    mdns.run();
    webServer.tick();
    led.tick();
    if (ntp.update()) {
        time_t ntpEpoch = (time_t)ntp.getEpochTime();
        if (ntpEpoch >= 1577836800UL) setSysTime(ntpEpoch);
    }
    engine.tick();
    calendar.tick();
    weather.tick();
    nobo.tick();
    emailService.tick();
    ensureNoboProfiles();
}
