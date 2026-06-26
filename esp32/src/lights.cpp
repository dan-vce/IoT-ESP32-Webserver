#include "lights.h"

// timing constants (ms)
static constexpr int CHASE_DELAY = 100;
static constexpr int BLINK_DELAY = 500;
static constexpr int ALT_DELAY = 600;
static constexpr int FLICKER_STEPS = 40;

// module state
static String s_pattern = LIGHTS_DEFAULT_PATTERN;

// forward declarations
static void allOn();
static void allOff();
static void arrayOn(const int* pins, int count);
static void arrayOff(const int* pins, int count);
static void patternChase();
static void patternBlink();
static void patternAlternating();
static void patternFlicker();
static void patternTwinkle();

// Public API
void lightsInit() {
    for (int i = 0; i < NUM_LEDS; i++) {
        pinMode(LED_PINS[i], OUTPUT);
        digitalWrite(LED_PINS[i], LOW);
    }
}

void lightsSetPattern(const String& pattern) {
    if (pattern == s_pattern) return;
    s_pattern = pattern;
    allOff();
    Serial.print("Pattern changed to: ");
    Serial.println(s_pattern);
}

void lightsUpdate() {
    if (s_pattern == "chase") patternChase();
    else if (s_pattern == "blink") patternBlink();
    else if (s_pattern == "alternating") patternAlternating();
    else if (s_pattern == "flicker") patternFlicker();
    else if (s_pattern == "twinkle") patternTwinkle();
}

//===============HELPERS===============
static void allOff() {
    for (int i = 0; i < NUM_LEDS; i++) {
        digitalWrite(LED_PINS[i], LOW);
    }
}

static void allOn() {
    for (int i = 0; i < NUM_LEDS; i++) {
        digitalWrite(LED_PINS[i], HIGH);
    }
}

static void arrayOn(const int* pins, int count) {
    for (int i = 0; i < count; i++) {
        digitalWrite(pins[i], HIGH);
    }
}

static void arrayOff(const int* pins, int count) {
    for (int i = 0; i < count; i++) {
        digitalWrite(pins[i], LOW);
    }
}

//===============PATTERNS===============
static void patternChase() {
    for (int i = 0; i < NUM_LEDS; i++) {
        allOff();
        digitalWrite(LED_PINS[i], HIGH);
        delay(CHASE_DELAY);
    }
    for (int i = NUM_LEDS - 2; i >= 1; i--) {
        allOff();
        digitalWrite(LED_PINS[i], HIGH);
        delay(CHASE_DELAY);
    }
}

static void patternBlink() {
    allOn();
    delay(BLINK_DELAY);
    allOff();
    delay(BLINK_DELAY);
}

static void patternAlternating() {
    allOff();
    arrayOn(RED_LEDS, NUM_RED);
    delay(ALT_DELAY);
    arrayOff(RED_LEDS, NUM_RED);
    arrayOn(GREEN_LEDS, NUM_GREEN);
    delay(ALT_DELAY);
    arrayOff(GREEN_LEDS, NUM_GREEN);
    arrayOn(YELLOW_LEDS, NUM_YELLOW);
    delay(ALT_DELAY);
    arrayOff(YELLOW_LEDS, NUM_YELLOW);
}

//flicker up from the bottom to simulate fire. LED cannot light if the one before it is not lit
static void patternFlicker() {
    for (int step = 0; step < FLICKER_STEPS; step++) {
        bool extinguished = false;
        for (int i = 0; i < NUM_LEDS; i++) {
            if (extinguished) {
                digitalWrite(LED_PINS[i], LOW);
            } else {
                int threshold = 95 - ((85 * i) / (NUM_LEDS - 1));
                if (random(100) < threshold) {
                    digitalWrite(LED_PINS[i], HIGH);
                } else {
                    digitalWrite(LED_PINS[i], LOW);
                    extinguished = true;
                }
            }
        }
        delay(random(20, 80));
    }
}

//bonus pattern, randomly twinkle LEDs on and off
static void patternTwinkle() {
    for (int step = 0; step < FLICKER_STEPS; step++) {
        int pin = LED_PINS[random(NUM_LEDS)];
        digitalWrite(pin, random(2) ? HIGH : LOW);
        delay(random(20, 80));
    }
}
