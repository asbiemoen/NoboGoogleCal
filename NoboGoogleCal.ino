#include <WiFiS3.h>
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
static NTPClient ntp(ntpUdp, "pool.ntp.org", 3600);

// ─── Components ───────────────────────────────────────────────────────────────
static NoboController  nobo;
static CalendarManager calendar;
static WeatherService  weather;
static ScheduleEngine  engine(nobo, calendar, weather);
static AppWebServer    webServer(engine, weather);
static LEDDisplay      led(engine, weather);

// ─── WiFi ─────────────────────────────────────────────────────────────────────
static void connectWifi() {
    Serial.print(F("Connecting to WiFi"));
    WiFi.begin(SECRET_SSID, SECRET_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print('.');
    }
    Serial.println();
    Serial.print(F("IP: "));
    Serial.println(WiFi.localIP());
}

// ─── setup / loop ─────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(9600);
    delay(1500);

    connectWifi();

    ntp.begin();
    ntp.update();

    struct timeval tv { (time_t)ntp.getEpochTime(), 0 };
    settimeofday(&tv, nullptr);

    nobo.begin(NOBO_HUB_IP, NOBO_HUB_SERIAL);

    for (int i = 0; i < ZONE_COUNT; i++) {
        nobo.ensureProfileExists(ZONES[i].name);
    }

    calendar.begin(ZONES, ZONE_COUNT);
    weather.begin(WEATHER_CITY, WEATHER_API_KEY);
    engine.begin(ZONES, ZONE_COUNT);
    webServer.begin(WEB_PASSWORD);
    led.begin();

    Serial.println(F("NoboGoogleCal ready."));
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("WiFi lost — reconnecting..."));
        connectWifi();
    }

    ntp.update();

    webServer.tick();
    led.tick();
    engine.tick();
    calendar.tick();
    weather.tick();
    nobo.tick();
}
