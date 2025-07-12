#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <SD_MMC.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "settings.h"
#include "wifimgr.h"
#include <esp_heap_caps.h>
#include <esp_system.h>
#include "led.h"
#include "web_landing.h"
#include "web_diag.h"
#include "config.h"
#include "web_config.h"
#include "message.h"
#include "image.h"     // <-- add this for image support
#include "time.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "web_fileman.h"
#include "web_message.h"

#define PINK  display.color565(255, 105, 180)
#define WHITE 0xFFFF
#define BLACK 0x0000

extern AsyncWebServer server;

// --- GLOBAL display lock ---
volatile bool g_displayLock = false;
static unsigned long messageShownAt = 0;
static unsigned long messageLockDuration = 3500; // 3.5 seconds

// --- DEVICE CHECK-IN PATCH ---
void checkInWithServer() {
  DeviceConfig& cfg = Config::get();
  if (cfg.serverAddress.isEmpty() || cfg.deviceName.isEmpty()) {
    return;
  }
  String url = "http://" + cfg.serverAddress + ":6969/api/checkin";
  String payload = "{\"device_id\":\"" + cfg.deviceName + "\"}";
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.POST(payload);
  http.end();
}

// --- SERVER MESSAGE PULL PATCH ---
void pullMessagesFromServer() {
  DeviceConfig& cfg = Config::get();
  if (cfg.serverAddress.isEmpty() || cfg.deviceName.isEmpty()) {
    return;
  }

  String url = "http://" + cfg.serverAddress + ":6969/api/pull";
  String payload = "{\"device_id\":\"" + cfg.deviceName + "\"}";

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(payload);

  if (httpCode == 200) {
    String response = http.getString();
    StaticJsonDocument<2048> doc;
    DeserializationError err = deserializeJson(doc, response);
    if (!err && doc["messages"].is<JsonArray>()) {
      for (JsonObject msg : doc["messages"].as<JsonArray>()) {
        String text      = msg["text"]    | "";
        String sender    = msg["sender"]  | "";
        String timeRecv  = msg["time"]    | getCurrentTimeString();

        // --- PATCH: LED/Heartbeat support (hex or decimal) ---
        bool useLedColor     = msg["useLedColor"]    | false;
        uint32_t ledColor    = 0;
        if (msg["ledColor"].is<const char*>()) {
          String c = msg["ledColor"].as<const char*>();
          ledColor = strtoul(c.c_str(), nullptr, 16);
        } else if (msg["ledColor"].is<uint32_t>()) {
          ledColor = msg["ledColor"] | 0;
        }

        bool useHeartbeat    = msg["useHeartbeat"]   | false;
        uint32_t heartbeatColor = 0;
        if (msg["heartbeatColor"].is<const char*>()) {
          String h = msg["heartbeatColor"].as<const char*>();
          heartbeatColor = strtoul(h.c_str(), nullptr, 16);
        } else if (msg["heartbeatColor"].is<uint32_t>()) {
          heartbeatColor = msg["heartbeatColor"] | 0;
        }
        uint8_t heartbeatPulses = msg["heartbeatPulses"] | 0;

        // --- IMAGE MESSAGE HANDLING ---
        if (text.startsWith("[IMAGE]")) {
          String imgFile = text.substring(7);
          imgFile.trim();
          displayShowNotification("Incoming LoveByte!");
          delay(650);
          ImageHandler::receive(imgFile, cfg.serverAddress);
          g_displayLock = true;
          messageShownAt = millis();
          continue;
        }

        // --- NORMAL MESSAGE HANDLING ---
        displayShowNotification("Incoming LoveByte!");
        delay(650);

        if (useLedColor || useHeartbeat) {
          MessageHandler::receive(
            text, sender, timeRecv,
            ledColor, useLedColor, useHeartbeat, heartbeatColor, heartbeatPulses
          );
        } else {
          MessageHandler::receive(text, sender, timeRecv);
        }
        g_displayLock = true;
        messageShownAt = millis();
      }
    }
  }
  http.end();
}

