#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
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
#include "config.h"  // must come after Types.h (uses ZoneConfig, HeatingStatus)

// ─── Arduino Cloud variables ──────────────────────────────────────────────────
String cloudStatus;
float  cloudTemp;
String cloudLastSync;
String cloudNextEvent;

// ─── NTP ──────────────────────────────────────────────────────────────────────
static WiFiUDP   ntpUdp;
static NTPClient ntp(ntpUdp, "pool.ntp.org", 3600);  // UTC+1 default (winter)

// ─── Components ───────────────────────────────────────────────────────────────
static NoboController  nobo;
static CalendarManager calendar;
static WeatherService  weather;
static ScheduleEngine  engine(nobo, calendar, weather);
static AppWebServer    webServer(engine, weather);
static LEDDisplay      led(engine, weather);

// ─── Arduino Cloud property registration ─────────────────────────────────────
static void initCloudProperties() {
    ArduinoCloud.setBoardId(SECRET_DEVICE_ID);
    ArduinoCloud.setSecretDeviceKey(SECRET_DEVICE_KEY);
    ArduinoCloud.addProperty(cloudStatus,    READ, ON_CHANGE, nullptr);
    ArduinoCloud.addProperty(cloudTemp,      READ, ON_CHANGE, nullptr);
    ArduinoCloud.addProperty(cloudLastSync,  READ, ON_CHANGE, nullptr);
    ArduinoCloud.addProperty(cloudNextEvent, READ, ON_CHANGE, nullptr);
}

void setup() {
    Serial.begin(9600);
    delay(1500);

    initCloudProperties();

    static WiFiConnectionHandler conn(SECRET_SSID, SECRET_PASS);
    ArduinoCloud.begin(conn);
    ArduinoCloud.printDebugInfo();

    // Sync RTC from NTP once WiFi is up
    ntp.begin();
    ntp.update();
    struct timeval tv { (time_t)ntp.getEpochTime(), 0 };
    settimeofday(&tv, nullptr);

    nobo.begin(NOBO_HUB_IP, NOBO_HUB_SERIAL);

    // Provision week profiles for each configured zone
    for (int i = 0; i < ZONE_COUNT; i++) {
        nobo.ensureProfileExists(ZONES[i].name);
    }

    calendar.begin(ZONES, ZONE_COUNT);
    weather.begin(WEATHER_CITY, WEATHER_API_KEY);
    engine.begin(ZONES, ZONE_COUNT);
    webServer.begin(WEB_PASSWORD);
    led.begin();
}

void loop() {
    // Arduino Cloud update must run every iteration for OTA to work
    ArduinoCloud.update();
    ntp.update();

    webServer.tick();
    led.tick();
    engine.tick();
    calendar.tick();
    weather.tick();
    nobo.tick();

    // Push state to Arduino Cloud
    cloudStatus    = engine.statusString();
    cloudTemp      = weather.currentTemp();
    cloudLastSync  = calendar.lastSyncTime();
    cloudNextEvent = engine.nextEventString();
}
