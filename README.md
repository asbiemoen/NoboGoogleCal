# NoboGoogleCal

Control [Nobø Energy Hub](https://www.nobo.no/en/) heating zones automatically using Google Calendar — running on an **Arduino Uno R4 WiFi**.

## How it works

Each heating zone on the Nobø hub is linked to a Google Calendar. The Arduino fetches the ICS feed from each calendar once per hour and builds a 7-day schedule. A configurable number of hours before each event starts, the zone switches to **Comfort** mode. When the event ends, the zone resets to its default status (typically **Eco**).

```
Google Calendar (ICS feed)
        │
        ▼ (HTTPS, every 60 min)
Arduino Uno R4 WiFi
  ├── ScheduleEngine  →  Nobø Energy Hub (TCP :27779)
  ├── WeatherService  →  OpenWeatherMap API
  ├── AppWebServer    →  Local browser dashboard  (http://heat.local)
  ├── MiniMDNS        →  mDNS announcements (heat.local)
  ├── EmailService    →  Daily summary email via Resend.com
  └── LEDDisplay      →  Built-in LED matrix
```

## Scheduled operations

All operations run from the main `loop()` — there is no RTOS or interrupt scheduling. Each component tracks its own last-run timestamp and fires when its interval has elapsed.

| Operation | Interval | Notes |
|---|---|---|
| Calendar sync | 60 min | Staggered: one zone per minute to avoid back-to-back blocking fetches |
| Schedule evaluation | 60 sec | Reads current events and sends comfort/eco overrides to Nobø |
| Weather update | 3 hours | Fetches daily average temperature from OpenWeatherMap; uses seasonal fallback on failure |
| Nobø keepalive | 30 sec | Sends a `Y02` ping to keep the TCP connection alive |
| Nobø reconnect | 30 sec | Retries connection automatically when disconnected |
| NTP sync | On update | `NTPClient` updates passively; clock is re-anchored when a valid epoch arrives |
| mDNS announce | 60 sec | Broadcasts an unsolicited A-record for `<hostname>.local` |
| Daily email | Configurable | Sends a heating summary via Resend.com at a user-set time |

### Calendar sync in detail

Each zone's ICS feed is fetched independently on a staggered schedule (zone 0 at minute 0, zone 1 at minute 1, etc.) so the loop is never blocked by two simultaneous downloads. A single fetch can take up to 15 seconds for a 13 KB feed; a 15-second stall detector breaks out early if the server stops sending data before TCP closes cleanly.

On boot the last saved event list is loaded from EEPROM so the schedule is available immediately, before the first live sync completes.

### Nobø connection

The Arduino connects to the Nobø Energy Hub over TCP port 27779 using the Nobø Hub Protocol v1.1. If the connection drops, it retries every 30 seconds. Calendar events continue to be fetched and evaluated while the hub is offline; overrides are applied as soon as the connection is re-established.

## Features

- **N zones** — one Google Calendar per heating zone (configurable)
- **Pre-heating** — switches to Comfort X hours before an event (default: 1 hour, configurable per zone)
- **Auto-reset** — returns to default status (Eco) when the event ends
- **Removed events** — detected on each sync; zone resets to default
- **Overlap handling** — overlapping events merge into a single Comfort period
- **Hourly sync** — fetches next 7 days from all calendars
- **Weather integration** — skips Comfort if average daily temperature exceeds 10 °C
- **Weather fallback** — uses seasonal logic when the API is unreachable
- **mDNS** — board reachable at `heat.local` on the local network (configurable)
- **Local web dashboard** — zone status, 7-day event view, sync status per zone, next-event countdown
- **Session login** — dashboard is public; Settings page requires a password
- **Runtime settings** — WiFi credentials, Nobø IP, weather city, hostname, email — all editable via the web UI and persisted to EEPROM; no reflash needed
- **Daily email** — optional summary via [Resend.com](https://resend.com) with heating status and upcoming events
- **LED display** — scrolling status, IP address at boot, heating countdown on the built-in LED matrix
- **Startup provisioning** — creates Nobø weekly programs automatically if they do not already exist
- **Timestamped serial logging** — all log output includes Norway local time

## Hardware

| Component | Details |
|---|---|
| Microcontroller | Arduino Uno R4 WiFi |
| Heating controller | Nobø Energy Hub (local network, TCP port 27779) |

## Libraries required

- `ArduinoHttpClient` — HTTPS requests
- `ArduinoJson` — JSON parsing (weather API)
- `Arduino_LED_Matrix` + `ArduinoGraphics` — LED matrix display
- `NTPClient` — time synchronisation

## Project structure

```
NoboGoogleCal/
├── NoboGoogleCal.ino           # Main sketch
├── arduino_secrets.h           # WiFi credentials — gitignored
├── config.h                    # Installation-specific config — gitignored
├── config.example.h            # Config template with placeholders — committed
├── src/
│   ├── Types.h                 # Shared types (ZoneConfig, CalEvent, …)
│   ├── NVMConfig.h             # EEPROM layout and load/save helpers
│   ├── AppLog.h/cpp            # Timestamped serial logging
│   ├── CalendarManager.h/cpp   # ICS fetching and parsing
│   ├── NoboController.h/cpp    # Nobø TCP protocol
│   ├── ScheduleEngine.h/cpp    # Core scheduling logic
│   ├── WeatherService.h/cpp    # OpenWeatherMap + fallback
│   ├── AppWebServer.h/cpp      # HTTP server, dashboard and settings
│   ├── EmailService.h/cpp      # Daily summary email (Resend.com)
│   ├── MiniMDNS.h/cpp          # Lightweight mDNS announcer
│   └── LEDDisplay.h/cpp        # LED matrix output
└── .gitignore
```

## Setup

### 1. WiFi credentials

Copy `arduino_secrets.example.h` to `arduino_secrets.h` and fill in your values:

```cpp
#define SECRET_SSID  "your-wifi-ssid"
#define SECRET_PASS  "your-wifi-password"
// Optional secondary network:
// #define SECRET_SSID2 "backup-ssid"
// #define SECRET_PASS2 "backup-pass"
```

### 2. Installation config

Copy `config.example.h` to `config.h` and fill in your values:

```cpp
#define NOBO_HUB_IP      "192.168.x.x"
#define NOBO_HUB_SERIAL  "123456789012"
#define WEB_PASSWORD     "password"
#define MDNS_NAME        "heat"          // reachable at heat.local
#define WEATHER_CITY     "Oslo"
#define WEATHER_API_KEY  "your-openweathermap-key"
```

Add your zones — one per Google Calendar. Get the private ICS URL from **Google Calendar → Settings → Integrate calendar → Secret address in iCal format**.

```cpp
#define ZONE_COUNT 2
const ZoneConfig ZONES[ZONE_COUNT] = {
    { "Main Hall",  "https://calendar.google.com/calendar/ical/…/basic.ics", 1, STATUS_ECO, "Event" },
    { "Youth Room", "https://calendar.google.com/calendar/ical/…/basic.ics", 1, STATUS_ECO, "Event" },
};
```

### 3. Nobø hub

The Arduino must be on the same local network as the Nobø Energy Hub. Zone-to-weekly-program assignment is done manually in the Nobø app once; the Arduino manages the weekly programs from that point on.

### 4. Email notifications (optional)

Sign up for a free API key at [resend.com](https://resend.com) (100 emails/day on the free tier). Add to `config.h`:

```cpp
#define RESEND_API_KEY   "re_..."
#define RESEND_FROM      "noreply@yourdomain.com"
#define RESEND_TO        "you@example.com"
#define EMAIL_DAILY_TIME "07:00"   // HH:MM Norway local time
```

These are compile-time defaults; they can also be set at runtime via the Settings page without reflashing.

## Web dashboard

Open `http://heat.local/` (or `http://<arduino-ip>/`) in a browser on your local network.

| Page | Access | Contents |
|---|---|---|
| Dashboard `/` | Public | Zone status with colored borders, 7-day timeline, next-event countdown, weather, activity log |
| Login `/login` | — | Password prompt |
| Settings `/settings` | Password | WiFi, Nobø, weather, mDNS hostname, email notifications — saved to EEPROM |

Zone cards use colored top borders to show the current heating status at a glance:
- **Orange** — Comfort (heating active)
- **Blue** — Eco
- **Muted blue** — Away

Settings changes that affect WiFi or Nobø connectivity trigger an automatic reboot. All other changes (weather city, email, hostname) take effect immediately.

## License

MIT
