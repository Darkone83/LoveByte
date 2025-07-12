#include "web_config.h"
#include "config.h"
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <time.h>

// Forward declaration for NTP sync
void ntpSync(int timezone_hours);

// HTML page for config
static String htmlConfigPage() {
    auto& cfg = Config::get();
    String html =
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "  <title>LoveByte Config</title>\n"
        "  <meta name=\"viewport\" content=\"width=360, initial-scale=1\">\n"
        "  <style>\n"
        "    body { background:#101016; color:#fff; font-family:sans-serif; text-align:center; margin:0; }\n"
        "    .section { background:#181825; margin:22px auto; max-width:340px; padding:20px 20px 12px 20px; border-radius:12px; box-shadow:0 2px 14px #0006; text-align:left;}\n"
        "    input[type=text],input[type=number] { width: 98%; margin: 5px 0; border-radius: 6px; border: 1px solid #999; font-size: 1em; padding: 7px;}\n"
        "    .btn { margin-top:12px; width:100%; background:#ff69b4; color:#fff; border:none; border-radius:10px; font-size:1.2em; font-weight:bold; cursor:pointer;}\n"
        "    .btn:hover { background:#f032a8; }\n"
        "    label { font-weight:bold; margin-top:10px; display:block;}\n"
        "  </style>\n"
        "</head>\n"
        "<body>\n"
        "  <h2>Device Configuration</h2>\n"
        "  <form id=\"cfgform\" class=\"section\" onsubmit=\"saveCfg();return false;\">\n"
        "    <label>Device Name:</label>\n"
        "    <input type=\"text\" name=\"devname\" id=\"devname\" value=\"" + cfg.deviceName + "\">\n"
        "    <label>Timezone Offset (hours, e.g. -7 for PDT):</label>\n"
        "    <input type=\"number\" name=\"tz\" id=\"tz\" step=\"1\" min=\"-12\" max=\"14\" value=\"" + String(cfg.timezone/3600) + "\">\n"
        "    <label>Postal/ZIP Code:</label>\n"
        "    <input type=\"text\" name=\"postal\" id=\"postal\" value=\"" + cfg.weatherPostal + "\">\n"
        "    <label>Country Code (2-letter):</label>\n"
        "    <input type=\"text\" name=\"country\" id=\"country\" maxlength=\"2\" value=\"" + cfg.weatherCountry + "\">\n"
        "    <label>Weather API Key (OpenWeatherMap):</label>\n"
        "    <input type=\"text\" name=\"wkey\" id=\"wkey\" value=\"" + cfg.weatherApiKey + "\">\n"
        "    <label>Server Address:</label>\n"
        "    <input type=\"text\" name=\"server\" id=\"server\" value=\"" + cfg.serverAddress + "\">\n"
        "    <div>Last NTP Sync: <span id=\"ntptime\">" + cfg.lastNtpTime + "</span></div>\n"
        "    <button type=\"submit\" class=\"btn\">Save</button>\n"
        "    <button type=\"button\" class=\"btn\" style=\"background:#76e;font-size:1em;margin-top:8px;\" onclick=\"syncTime()\">Sync Time Now</button>\n"
        "  </form>\n"
        "  <script>\n"
        "    function saveCfg() {\n"
        "      let data = {\n"
        "        devname: document.getElementById('devname').value,\n"
        "        tz: document.getElementById('tz').value,\n"
        "        postal: document.getElementById('postal').value,\n"
        "        country: document.getElementById('country').value,\n"
        "        wkey: document.getElementById('wkey').value,\n"
        "        server: document.getElementById('server').value\n"
        "      };\n"
        "      fetch('/api/config/save', {\n"
        "        method: 'POST',\n"
        "        headers: {'Content-Type': 'application/json'},\n"
        "        body: JSON.stringify(data)\n"
        "      }).then(r => r.text()).then(msg => { alert(msg); location.reload(); });\n"
        "    }\n"
        "    function syncTime() {\n"
        "      fetch('/api/config/ntpsync', { method: 'POST' })\n"
        "        .then(r => r.text())\n"
        "        .then(msg => { alert(msg); location.reload(); });\n"
        "    }\n"
        "  </script>\n"
        "  <p style=\"margin-top:2em;color:#ccc;font-size:.9em;\">&copy; Darkone83</p>\n"
        "</body>\n"
        "</html>\n";
    return html;
}

void setupConfigPageRoutes(AsyncWebServer& server) {
    // Serve config page
    server.on("/lb/config", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/html", htmlConfigPage());
    });

    // Config save endpoint (POST, JSON)
    server.on("/api/config/save", HTTP_POST, [](AsyncWebServerRequest* request){}, NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t, size_t) {
            StaticJsonDocument<256> doc;
            DeserializationError err = deserializeJson(doc, data, len);
            if (err) {
                request->send(400, "text/plain", "Bad JSON");
                return;
            }
            auto& cfg = Config::get();
            bool doNtp = false;
            if (doc.containsKey("devname")) cfg.deviceName = doc["devname"].as<String>();
            if (doc.containsKey("tz")) {
                int newTz = doc["tz"].as<int>() * 3600;
                if (newTz != cfg.timezone) doNtp = true;
                cfg.timezone = newTz;
            }
            if (doc.containsKey("postal"))  cfg.weatherPostal = doc["postal"].as<String>();
            if (doc.containsKey("country")) cfg.weatherCountry = doc["country"].as<String>();
            if (doc.containsKey("wkey"))    cfg.weatherApiKey = doc["wkey"].as<String>();
            if (doc.containsKey("server"))  cfg.serverAddress = doc["server"].as<String>();
            Config::save();
            if (doNtp) {
                ntpSync(cfg.timezone / 3600);  // NTP sync after timezone change
            }
            request->send(200, "text/plain", "Saved");
        }
    );

    // Manual NTP sync endpoint
    server.on("/api/config/ntpsync", HTTP_POST, [](AsyncWebServerRequest* request) {
        auto& cfg = Config::get();
        ntpSync(cfg.timezone / 3600);
        request->send(200, "text/plain", "NTP Sync Complete");
    });
}

// ---- NTP sync function ----
void ntpSync(int timezone_hours) {
    const char* ntpServer = "pool.ntp.org";
    int gmtOffset_sec = timezone_hours * 3600;
    int daylightOffset_sec = 0;
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    struct tm timeinfo;
    for (int i = 0; i < 10; ++i) {
        if (getLocalTime(&timeinfo)) break;
        delay(500);
    }
    if (getLocalTime(&timeinfo)) {
        char buf[32];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
        auto& cfg = Config::get();
        cfg.lastNtpTime = String(buf);
        Config::save();
    }
}
