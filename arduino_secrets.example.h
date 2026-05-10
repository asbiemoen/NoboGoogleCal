#pragma once
// Copy this file to arduino_secrets.h and fill in your values.
// arduino_secrets.h is gitignored — never commit it.

// ─── WiFi ─────────────────────────────────────────────────────────────────────
#define SECRET_SSID  "your-wifi-ssid"
#define SECRET_PASS  "your-wifi-password"

// Optional: secondary network tried if primary fails (remove the // to enable)
// #define SECRET_SSID2 "backup-wifi-ssid"
// #define SECRET_PASS2 "backup-wifi-password"

// ─── Arduino Cloud ────────────────────────────────────────────────────────────
// Device ID from create.arduino.cc/iot/devices
// Secret Key is stored in board NVM during provisioning — not needed here
#define SECRET_DEVICE_ID  "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
