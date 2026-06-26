#include <Arduino.h>
#include "lights.h"
#include "wifi_manager.h"
#include "server_client.h"

//Configuration
static const char* SERVER_URL = "http://172.20.10.11:5000/sensor";

//App state
enum class AppMode {PROVISIONING, NORMAL};
static AppMode s_mode = AppMode::PROVISIONING;
static String s_currentPattern = LIGHTS_DEFAULT_PATTERN;

//Setup
void setup() {
    Serial.begin(115200);
    delay(2000); //wait for serial monitor to initialize

    lightsInit();
    wifiManagerInit();

    WifiStatus status = wifiManagerHasCredentials();

    if (status == WifiStatus::NO_CREDENTIALS) {
        Serial.println("No credentials, entering provisioning mode");
        wifiManagerStartAP();
        return;
    }

    WifiStatus conn = wifiManagerConnectSaved();
    if (conn == WifiStatus::CONNECTED) {
        Serial.println("Connected, entering normal mode");
        s_mode = AppMode::NORMAL;
        serverClientInit(SERVER_URL);
    } else {
        Serial.println("Saved credentials failed, re-entering provisioning mode");
        wifiManagerClearCredentials();
        s_mode = AppMode::PROVISIONING;
        wifiManagerStartAP();
    }
}

//Loop
void loop() {
    if (s_mode == AppMode::PROVISIONING) {
        wifiManagerHandleAP();
        return;
    }

    // NORMAL mode: poll server, then run LED pattern
    serverClientPoll(s_currentPattern);
    lightsSetPattern(s_currentPattern);
    lightsUpdate();
}
