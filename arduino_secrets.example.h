#pragma once
// Copy this file to arduino_secrets.h and fill in your values.
// arduino_secrets.h is gitignored — never commit it.

// ─── WiFi ─────────────────────────────────────────────────────────────────────
#define SECRET_SSID  "your-wifi-ssid"
#define SECRET_PASS  "your-wifi-password"

// ─── Arduino Cloud (optional — needed for remote monitoring and cloud OTA) ────
// If you are not using Arduino Cloud, leave these as-is.
// To enable: register device at https://create.arduino.cc/iot/devices
// and paste the Device ID and Secret Key shown during provisioning.
// #define SECRET_DEVICE_ID   "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
// #define SECRET_DEVICE_KEY  "your-arduino-cloud-secret-key"
