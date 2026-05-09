#include <WiFiS3.h>
#include <ArduinoOTA.h>
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

// ─── OTA ──────────────────────────────────────────────────────────────────────
static void setupOTA() {
    ArduinoOTA.setHostname("nobogooglecal");
    ArduinoOTA.setPassword(WEB_PASSWORD);  // same password as web settings

    ArduinoOTA.onStart([]() {
        Serial.println(F("OTA update starting..."));
    });
    ArduinoOTA.onEnd([]() {
        Serial.println(F("OTA update complete — rebooting"));
    });
    ArduinoOTA.onError([](ota_error_t e) {
        Serial.print(F("OTA error: "));
        Serial.println(e);
    });

    ArduinoOTA.begin();
    Serial.print(F("OTA ready — hostname: nobogooglecal, IP: "));
    Serial.println(WiFi.localIP());
}

// ─── setup / loop ─────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(9600);
    delay(1500);

    connectWifi();
    setupOTA();

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
    // OTA must be handled every iteration
    ArduinoOTA.handle();

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("WiFi lost — reconnecting..."));
        connectWifi();
        ArduinoOTA.begin();
    }

    ntp.update();

    webServer.tick();
    led.tick();
    engine.tick();
    calendar.tick();
    weather.tick();
    nobo.tick();
}
