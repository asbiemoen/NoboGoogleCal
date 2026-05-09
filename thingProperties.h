#pragma once
#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#include "arduino_secrets.h"

// Cloud variables — pushed to Arduino Cloud every time they change
String cloudStatus;
float  cloudOutsideTemp;
String cloudLastSync;
String cloudNextEvent;

void initProperties() {
    ArduinoCloud.setBoardId(SECRET_DEVICE_ID);
    // Secret Key is stored in board NVM during provisioning — no need to set here
    ArduinoCloud.addProperty(cloudStatus,      READ, ON_CHANGE, 30 * SECONDS);
    ArduinoCloud.addProperty(cloudOutsideTemp, READ, ON_CHANGE, 30 * SECONDS);
    ArduinoCloud.addProperty(cloudLastSync,    READ, ON_CHANGE, 30 * SECONDS);
    ArduinoCloud.addProperty(cloudNextEvent,   READ, ON_CHANGE, 30 * SECONDS);
}

WiFiConnectionHandler ArduinoIoTPreferredConnection(SECRET_SSID, SECRET_PASS);