// --- LOGGED NTP SYNC (STATUS) ---
bool syncNtpAndWaitLogged() {
  int tz_sec = Config::get().timezone * 3600;
  configTime(Config::get().timezone, 0, "pool.ntp.org");
  struct tm timeinfo;
  int retries = 0;
  bool synced = false;
  while (retries < 20) {
    if (getLocalTime(&timeinfo) && timeinfo.tm_year >= 70) {
      synced = true;
      break;
    }
    delay(500);
    retries++;
  }
  if (synced) {
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    Config::get().lastNtpTime = String(buf);
    Config::save();
    return true;
  } else {
    return false;
  }
}

// --- TIMESTAMP HELPER ---
String getCurrentTimeString() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo) && timeinfo.tm_year >= 70) {
    char buf[24] = {0};
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(buf);
  }
  return "1970-01-01 00:00:00";
}

// Draw centered text
void drawCenteredText(const char* text, int y, uint16_t color, uint8_t size = 1) {
  display.setFont(DISPLAY_FONT);
  display.setTextColor(color, BLACK);
  display.setTextSize(size);
  int textW = display.textWidth(text);
  int xpos = (display.width() - textW) / 2;
  display.setCursor(xpos, y);
  display.print(text);
  display.setTextSize(1);
}

void showSplash() {
  String splashPath = "/res/splash.jpg";
  if (SD_MMC.exists(splashPath)) {
    File f = SD_MMC.open(splashPath.c_str());
    if (f) {
      size_t jpgLen = f.size();
      uint8_t *jpgBuf = (uint8_t*)ps_malloc(jpgLen);
      if (!jpgBuf) jpgBuf = (uint8_t*)malloc(jpgLen);
      if (jpgBuf && f.read(jpgBuf, jpgLen) == jpgLen) {
        display.drawJpg(jpgBuf, jpgLen, 0, 0, display.width(), display.height());
      }
      free(jpgBuf);
      f.close();
    } else {
      display.fillScreen(BLACK);
      int y = (display.height() - 28) / 2;
      drawCenteredText("LoveByte", y, PINK);
    }
  } else {
    display.fillScreen(BLACK);
    int y = (display.height() - 28) / 2;
    drawCenteredText("LoveByte", y, PINK);
  }
  delay(3000);
}

void showApInfo() {
  display.fillScreen(BLACK);
  String s1 = "AP: " + WiFi.softAPSSID();
  String s2 = "IP: " + WiFi.softAPIP().toString();
  int lineHeight = 28;
  int totalHeight = 3 * lineHeight;
  int startY = (display.height() - totalHeight) / 2;
  drawCenteredText(s1.c_str(), startY, WHITE);
  drawCenteredText(s2.c_str(), startY + lineHeight, WHITE);
  drawCenteredText("Please connect to WiFi to continue", startY + 2 * lineHeight, PINK);
}

void showWelcomeScreen() {
  display.fillScreen(BLACK);
  int size = 2;
  int y = (display.height() - 8 * size) / 2;
  drawCenteredText("Welcome to LoveByte", y, PINK, size);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  Led::begin();
  Led::setMode(LedMode::BootBlink);

  display.init();
  display.setRotation(1);
  display.fillScreen(BLACK);

  analogReadResolution(12);

  SD_MMC.setPins(PIN_SD_CLK, PIN_SD_CMD, PIN_SD_D0, PIN_SD_D1, PIN_SD_D2, PIN_SD_D3);
  bool sd_ok = SD_MMC.begin("/sd", false);

  WiFiMgr::begin();
  Config::begin();
  setupMessagePageRoutes(server);
  setupFileManagerRoutes(server);
  setupConfigPageRoutes(server);
  setupDiagPageRoutes(server);
  setupLandingPageRoutes(server);

  if (sd_ok) showSplash();
  else {
    display.fillScreen(BLACK);
    int y = (display.height() - 28) / 2;
    drawCenteredText("SD Not Found!", y, PINK);
    delay(2000);
  }

  // Wait up to 7s for WiFi connection
  unsigned long wifiStart = millis();
  while (!WiFiMgr::isConnected() && millis() - wifiStart < 7000) {
    WiFiMgr::loop();
    Led::loop();
    delay(8);
  }

  if (WiFiMgr::isConnected()) {
    Led::setMode(LedMode::RainbowFade);
    syncNtpAndWaitLogged();
    checkInWithServer(); // Device check-in to server
    showWelcomeScreen();

    if (MDNS.begin("lovebyte")) {
      MDNS.addService("http", "tcp", 80);
      MDNS.addServiceTxt("http", "tcp", "path", "/lb");
    }
  } else {
    Led::setMode(LedMode::BreathePink);
    showApInfo();
  }
}

