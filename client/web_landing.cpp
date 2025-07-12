#include "web_landing.h"
#include <Arduino.h>
#include <SD_MMC.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Serve /lb (landing page)
void setupLandingPageRoutes(AsyncWebServer& server) {
    // Landing Page
    server.on("/lb", HTTP_GET, [](AsyncWebServerRequest* request){
        String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>LoveByte Hub</title>
  <meta name="viewport" content="width=360, initial-scale=1">
  <style>
    body { background:#101016; color:#fff; font-family:sans-serif; text-align:center; margin:0; }
    .logo { width:128px; margin:32px auto 20px auto; display:block; border-radius:16px; box-shadow:0 0 24px #2228; }
    .btn { display:block; width:220px; margin:18px auto; padding:18px 0; background:#ff69b4; color:#fff; border:none; border-radius:10px; font-size:1.3em; font-weight:bold; letter-spacing:2px; cursor:pointer; transition:background .2s; box-shadow:0 2px 12px #0004;}
    .btn:hover { background:#f032a8; }
  </style>
</head>
<body>
  <img class="logo" src="/res/splash.jpg" alt="LoveByte Logo">
  <h2>Welcome to LoveByte!</h2>
  <button class="btn" onclick="location.href='/lb/cloud'">LoveByte Messenger</button>
  <button class="btn" onclick="location.href='/lb/fileman'">File Manager</button>
  <button class="btn" onclick="location.href='/lb/config'">Configure Device</button>
  <button class="btn" onclick="location.href='/lb/diag'">Diagnostics</button>
  <p style="margin-top:2em;color:#ccc;font-size:.9em;">&copy; Darkone83</p>
</body>
</html>
        )rawliteral";
        request->send(200, "text/html", html);
    });

    // Serve splash image from SD card at /res/splash.jpg
    server.on("/res/splash.jpg", HTTP_GET, [](AsyncWebServerRequest* request){
        File file = SD_MMC.open("/res/splash.jpg");
        if (!file || file.isDirectory()) {
            request->send(404, "text/plain", "Splash image not found");
            return;
        }
        request->send(file, String("/res/splash.jpg"), String("image/jpeg"), false);
        // Do NOT close the file; send() will handle it.
    });
}
