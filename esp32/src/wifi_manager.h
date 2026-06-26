#pragma once
#include <Arduino.h>

enum class WifiStatus {
    CONNECTED,
    NO_CREDENTIALS,
    CONNECT_FAILED,
    HAS_CREDENTIALS
};

// Initialise WiFi manager. Call once at the start of setup().
void wifiManagerInit();

// Returns NO_CREDENTIALS if no SSID/password are stored in NVS,
// otherwise CONNECTED (as a hint that credentials exist, does not attempt connection).
WifiStatus wifiManagerHasCredentials();

// Read stored credentials from NVS and attempt WiFi.begin().
// Returns CONNECTED on success or CONNECT_FAILED after a 15-second timeot.
WifiStatus wifiManagerConnectSaved();

// Switch to AP mode, start DNS server and web server for captive portal.
void wifiManagerStartAP();

// Process one tick of the captive portal. Call every loop() iteration while
// in PROVISIONING mode.
void wifiManagerHandleAP();

// Persist credentials to NVS. Returns true on success.
bool wifiManagerSaveCredentials(const String& ssid, const String& password);

// Erase stored credentials from NVS (used before falling back to provisioning).
void wifiManagerClearCredentials();
