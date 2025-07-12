#include "message.h"
#include "config.h"
#include "led.h"
#include <SD_MMC.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <vector>
#include <algorithm>
#include <time.h>

// Forward declarations for display helpers (must exist elsewhere in project)
extern void displayShowNotification(const String& txt);
extern void displayShowMessage(const String& txt);

#define MESSAGE_DIR "/messages"

// -------- PATCH CONTROL: Comment this to DISABLE weather fetch (sets dummy data) --------
#define ENABLE_WEATHER_FETCH

static void ensureDir() {
    if (!SD_MMC.exists(MESSAGE_DIR)) {
        SD_MMC.mkdir(MESSAGE_DIR);
    }
}

// Converts "YYYY-MM-DD HH:MM:SS" to "MM/DD/YY HH:MM AM/PM"
static String prettyTime(const String& in) {
    int year, month, day, hour, min, sec;
    if (sscanf(in.c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &min, &sec) == 6) {
        char buf[24];
        int year2 = year % 100;
        int hour12 = hour % 12;
        if (hour12 == 0) hour12 = 12;
        const char* ampm = (hour < 12) ? "AM" : "PM";
        snprintf(buf, sizeof(buf), "%02d/%02d/%02d %02d:%02d %s", month, day, year2, hour12, min, ampm);
        return String(buf);
    }
    return in;
}

// Internal helper to fetch weather data (OpenWeatherMap)
static void fetchWeather(String& weather, String& city, String& country, int& tempF) {
    weather = ""; city = ""; country = ""; tempF = 0;
    DeviceConfig& cfg = Config::get();
    if (cfg.weatherApiKey.isEmpty() || cfg.weatherPostal.isEmpty() || cfg.weatherCountry.isEmpty()) {
        return;
    }
    String url = "http://api.openweathermap.org/data/2.5/weather?zip=" + cfg.weatherPostal + "," + cfg.weatherCountry + "&appid=" + cfg.weatherApiKey + "&units=imperial";
    HTTPClient http;
    http.setTimeout(2000);
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode == 200) {
        String payload = http.getString();
        StaticJsonDocument<1536> doc;
        auto err = deserializeJson(doc, payload);
        if (err == DeserializationError::Ok) {
            if (doc["weather"] && doc["weather"][0]["main"])
                weather = doc["weather"][0]["main"].as<String>();
            if (doc["name"])
                city = doc["name"].as<String>();
            if (doc["sys"] && doc["sys"]["country"])
                country = doc["sys"]["country"].as<String>();
            if (doc["main"] && doc["main"]["temp"])
                tempF = int(doc["main"]["temp"].as<float>());
        }
    }
    http.end();
}

// Helper: Apply LED effects according to message fields (heartbeat takes priority)
static void applyMessageLedEffect(const Message& msg) {
    Serial.printf("[Message] LED: ledColor=0x%06lx useLed=%d | heartbeat=0x%06lx useHB=%d pulses=%d\n",
        (unsigned long)msg.ledColor, msg.useLedColor, (unsigned long)msg.heartbeatColor, msg.useHeartbeat, msg.heartbeatPulses);

    // Heartbeat takes priority over static color
    if (msg.useHeartbeat && msg.heartbeatColor != 0 && msg.heartbeatPulses > 0) {
        Serial.println("[Message] Triggering HEARTBEAT effect");
        Led::heartbeat(msg.heartbeatColor, msg.heartbeatPulses);
    } else if (msg.useLedColor && msg.ledColor != 0) {
        Serial.println("[Message] Triggering STATIC COLOR effect");
        Led::setColor(msg.ledColor);
        Led::setMode(LedMode::StaticColor);
    } else {
        Serial.println("[Message] No LED effect triggered");
    }
}

// Save a message with a unique filename and show notification & message
bool MessageHandler::receive(const String& text, const String& sender, const String& timeReceived,
                             uint32_t ledColor, bool useLedColor, bool useHeartbeat, uint32_t heartbeatColor, uint8_t heartbeatPulses) {
    ensureDir();

    String weather, city, country;
    int tempF = 0;
#ifdef ENABLE_WEATHER_FETCH
    fetchWeather(weather, city, country, tempF);
#else
    weather = "TEST"; city = "NoNet"; country = "XX"; tempF = 42;
#endif

    String base = String(MESSAGE_DIR) + "/" + timeReceived;
    base.replace(' ', '_');
    base.replace(':', '-');
    String filename = base + ".txt";
    int counter = 1;
    while (SD_MMC.exists(filename)) {
        filename = base + "_" + String(counter++) + ".txt";
    }

    File file = SD_MMC.open(filename, FILE_WRITE);
    if (!file) return false;

    StaticJsonDocument<512> doc;
    doc["text"] = text;
    doc["sender"] = sender;
    doc["time"] = timeReceived;
    doc["weather"] = weather;
    doc["city"] = city;
    doc["country"] = country;
    doc["tempF"] = tempF;

    // Always store colors as hex strings for consistency
    char ledColorStr[8], hbColorStr[8];
    snprintf(ledColorStr, sizeof(ledColorStr), "%06X", ledColor);
    snprintf(hbColorStr, sizeof(hbColorStr), "%06X", heartbeatColor);
    doc["ledColor"] = String(ledColorStr);
    doc["useLedColor"] = useLedColor;
    doc["useHeartbeat"] = useHeartbeat;
    doc["heartbeatColor"] = String(hbColorStr);
    doc["heartbeatPulses"] = heartbeatPulses;

    if (serializeJson(doc, file) == 0) {
        file.close();
        return false;
    }
    file.close();

    displayShowNotification("Incoming LoveByte!");
    delay(500);

    // --- Always trigger LED effect right here (like web_diag does) ---
    Message msg;
    msg.ledColor = ledColor;
    msg.useLedColor = useLedColor;
    msg.useHeartbeat = useHeartbeat;
    msg.heartbeatColor = heartbeatColor;
    msg.heartbeatPulses = heartbeatPulses;
    applyMessageLedEffect(msg);

    // Now load and show the just-written message
    if (MessageHandler::load(filename, msg)) {
        String disp = MessageHandler::formatForDisplay(msg);
        displayShowMessage(disp);
        // No need to re-apply LED effect, already done above.
    }
    return true;
}

