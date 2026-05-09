#pragma once
// Copy this file to config.h and fill in your values.
// config.h is gitignored — never commit it.
// Zone types (ZoneConfig, ECO, etc.) are defined in src/ScheduleEngine.h

// ─── Nobo Energy Hub ──────────────────────────────────────────────────────────
#define NOBO_HUB_IP   "192.168.x.x"   // Local IP of your Nobø Energy Hub

// ─── Web interface ────────────────────────────────────────────────────────────
#define WEB_PASSWORD  "password"       // Password for the settings page

// ─── Weather (OpenWeatherMap) ─────────────────────────────────────────────────
#define WEATHER_CITY    "Oslo"         // Nearest city for weather lookup
#define WEATHER_API_KEY "your-api-key" // Free key from openweathermap.org

// ─── Zone configuration ───────────────────────────────────────────────────────
// One entry per heating zone. Each zone maps to one Nobø weekly program
// and one Google Calendar ICS feed.
//
// Get the ICS URL from: Google Calendar → ⚙ Settings → Integrate calendar
//                       → "Secret address in iCal format"
//
// ZoneConfig fields: { "Zone name", "ICS URL", preheat_hours, default_status }
//   default_status : ECO | AWAY | NORMAL
//   preheat_hours  : hours before event start to switch to COMFORT

#define ZONE_COUNT 2

// clang-format off
const ZoneConfig ZONES[ZONE_COUNT] = {
    { "TMS Hovedsal",         "https://calendar.google.com/calendar/ical/CALENDAR_ID/private-SECRET/basic.ics", 1, ECO },
    { "TMS-RAST-Ungdomsrom",  "https://calendar.google.com/calendar/ical/CALENDAR_ID/private-SECRET/basic.ics", 1, ECO },
};
// clang-format on
