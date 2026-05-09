#include <WiFiS3.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>

#include "arduino_secrets.h"
#include "thingProperties.h"
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

// ─── Cloud variable sync ──────────────────────────────────────────────────────
static uint32_t _lastCloudSyncMs = 0;

static void syncCloudVariables() {
    if (millis() - _lastCloudSyncMs < 30000UL) return;
    _lastCloudSyncMs   = millis();
    cloudStatus        = engine.statusString();
    cloudOutsideTemp   = weather.currentTemp();
    cloudLastSync      = calendar.lastSyncTime();
    cloudNextEvent     = engine.nextEventString();
}

// ─── WiFi ─────────────────────────────────────────────────────────────────────
// Returns true if connected. Makes one bounded attempt (20 × 500 ms = 10 s).
static bool connectWifi() {
    Serial.print(F("Connecting to WiFi"));
    WiFi.begin(SECRET_SSID, SECRET_PASS);
    for (uint8_t i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
        delay(500);
        Serial.print('.');
    }
    Serial.println();
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("WiFi connect failed"));
        return false;
    }
    Serial.print(F("IP: "));
    Serial.println(WiFi.localIP());
    return true;
}

// ─── setup / loop ─────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(9600);
    delay(1500);

    while (!connectWifi()) { /* retry until network is up */ }

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

    initProperties();
    ArduinoCloud.begin(ArduinoIoTPreferredConnection);
    ArduinoCloud.printDebugInfo();

    Serial.println(F("NoboGoogleCal ready."));
}

void loop() {
    // ArduinoCloud handles WiFi reconnection internally
    ArduinoCloud.update();

    ntp.update();

    webServer.tick();
    led.tick();
    engine.tick();
    calendar.tick();
    weather.tick();
    nobo.tick();

    syncCloudVariables();
}
