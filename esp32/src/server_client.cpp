#include "server_client.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

static constexpr int TEMP_PIN = 13;
static constexpr unsigned long POLL_INTERVAL = 2000;

static const char* s_serverUrl = nullptr;
static unsigned long s_lastPollTime = 0;

void serverClientInit(const char* serverUrl) {
    s_serverUrl = serverUrl;
    analogReadResolution(12);  // 12-bit ADC (0–4095)
}

//POST to server to send sensor data and recieve commands
void serverClientPoll(String& patternOut) {
    if (WiFi.status() != WL_CONNECTED) return;
    if (millis() - s_lastPollTime < POLL_INTERVAL) return;

    int raw = analogRead(TEMP_PIN);
    float voltage = (raw / 4095.0f) * 3.3f;
    float temperature = (voltage - 0.5f) / 0.01f;  // TMP36: Vout = 500mV + 10mV/C

    HTTPClient http;
    http.begin(s_serverUrl);
    http.addHeader("Content-Type", "application/json");

    JsonDocument reqDoc;
    reqDoc["temperature"] = temperature;
    String body;
    serializeJson(reqDoc, body);

    int httpCode = http.POST(body);

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        JsonDocument resDoc;
        DeserializationError err = deserializeJson(resDoc, payload);
        if (!err) {
            // expected response: { "command": { "pattern": "<name>" } }
            const char* newPattern = resDoc["command"]["pattern"];
            if (newPattern) {
                patternOut = newPattern; //patternOut is a byRef for global s_currentPattern in main.cpp
            }
        }
    } else {
        Serial.print("HTTP error: ");
        Serial.println(httpCode);
    }

    http.end();
    s_lastPollTime = millis();
}
