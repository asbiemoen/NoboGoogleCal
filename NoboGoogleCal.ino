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
static bool connectWifi() {
    Serial.print(F("Connecting to WiFi"));
    WiFi.begin(SECRET_SSID, SECRET_PASS);
    for (uint8_t i = 0; i < 40; i++) {
        if (WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0)) break;
        delay(500);
        Serial.print('.');
    }
    Serial.println();
    if (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0, 0, 0, 0)) {
        Serial.println(F("WiFi connect failed"));
        return false;
    }
    Serial.print(F("IP: "));
    Serial.println(WiFi.localIP());
    return true;
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

    RTCTime startTime(epoch);
    RTC.setTime(startTime);
    struct timeval tv = { epoch, 0 };
    settimeofday(&tv, nullptr);

    // Print time so we can verify it on the serial monitor
    {
        time_t now = time(nullptr);
        struct tm* utc = gmtime(&now);
        int off = norwayOffsetSeconds(utc);
        time_t localNow = now + off;
        struct tm* loc = gmtime(&localNow);
        char tbuf[40];
        snprintf(tbuf, sizeof(tbuf), "Time (UTC):   %04d-%02d-%02d %02d:%02d:%02d",
                 utc->tm_year+1900, utc->tm_mon+1, utc->tm_mday,
                 utc->tm_hour, utc->tm_min, utc->tm_sec);
        Serial.println(tbuf);
        snprintf(tbuf, sizeof(tbuf), "Time (local): %04d-%02d-%02d %02d:%02d:%02d (UTC+%d)",
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
    ntp.update();
    engine.tick();
    calendar.tick();
    weather.tick();
    nobo.tick();
    ensureNoboProfiles();
}
