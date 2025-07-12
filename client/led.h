// led.h
#pragma once

#include <stdint.h>

enum class LedMode {
    BootBlink,
    BreathePink,
    StaticColor,
    RainbowFade,
    Heartbeat // not a mode, but present for API completeness
};

namespace Led {
    void begin();
    void setMode(LedMode m);
    LedMode getMode();

    void setColor(uint32_t rgb);
    uint32_t getColor();

    void setBrightness(uint8_t b);
    uint8_t getBrightness();

    void heartbeat(uint32_t rgb, uint8_t pulses);
    void loop();
}
