#pragma once
#include <LovyanGFX.hpp>
#include <Adafruit_NeoPixel.h>

// ====== Board Pinouts ======
#define PIN_TFT_SCLK    40
#define PIN_TFT_MOSI    45
#define PIN_TFT_MISO    -1
#define PIN_TFT_DC      41
#define PIN_TFT_CS      42
#define PIN_TFT_RST     39
#define PIN_TFT_BL      46

#define PIN_WS2812      38
#define PIN_BAT_ADC     4

#define PIN_SD_CLK      14
#define PIN_SD_CMD      15
#define PIN_SD_D0       16
#define PIN_SD_D1       18
#define PIN_SD_D2       17
#define PIN_SD_D3       21

// ====== LovyanGFX Config & Instantiation ======
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel;
  lgfx::Bus_SPI _bus;
  lgfx::Light_PWM _light;
public:
  LGFX(void) {
    { // SPI bus config
      auto cfg = _bus.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read = 16000000;
      cfg.spi_3wire = true;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = PIN_TFT_SCLK;
      cfg.pin_mosi = PIN_TFT_MOSI;
      cfg.pin_miso = PIN_TFT_MISO;
      cfg.pin_dc   = PIN_TFT_DC;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    { // Panel config
      auto cfg = _panel.config();
      cfg.pin_cs   = PIN_TFT_CS;
      cfg.pin_rst  = PIN_TFT_RST;
      cfg.invert   = true;
      cfg.panel_width  = 172;
      cfg.panel_height = 320;
      cfg.memory_width  = 172;
      cfg.memory_height = 320;
      cfg.offset_x = -34;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      _panel.config(cfg);
    }
    { // Backlight (PWM)
      auto cfg = _light.config();
      cfg.pin_bl = PIN_TFT_BL;
      cfg.invert = false;
      _light.config(cfg);
      _panel.setLight(&_light);
    }
    setPanel(&_panel);
  }
};
// --- Global instance ---
inline LGFX display;

// ====== WS2812 Config & Instantiation ======
#define NUM_PIXELS   1
inline Adafruit_NeoPixel rgb(NUM_PIXELS, PIN_WS2812, NEO_GRB + NEO_KHZ800);

// ====== Display Appearance ======
#define DISPLAY_FONT           &fonts::Font2
#define DISPLAY_TEXT_COLOR     TFT_WHITE
#define DISPLAY_BG_COLOR       TFT_BLACK

// ====== Battery ======
#define VBAT_SCALE   5.7f   // Divider ratio
#define VBAT_VREF    3.3f   // Reference voltage
