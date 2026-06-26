#pragma once
#include <Arduino.h>

constexpr const char* LIGHTS_DEFAULT_PATTERN = "chase";

//pin assignments
constexpr int LED_PINS[]    = { 12, 11, 10, 9, 6, 5 };
constexpr int NUM_LEDS      = 6;
constexpr int RED_LEDS[]    = { 12, 9 };
constexpr int GREEN_LEDS[]  = { 11, 6 };
constexpr int YELLOW_LEDS[] = { 10, 5 };
constexpr int NUM_RED       = 2;
constexpr int NUM_GREEN     = 2;
constexpr int NUM_YELLOW    = 2;

//public API
void lightsInit();
void lightsSetPattern(const String& pattern);
void lightsUpdate();