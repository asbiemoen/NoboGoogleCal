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
  ├── WebServer       →  Local browser dashboard
  └── LEDDisplay      →  Built-in LED matrix
```

## Scheduled operations

All operations run from the main `loop()` — there is no RTOS or interrupt scheduling. Each component tracks its own last-run timestamp and fires when its interval has elapsed.

| Operation | Interval | Notes |
|---|---|---|
| Calendar sync | 60 min | Staggered: one zone per minute to avoid back-to-back blocking fetches |
| Schedule evaluation | 60 sec | Reads current events and sends comfort/eco overrides to Nobø |
| Weather update | 3 hours | Fetches temperature from OpenWeatherMap; uses seasonal fallback on failure |
| Nobø keepalive | 30 sec | Sends a `Y02` ping to keep the TCP connection alive |
| Nobø reconnect | 30 sec | Retries connection automatically when disconnected |
| NTP sync | On update | `NTPClient` updates passively; clock is re-anchored when a valid epoch arrives |
| EEPROM save | Once per day | Persists the current event list to flash after the first successful sync of the day |

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
- **Weather integration** — skips Comfort if average daytime temperature exceeds 10 °C (Oslo default)
- **Weather fallback** — if the weather API is unreachable, Comfort is skipped in June, July and August
- **Local web dashboard** — shows zone status and the next 7 days of events; open on the local network
- **Password-protected settings** — change per-zone settings via the web UI (default password: `password`)
- **LED display** — scrolling status on the Arduino's built-in LED matrix
- **Startup provisioning** — creates Nobø weekly programs automatically if they do not already exist

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
├── NoboGoogleCal.ino       # Main sketch
├── arduino_secrets.h       # WiFi credentials — gitignored
├── config.h                # Installation-specific config — gitignored
├── config.example.h        # Config template with placeholders — committed
├── src/
│   ├── CalendarManager.h/cpp   # ICS fetching and parsing
│   ├── NoboController.h/cpp    # Nobø TCP protocol
│   ├── ScheduleEngine.h/cpp    # Core scheduling logic
│   ├── WeatherService.h/cpp    # OpenWeatherMap + fallback
│   ├── WebServer.h/cpp         # HTTP server and dashboard
│   └── LEDDisplay.h/cpp        # LED matrix output
└── .gitignore
```

## Setup

### 1. WiFi credentials

Copy `arduino_secrets.example.h` to `arduino_secrets.h` and fill in your values:

```cpp
#define SECRET_SSID  "your-wifi-ssid"
#define SECRET_PASS  "your-wifi-password"
```

### 2. Installation config

Copy `config.example.h` to `config.h` and fill in your values:

```cpp
#define NOBO_HUB_IP   "192.168.x.x"
#define WEB_PASSWORD  "password"
#define WEATHER_CITY  "Oslo"
#define WEATHER_API_KEY "your-openweathermap-key"
```

Add your zones — one per Google Calendar. Get the private ICS URL from **Google Calendar → Settings → Integrate calendar → Secret address in iCal format**.

### 3. Nobø hub

The Arduino must be on the same local network as the Nobø Energy Hub. Zone-to-weekly-program assignment is done manually in the Nobø app once; the Arduino manages the weekly programs from that point on.

## Web dashboard

Open `http://<arduino-ip>/` in a browser on your local network.

- **Dashboard** (open): zone status, current heating state, next 7 days of events, weather
- **Settings** (password-protected): pre-heat hours, default status, weather city, web password

## License

MIT
