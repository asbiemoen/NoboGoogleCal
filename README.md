# NoboGoogleCal

Control [Nobø Energy Hub](https://www.nobo.no/en/) heating zones automatically using Google Calendar — running on an **Arduino Uno R4 WiFi** with **Arduino Cloud** support.

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
        │
        ▼ (Arduino Cloud)
  OTA updates + remote monitoring
```

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
- **Arduino Cloud** — OTA firmware updates and remote monitoring via cloud variables
- **Startup provisioning** — creates Nobø weekly programs automatically if they do not already exist

## Hardware

| Component | Details |
|---|---|
| Microcontroller | Arduino Uno R4 WiFi |
| Heating controller | Nobø Energy Hub (local network, TCP port 27779) |
| Cloud | Arduino Cloud (OTA + monitoring) |

## Libraries required

- `ArduinoIoTCloud` + `Arduino_ConnectionHandler` — Arduino Cloud / OTA
- `ArduinoHttpClient` — HTTPS requests
- `ArduinoJson` — JSON parsing (weather API)
- `Arduino_LED_Matrix` + `ArduinoGraphics` — LED matrix display
- `NTPClient` — time synchronisation

## Project structure

```
NoboGoogleCal/
├── NoboGoogleCal.ino       # Main sketch
├── arduino_secrets.h       # WiFi credentials — gitignored (Arduino Cloud)
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

### 1. Arduino Cloud

Create a new Thing in [Arduino Cloud](https://create.arduino.cc/iot). The following variables will be auto-generated in `thingProperties.h`:

| Variable | Type | Description |
|---|---|---|
| `statusMessage` | String | Current system status |
| `outsideTemp` | float | Outside temperature from weather API |
| `lastSync` | String | Timestamp of last calendar sync |
| `nextEvent` | String | Next upcoming event across all zones |

OTA is enabled automatically when the device is connected to Arduino Cloud.

### 2. WiFi credentials

Arduino Cloud generates `arduino_secrets.h` automatically. It is gitignored.

```cpp
#define SECRET_SSID           "your-wifi-ssid"
#define SECRET_OPTIONAL_PASS  "your-wifi-password"
```

### 3. Installation config

Copy `config.example.h` to `config.h` and fill in your values:

```cpp
#define NOBO_HUB_IP   "192.168.x.x"
#define WEB_PASSWORD  "password"
#define WEATHER_CITY  "Oslo"
#define WEATHER_API_KEY "your-openweathermap-key"
```

Add your zones — one per Google Calendar. Get the private ICS URL from **Google Calendar → Settings → Integrate calendar → Secret address in iCal format**.

### 4. Nobø hub

The Arduino must be on the same local network as the Nobø Energy Hub. Zone-to-weekly-program assignment is done manually in the Nobø app once; the Arduino manages the weekly programs from that point on.

## Web dashboard

Open `http://<arduino-ip>/` in a browser on your local network.

- **Dashboard** (open): zone status, current heating state, next 7 days of events, weather
- **Settings** (password-protected): pre-heat hours, default status, weather city, web password

## License

MIT