// Overload for backward compatibility: (no led fields)
bool MessageHandler::receive(const String& text, const String& sender, const String& timeReceived) {
    return MessageHandler::receive(text, sender, timeReceived, 0, false, false, 0, 0);
}

// Load by filename
bool MessageHandler::load(const String& filename, Message& out) {
    String fullpath = filename.startsWith("/") ? filename : String(MESSAGE_DIR) + "/" + filename;
    File file = SD_MMC.open(fullpath, FILE_READ);
    if (!file) return false;

    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, file);
    file.close();
    if (err) return false;

    out.text = doc["text"] | "";
    out.sender = doc["sender"] | "";
    out.timeReceived = doc["time"] | "";
    out.weather = doc["weather"] | "";
    out.city = doc["city"] | "";
    out.country = doc["country"] | "";
    out.tempF = doc["tempF"] | 0;
    out.filename = fullpath;

    // Always process as string if possible, like web_diag.cpp does:
    out.ledColor = 0;
    if (doc["ledColor"].is<const char*>()) {
        out.ledColor = (uint32_t)strtol(doc["ledColor"].as<const char*>(), nullptr, 16);
    } else if (doc["ledColor"].is<int>() || doc["ledColor"].is<uint32_t>()) {
        out.ledColor = doc["ledColor"];
    }

    out.useLedColor = doc["useLedColor"] | false;
    out.useHeartbeat = doc["useHeartbeat"] | false;

    // ---- KEY PATCH: always parse as string if present, just like web_diag does ----
    out.heartbeatColor = 0;
    if (doc["heartbeatColor"].is<const char*>()) {
        out.heartbeatColor = (uint32_t)strtol(doc["heartbeatColor"].as<const char*>(), nullptr, 16);
    } else if (doc["heartbeatColor"].is<int>() || doc["heartbeatColor"].is<uint32_t>()) {
        out.heartbeatColor = doc["heartbeatColor"];
    }

    out.heartbeatPulses = doc["heartbeatPulses"] | 0;

    return true;
}



bool MessageHandler::latest(Message& out) {
    auto files = MessageHandler::getAllFilenames();
    if (files.empty()) return false;
    return load(files.back(), out);
}

size_t MessageHandler::count() {
    auto count = MessageHandler::getAllFilenames().size();
    return count;
}

String MessageHandler::formatForDisplay(const Message& msg) {
    String display;
    display += msg.sender + "\n";
    display += msg.text + "\n\n";
    String weatherLine;
    if (msg.weather.length() > 0 && msg.tempF != 0) {
        weatherLine = msg.weather + " " + String(msg.tempF) + "F";
    } else if (msg.weather.length() > 0) {
        weatherLine = msg.weather;
    } else if (msg.tempF != 0) {
        weatherLine = String(msg.tempF) + "F";
    }
    display += weatherLine + "\n";
    String cityLine;
    if (msg.city.length() > 0 && msg.country.length() > 0) {
        cityLine = msg.city + ", " + msg.country;
    } else if (msg.city.length() > 0) {
        cityLine = msg.city;
    } else if (msg.country.length() > 0) {
        cityLine = msg.country;
    }
    display += cityLine + "\n\n";
    display += prettyTime(msg.timeReceived) + "\n";
    return display;
}

bool MessageHandler::remove(size_t idx) {
    auto files = MessageHandler::getAllFilenames();
    if (idx >= files.size()) return false;
    return SD_MMC.remove(files[idx]);
}

bool MessageHandler::remove(const String& filename) {
    String fullpath = filename.startsWith("/") ? filename : String(MESSAGE_DIR) + "/" + filename;
    return SD_MMC.remove(fullpath);
}

std::vector<String> MessageHandler::getAllFilenames() {
    ensureDir();
    std::vector<String> files;
    File dir = SD_MMC.open(MESSAGE_DIR);
    if (!dir) return files;
    File entry;
    while ((entry = dir.openNextFile())) {
        if (!entry.isDirectory()) files.push_back(String(entry.name()));
        entry.close();
    }
    dir.close();
    std::sort(files.begin(), files.end());
    return files;
}

void MessageHandler::listFilenames(String* arr, size_t max, size_t& actual) {
    auto files = getAllFilenames();
    actual = files.size();
    for (size_t i = 0; i < files.size() && i < max; ++i)
        arr[i] = files[i];
}

void MessageHandler::clearAll() {
    auto files = getAllFilenames();
    for (auto& f : files) SD_MMC.remove(f);
}

void MessageHandler::showIncomingNotification() {
    displayShowNotification("Incoming LoveByte!");
}

bool MessageHandler::showMessageOnDisplay(size_t idx) {
    Message msg;
    if (!load(idx, msg)) return false;
    String disp = formatForDisplay(msg);
    displayShowMessage(disp);
    applyMessageLedEffect(msg);
    return true;
}

bool MessageHandler::showMessageOnDisplay(const String& filename) {
    Message msg;
    if (!load(filename, msg)) return false;
    String disp = formatForDisplay(msg);
    displayShowMessage(disp);
    applyMessageLedEffect(msg);
    return true;
}
