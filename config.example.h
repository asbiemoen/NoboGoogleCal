#pragma once
// Copy this file to config.h and fill in your values.
// config.h is gitignored — never commit it.
// Zone types (ZoneConfig, ECO, etc.) are defined in src/ScheduleEngine.h

// ─── Nobo Energy Hub ──────────────────────────────────────────────────────────
#define NOBO_HUB_IP     "192.168.x.x"      // Local IP of your Nobø Energy Hub
#define NOBO_HUB_SERIAL "123456789012"     // 12-digit serial on the hub label (only first 3 digits used in handshake)

// ─── Web interface ────────────────────────────────────────────────────────────
#define WEB_PASSWORD  "password"       // Password for the settings page
#define SITE_TITLE    "Heating Controller"  // Displayed in browser tab and page header
#define MDNS_NAME     "varme"          // Board reachable at varme.local on the local network

// ─── Weather (OpenWeatherMap) ─────────────────────────────────────────────────
#define WEATHER_CITY    "Oslo"         // Nearest city for weather lookup
#define WEATHER_API_KEY "your-api-key" // Free key from openweathermap.org

// ─── Email notifications (Resend.com) ────────────────────────────────────────
// All fields are optional compile-time defaults; they can be overridden at
// runtime via the Settings page without reflashing.
// Sign up for a free API key at resend.com (100 emails/day free tier).
// #define RESEND_API_KEY   "re_..."
// #define RESEND_FROM      "noreply@yourdomain.com"
// #define RESEND_TO        "you@example.com"
// #define EMAIL_DAILY_TIME "07:00"   // HH:MM Norway local time

// ─── Zone configuration ───────────────────────────────────────────────────────
// One entry per heating zone. Each zone maps to one Nobø weekly program
// and one Google Calendar ICS feed.
//
// Get the ICS URL from: Google Calendar → ⚙ Settings → Integrate calendar
//                       → "Secret address in iCal format"
//
// ZoneConfig fields: { "Zone name", "ICS URL", preheat_hours, default_status, "Event label" }
//   default_status : STATUS_ECO | STATUS_AWAY | STATUS_NORMAL
//   preheat_hours  : hours before event start to switch to COMFORT
//   event_label    : text shown when Google Calendar returns "Busy" (public calendar privacy setting)

#define ZONE_COUNT 2

// clang-format off
const ZoneConfig ZONES[ZONE_COUNT] = {
    { "Main Hall",   "https://calendar.google.com/calendar/ical/CALENDAR_ID/public/basic.ics", 1, STATUS_ECO, "Church event" },
    { "Youth Room",  "https://calendar.google.com/calendar/ical/CALENDAR_ID/public/basic.ics", 1, STATUS_ECO, "Youth event"  },
};
// clang-format on
