#include "led.h"
#include "settings.h"
#include <Arduino.h>

#define RGB_PIN PIN_WS2812
#define BRIGHTNESS_MAX 255

extern "C" void neopixelWrite(uint8_t pin, uint8_t r, uint8_t g, uint8_t b);

// State
static LedMode currentMode = LedMode::BootBlink;
static uint32_t staticColor = 0xFFFFFF;
static uint8_t ledBrightness = BRIGHTNESS_MAX;

static unsigned long lastAnim = 0;
static bool blink = false;
static uint8_t breathe = 0;
static int breatheDir = 1;
static bool staticDirty = true;

// Heartbeat
static bool heartbeatActive = false;
static uint32_t heartbeatColor = 0xFF0055;
static uint8_t heartbeatPulses = 2;
static uint8_t heartbeatPhase = 0;
static unsigned long heartbeatPhaseStart = 0;

// -- Color helpers (with brightness) --
static void setLedColor(uint8_t r, uint8_t g, uint8_t b) {
    float scale = ledBrightness / 255.0f;
    neopixelWrite(RGB_PIN, g * scale, r * scale, b * scale);
}
static void setLedColor32(uint32_t color) {
    setLedColor((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
}

// -- Public API --
void Led::begin() {
    neopixelWrite(RGB_PIN, 0, 0, 0);
    lastAnim = millis();
    blink = false;
    breathe = 0;
    breatheDir = 1;
    staticDirty = true;
    heartbeatActive = false;
    currentMode = LedMode::BootBlink;
}
void Led::setMode(LedMode m) {
    if (m == currentMode && !heartbeatActive) return;
    currentMode = m;
    lastAnim = millis();
    blink = false;
    breathe = 0;
    breatheDir = 1;
    staticDirty = true;
    heartbeatActive = false;
    // Show first frame instantly
    switch (m) {
      case LedMode::BootBlink: setLedColor(ledBrightness, ledBrightness, ledBrightness); break;
      case LedMode::BreathePink: setLedColor(255, 105, 180); break;
      case LedMode::StaticColor: setLedColor32(staticColor); break;
      case LedMode::RainbowFade: setLedColor(ledBrightness, 0, 0); break;
      default: break;
    }
}
LedMode Led::getMode() { return currentMode; }

void Led::setColor(uint32_t rgb) {
    staticColor = rgb;
    staticDirty = true;
    if (currentMode == LedMode::StaticColor && !heartbeatActive) {
        setLedColor32(staticColor); // Immediate update!
        staticDirty = false;
    }
}
uint32_t Led::getColor() { return staticColor; }

void Led::setBrightness(uint8_t b) {
    ledBrightness = b;
    staticDirty = true;
    if (!heartbeatActive) {
        // For all modes: update immediately.
        switch (currentMode) {
            case LedMode::StaticColor:
                setLedColor32(staticColor); break;
            case LedMode::BootBlink:
                setLedColor(ledBrightness, ledBrightness, ledBrightness); break;
            case LedMode::BreathePink:
                setLedColor(255, 105, 180); break;
            case LedMode::RainbowFade:
                setLedColor(ledBrightness, 0, 0); break;
            default: break;
        }
        staticDirty = false;
    }
}
uint8_t Led::getBrightness() { return ledBrightness; }

// ========== HEARTBEAT ==========
void Led::heartbeat(uint32_t rgb, uint8_t pulses) {
    heartbeatActive = true;
    heartbeatColor = rgb;
    heartbeatPulses = pulses ? pulses : 2;
    heartbeatPhase = 0;
    heartbeatPhaseStart = millis();
}

// ========== MAIN LOOP ==========
void Led::loop() {
    unsigned long now = millis();

    // --- Heartbeat interrupts all modes ---
    if (heartbeatActive) {
        // Pulse: ON, OFF, ON, OFF, ... (2 pulses default)
        static const uint16_t beatDur[] = { 80, 80, 80, 300 };
        uint8_t beatCount = heartbeatPulses * 2;
        if (heartbeatPhase < beatCount) {
            if (now - heartbeatPhaseStart >= beatDur[heartbeatPhase % 4]) {
                bool on = (heartbeatPhase % 2 == 0);
                setLedColor(
                    ((heartbeatColor >> 16) & 0xFF) * on,
                    ((heartbeatColor >> 8) & 0xFF) * on,
                    (heartbeatColor & 0xFF) * on
                );
                heartbeatPhase++;
                heartbeatPhaseStart = now;
            }
        } else {
            heartbeatActive = false;
            heartbeatPhase = 0;
            setMode(currentMode);
        }
        return;
    }

    switch (currentMode) {
      case LedMode::BootBlink:
        if (now - lastAnim > 300) {
            blink = !blink;
            uint8_t v = blink ? ledBrightness : 0;
            setLedColor(v, v, v);
            lastAnim = now;
        }
        break;

      case LedMode::BreathePink:
        if (now - lastAnim > 12) {
            breathe += breatheDir;
            if (breathe == 0 || breathe == 127) breatheDir = -breatheDir;
            // Pink: R full, G/B tuned for "hot pink"
            uint8_t r = breathe * 2;
            uint8_t g = (uint8_t)(breathe * 0.41); // ~105/255
            uint8_t b = (uint8_t)(breathe * 1.13); // ~180/255
            setLedColor(r, g, b);
            lastAnim = now;
        }
        break;

      case LedMode::StaticColor:
        if (staticDirty) {
            setLedColor32(staticColor);
            staticDirty = false;
        }
        break;

      case LedMode::RainbowFade:
        if (now - lastAnim > 8) {
            static uint16_t hue = 0;
            hue = (hue + 2) % 1536;
            uint8_t r = 0, g = 0, b = 0;
            uint16_t h = hue;

            if (h < 256) { r = ledBrightness; g = h; b = 0; }
            else if (h < 512) { r = 511 - h; g = ledBrightness; b = 0; }
            else if (h < 768) { r = 0; g = ledBrightness; b = h - 512; }
            else if (h < 1024) { r = 0; g = 1023 - h; b = ledBrightness; }
            else if (h < 1280) { r = h - 1024; g = 0; b = ledBrightness; }
            else { r = ledBrightness; g = 0; b = 1535 - h; }

            setLedColor(r, g, b);
            lastAnim = now;
        }
        break;
    }
}
