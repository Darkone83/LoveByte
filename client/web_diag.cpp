#include "web_diag.h"
#include <SD_MMC.h>
#include <WiFi.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include "led.h"
#include <ArduinoJson.h>
#include "message.h" // <-- include your message handler
#include "config.h"  // <-- Needed for Config::get().deviceName

static String htmlHeader() {
    return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>LoveByte Diagnostics</title>
  <meta name="viewport" content="width=360, initial-scale=1">
  <style>
    body { background:#101016; color:#fff; font-family:sans-serif; text-align:center; margin:0; }
    .logo { width:128px; margin:32px auto 20px auto; display:block; border-radius:16px; box-shadow:0 0 24px #2228; }
    .section { background:#181825; margin:22px auto; max-width:340px; padding:20px 20px 12px 20px; border-radius:12px; box-shadow:0 2px 14px #0006; text-align:left;}
    h2 { text-align:center; margin-bottom:0.4em;}
    .btn { display:inline-block; margin:8px 2px; padding:10px 20px; background:#ff69b4; color:#fff; border:none; border-radius:8px; font-size:1.1em; font-weight:bold; letter-spacing:1px; cursor:pointer; transition:background .2s; box-shadow:0 2px 8px #0004;}
    .btn:disabled { opacity:0.5; background:#444;}
    .btn:hover { background:#f032a8; }
    .flist { font-size:0.95em; color:#cce; margin:0.5em 0;}
    label { font-weight:bold; }
    input[type=range] { width:90%; margin-top:8px; }
    textarea { width:95%; min-height:60px; border-radius:6px; font-size:1.1em; margin:6px 0 10px 0; border:1px solid #555; }
  </style>
</head>
<body>
  <img class="logo" src="/res/splash.jpg" alt="LoveByte Logo">
  <h2>Diagnostics</h2>
)rawliteral";
}

static String htmlFooter() {
    return R"rawliteral(
  <p style="margin-top:2em;color:#ccc;font-size:.9em;">&copy; Darkone83</p>
  <script>
    document.addEventListener('DOMContentLoaded', function() {
      var slider = document.getElementById('led_bright');
      var label = document.getElementById('led_bright_label');
      if(slider) {
        slider.oninput = function() {
          label.innerText = slider.value;
        }
        slider.onchange = function() {
          fetch('/api/led/brightness', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({b: slider.value})
          });
        }
      }
      document.getElementById('set_static').onclick = function() {
        var color = document.getElementById('color').value.replace('#','');
        fetch('/api/led/static', {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify({c: color})
        });
      }
      document.getElementById('trigger_hb').onclick = function() {
        var color = document.getElementById('heartbeat_color').value.replace('#','');
        var pulses = document.getElementById('heartbeat_count').value;
        fetch('/api/led/heartbeat', {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify({c: color, p: pulses})
        });
      }
      // Send Test Message
      document.getElementById('send_testmsg').onclick = function() {
        var msg = document.getElementById('testmsg').value.trim();
        if (!msg) { alert("Please enter a test message."); return; }
        fetch('/api/message/test', {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify({text: msg})
        }).then(r => r.text()).then(t => { alert(t); });
      }
    });
  </script>
</body>
</html>
)rawliteral";
}

void setupDiagPageRoutes(AsyncWebServer& server) {
    // --- Main diagnostics page ---
    server.on("/lb/diag", HTTP_GET, [](AsyncWebServerRequest* request){
        String chipModel = String(ESP.getChipModel());
        String chipRev = String(ESP.getChipRevision());
        String chipId = String((uint32_t)ESP.getEfuseMac(), HEX);
        size_t freeHeap = ESP.getFreeHeap();
        size_t freePsram = ESP.getPsramSize() ? ESP.getFreePsram() : 0;
        uint64_t uptime_ms = esp_timer_get_time() / 1000ULL;

        // SD card
        String sdHtml;
        if (SD_MMC.begin("/sd", true)) {
            uint64_t sdSize = SD_MMC.cardSize();
            uint64_t sdUsed = SD_MMC.usedBytes();
            uint64_t sdFree = sdSize > sdUsed ? (sdSize - sdUsed) : 0;
            sdHtml += "<b>Card Present:</b> Yes<br>";
            sdHtml += "Size: " + String(sdSize / (1024*1024)) + " MB<br>";
            sdHtml += "Used: " + String(sdUsed / (1024*1024)) + " MB<br>";
            sdHtml += "Free: " + String(sdFree / (1024*1024)) + " MB<br>";

            // File list
            File root = SD_MMC.open("/");
            sdHtml += "<div class='flist'><b>Files:</b><ul>";
            File f;
            while ((f = root.openNextFile())) {
                sdHtml += "<li>";
                sdHtml += f.name();
                if (!f.isDirectory()) {
                    sdHtml += " (" + String(f.size()) + " bytes)";
                }
                sdHtml += "</li>";
                f.close();
            }
            sdHtml += "</ul></div>";

            sdHtml += "<form action='/lb/diag/format' method='POST' onsubmit='return confirm(\"Really format SD? (Not implemented)\")'><button class='btn'>Format SD Card</button></form>";
            root.close();
        } else {
            sdHtml += "<b>Card Present:</b> No<br>";
        }

        // WiFi info
        String wifiHtml;
        if (WiFi.isConnected()) {
            wifiHtml += "<b>SSID:</b> " + WiFi.SSID() + "<br>";
            wifiHtml += "<b>IP:</b> " + WiFi.localIP().toString() + "<br>";
            wifiHtml += "<b>RSSI:</b> " + String(WiFi.RSSI()) + " dBm<br>";
            wifiHtml += "<b>MAC:</b> " + WiFi.macAddress() + "<br>";
        } else {
            wifiHtml += "Not connected<br>";
        }

        String html = htmlHeader();
        html += "<div class='section'><b>Chip:</b> ESP32 " + chipModel + "<br>";
        html += "<b>Revision:</b> " + chipRev + "<br>";
        html += "<b>Chip ID:</b> " + chipId + "<br>";
        html += "<b>Free RAM:</b> " + String(freeHeap/1024) + " KB<br>";
        html += "<b>Free PSRAM:</b> " + String(freePsram/1024) + " KB<br>";
        html += "<b>Uptime:</b> " + String(uptime_ms/1000) + " sec<br></div>";

        html += "<div class='section'>" + sdHtml + "</div>";
        html += "<div class='section'>" + wifiHtml + "</div>";

        // LED Brightness Slider (API usage)
        uint8_t currBright = Led::getBrightness();
        html += "<div class='section'><label>LED Brightness:</label><br>";
        html += "<input type='range' id='led_bright' min='1' max='255' value='" + String(currBright) + "'>";
        html += " <span id='led_bright_label'>" + String(currBright) + "</span></div>";

        // Static Color Test
        html += "<div class='section'><label>Test Static Color:</label><br>";
        html += "<input type='color' id='color' value='#ffffff'>";
        html += "<button class='btn' id='set_static'>Set Static Color</button></div>";

        // Heartbeat Test
        html += "<div class='section'><label>Test Heartbeat:</label><br>";
        html += "<input type='color' id='heartbeat_color' value='#ff0055'>";
        html += "<label>Pulses:</label><input type='number' id='heartbeat_count' min='1' max='10' value='2' style='width:48px'>";
        html += "<button class='btn' id='trigger_hb'>Trigger Heartbeat</button></div>";

        // -- Test Message Sender --
        html += "<div class='section'>";
        html += "<label>Send Test Message:</label><br>";
        html += "<textarea id='testmsg' placeholder='Type a test message'></textarea><br>";
        html += "<button class='btn' id='send_testmsg'>Send Test Message</button><br><br>";

        // --- IMAGE/GIF UPLOAD SECTION (NEW) ---
        html += "<label>Send Test Image/GIF:</label><br>";
        html += "<form id='imgupload' enctype='multipart/form-data'>";
        html += "<input type='file' name='file' accept='image/jpeg,image/png,image/gif' required><br><br>";
        html += "<input type='hidden' id='recipient' name='recipient' value=''>";
        html += "<button class='btn' type='submit'>Send Image/GIF</button>";
        html += "</form>";
        html += "<script>";
        html += "fetch('/api/device_id').then(r=>r.text()).then(id=>document.getElementById('recipient').value=id.trim());";
        html += "document.getElementById('imgupload').onsubmit=function(e){";
        html += "e.preventDefault();";
        html += "var fd=new FormData(this);";
        html += "fetch('http://'+location.hostname+':6969/api/upload_image',{method:'POST',body:fd})";
        html += ".then(r=>r.json()).then(j=>alert(j.status==='uploaded'?'Image sent!':'Failed: '+(j.error||j.result)));";
        html += "};";
        html += "</script>";
        html += "</div>";

        html += htmlFooter();
        request->send(200, "text/html", html);
    });

    // --- SD Format handler (Not implemented for SD_MMC, always fails) ---
    server.on("/lb/diag/format", HTTP_POST, [](AsyncWebServerRequest* request){
        request->send(200, "text/html", "<b>Format not implemented for SD_MMC!<br><a href='/lb/diag'>Back</a>");
    });

    // --- PATCH: API for LED brightness (POST, JSON) ---
    server.on("/api/led/brightness", HTTP_POST, [](AsyncWebServerRequest* request){}, NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t, size_t) {
            StaticJsonDocument<64> doc;
            deserializeJson(doc, data, len);
            int b = doc["b"];
            if (b >= 1 && b <= 255) {
                Led::setBrightness(b);
            }
            request->send(200, "text/plain", "OK");
        }
    );
    // --- PATCH: API for LED static color (POST, JSON) ---
    server.on("/api/led/static", HTTP_POST, [](AsyncWebServerRequest* request){}, NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t, size_t) {
            StaticJsonDocument<64> doc;
            deserializeJson(doc, data, len);
            const char* c = doc["c"];
            uint32_t rgb = (uint32_t)strtol(c, nullptr, 16);
            Led::setColor(rgb);
            Led::setMode(LedMode::StaticColor);
            request->send(200, "text/plain", "OK");
        }
    );
    // --- PATCH: API for LED heartbeat (POST, JSON) ---
    server.on("/api/led/heartbeat", HTTP_POST, [](AsyncWebServerRequest* request){}, NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t, size_t) {
            StaticJsonDocument<64> doc;
            deserializeJson(doc, data, len);
            const char* c = doc["c"];
            int p = doc["p"];
            uint32_t rgb = (uint32_t)strtol(c, nullptr, 16);
            Led::heartbeat(rgb, p);
            request->send(200, "text/plain", "OK");
        }
    );

    // --- PATCH: API for sending test message ---
    server.on("/api/message/test", HTTP_POST, [](AsyncWebServerRequest* request){}, NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t, size_t) {
            StaticJsonDocument<256> doc;
            DeserializationError err = deserializeJson(doc, data, len);
            if (err) {
                request->send(400, "text/plain", "Bad JSON");
                return;
            }
            String text = doc["text"] | "";
            if (text.length() < 1) {
                request->send(400, "text/plain", "Empty message");
                return;
            }

            // Compose "fake" message metadata
            String now;
            time_t t = time(NULL);
            struct tm *tm_info = localtime(&t);
            char buf[32];
            strftime(buf, sizeof(buf), "%Y-%m-%d %H-%M-%S", tm_info);
            now = String(buf);

            // Optional: pull device info for sender/weather if desired
            String sender = "Self";
            String weather = "Test";

            bool ok = MessageHandler::receive(text, sender, now);

            if (ok) {
                request->send(200, "Test message received and saved.");
            } else {
                request->send(500, "Failed to save message.");
            }
        }
    );

    // --- API for device ID for upload form ---
    server.on("/api/device_id", HTTP_GET, [](AsyncWebServerRequest* request){
        String id = Config::get().deviceName;
        request->send(200, "text/plain", id);
    });
}
