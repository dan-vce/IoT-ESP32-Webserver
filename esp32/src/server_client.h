#pragma once
#include <Arduino.h>

// Initialise the server client. Must be called once when entering NORMAL mode.
// Stores the server URL and configures the ADC resolution for the temp sensor.
void serverClientInit(const char* serverUrl);

// Read temperature, POST it to the server, and write the returned pattern name
// into patternOut if the server provides one. Rate-limited to POLL_INTERVAL ms.
// Does nothing if WiFi is not connected.
void serverClientPoll(String& patternOut);
