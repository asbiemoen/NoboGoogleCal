#include <time.h>      // must precede RTC.h — fully defines struct tm before wchar.h sees it
#include <WiFiS3.h>
#include <RTC.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "arduino_secrets.h"
#include "src/Types.h"
#include "src/NoboController.h"
#include "src/CalendarManager.h"
#include "src/WeatherService.h"
#include "src/ScheduleEngine.h"
#include "src/AppWebServer.h"
#include "src/LEDDisplay.h"
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

// ─── Components ───────────────────────────────────────────────────────────────
static NoboController  nobo;
static CalendarManager calendar;
static WeatherService  weather;
static ScheduleEngine  engine(nobo, calendar, weather);
static AppWebServer    webServer(engine, weather, nobo, calendar);
static LEDDisplay      led(engine, weather);

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
    Serial.print(F("Connecting to "));
    Serial.print(ssid);
    WiFi.begin(ssid, pass);
    for (uint8_t i = 0; i < 40; i++) {
        if (WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0)) break;
        delay(500);
        Serial.print('.');
    }
    Serial.println();
    if (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0, 0, 0, 0)) {
        Serial.println(F("WiFi connect failed"));
        WiFi.disconnect();
        return false;
    }
    Serial.print(F("IP: "));
    Serial.println(WiFi.localIP());
    return true;
}

static bool connectWifi() {
    if (_tryWifi(SECRET_SSID, SECRET_PASS)) return true;
#ifdef SECRET_SSID2
    Serial.println(F("Trying secondary WiFi"));
    if (_tryWifi(SECRET_SSID2, SECRET_PASS2)) return true;
#endif
    return false;
}

// ─── setup / loop ─────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(1500);

    while (!connectWifi()) {}

    RTC.begin();
    ntp.begin();

    // Retry until we get a valid epoch (> 2020-01-01). closes #41
    Serial.print(F("NTP sync"));
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

    engine.begin(ZONES, ZONE_COUNT);
    webServer.begin(WEB_PASSWORD);
    led.begin();

    calendar.begin(ZONES, ZONE_COUNT);
    weather.begin(WEATHER_CITY, WEATHER_API_KEY);
    nobo.begin(NOBO_HUB_IP, NOBO_HUB_SERIAL);

    Serial.println(F("NoboGoogleCal ready."));
}

void loop() {
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
    ensureNoboProfiles();
}