void loop() {
  static bool lastConnected = false;
  static bool bootDone = false;
  static unsigned long lastPull = 0;

  WiFiMgr::loop();

  // --- PATCH: Unlock message lock after display duration ---
  if (g_displayLock) {
    Led::loop();
    if (millis() - messageShownAt > messageLockDuration) {
      clearDisplayedMessage();
    }
    return;
  }

  bool nowConnected = WiFiMgr::isConnected();
  if (bootDone && nowConnected != lastConnected) {
    if (nowConnected) {
      Led::setMode(LedMode::RainbowFade);
      showWelcomeScreen();
    } else {
      Led::setMode(LedMode::BreathePink);
      showApInfo();
    }
    lastConnected = nowConnected;
  }

  if (!bootDone) {
    if (nowConnected) {
      Led::setMode(LedMode::RainbowFade);
      showWelcomeScreen();
      bootDone = true;
      lastConnected = true;
    } else if (!nowConnected) {
      Led::setMode(LedMode::BreathePink);
      showApInfo();
      bootDone = true;
      lastConnected = false;
    }
  }

  // --- PATCH: Pull messages every 10 seconds ---
  if (nowConnected && millis() - lastPull > 10000) {
    pullMessagesFromServer();
    lastPull = millis();
  }

  Led::loop();
  ImageHandler_updateGif();
}

// --------------------------------------------------------------------------
// --- Notification and Message Display Implementations for MessageHandler ---
// --------------------------------------------------------------------------

void displayShowNotification(const String& txt) {
  // --- Stop any GIF playback before displaying notification ---
  ImageHandler::stopGifPlayback();

  display.fillScreen(BLACK);
  drawCenteredText(txt.c_str(), (display.height() - 24) / 2, PINK, 2);
}

void displayShowMessage(const String& txt) {
  // --- Stop any GIF playback before displaying message ---
  ImageHandler::stopGifPlayback();

  display.fillScreen(BLACK);
  display.setFont(DISPLAY_FONT);
  display.setTextColor(WHITE, BLACK);
  display.setTextSize(1);

  String lines[8]; int n = 0, last = 0, next = 0;
  while ((next = txt.indexOf('\n', last)) != -1 && n < 8) {
    lines[n++] = txt.substring(last, next);
    last = next + 1;
  }
  if (last < txt.length() && n < 8) lines[n++] = txt.substring(last);

  String sender   = n > 0 ? lines[0] : "";
  String message  = n > 1 ? lines[1] : "";
  String weather  = n > 3 ? lines[3] : "";
  String city     = n > 4 ? lines[4] : "";
  String datetime = n > 6 ? lines[6] : "";

  int margin = 8;

  int y = margin;
  if (sender.length()) {
    display.setCursor(margin, y);
    display.print(sender);
    y += 16;
  }
  if (message.length()) {
    display.setCursor(margin, y);
    display.print(message);
    y += 20;
  }
  if (weather.length()) {
    int wy = display.height() - 48;
    int wx = display.width() - display.textWidth(weather.c_str()) - margin;
    display.setCursor(wx, wy);
    display.print(weather);
  }
  if (city.length()) {
    int cy = display.height() - 34;
    int cx = display.width() - display.textWidth(city.c_str()) - margin;
    display.setCursor(cx, cy);
    display.print(city);
  }
  if (datetime.length()) {
    int fy = display.height() - 18;
    display.setCursor(margin, fy);
    display.print(datetime);
  }

  g_displayLock = true;
  messageShownAt = millis(); // Start lock timer
}

// Always clear lock after display duration
void clearDisplayedMessage() {
  g_displayLock = false;
}

// When you want to generate a message (direct user message, not from server pull)
void onReceiveSomeMessage(const String& text, const String& sender) {
  displayShowNotification("Incoming LoveByte!");
  delay(650);
  // Use the overload that does not set LED—direct user messages
  MessageHandler::receive(text, sender, getCurrentTimeString());
}
