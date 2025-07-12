#include "web_fileman.h"
#include <Arduino.h>
#include <SD_MMC.h>
#include <ESPAsyncWebServer.h>

// Helper: file size as string
String humanSize(size_t bytes) {
    if (bytes > 1024 * 1024) return String((float)bytes / 1024.0 / 1024.0, 1) + " MB";
    if (bytes > 1024) return String((float)bytes / 1024.0, 1) + " KB";
    return String(bytes) + " B";
}

void setupFileManagerRoutes(AsyncWebServer& server) {
    // --- File Manager Main Page ---
    server.on("/lb/fileman", HTTP_GET, [](AsyncWebServerRequest* request) {
        String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>LoveByte File Manager</title>
  <meta name="viewport" content="width=360, initial-scale=1">
  <style>
    body { background:#101016; color:#fff; font-family:sans-serif; text-align:center; margin:0; }
    .logo { width:128px; margin:32px auto 20px auto; display:block; border-radius:16px; box-shadow:0 0 24px #2228; }
    .btn { display:inline-block; margin:8px 4px; padding:12px 22px; background:#ff69b4; color:#fff; border:none; border-radius:10px; font-size:1.1em; font-weight:bold; cursor:pointer; transition:background .2s; box-shadow:0 2px 10px #0004;}
    .btn:hover { background:#f032a8; }
    .delbtn { background:#800020; }
    .delbtn:hover { background:#f032a8; }
    .filetbl { margin:20px auto; border-collapse:collapse; width:96%; }
    .filetbl th, .filetbl td { border:1px solid #32324c; padding:8px 4px; }
    .filetbl th { background:#202028; }
    .filetbl tr:nth-child(even) { background:#18181c; }
    .filetbl tr:hover { background:#212140; }
    a, a:visited { color:#fff; text-decoration:underline; }
    .viewimg { max-width:90vw; max-height:60vh; margin:8px auto; border-radius:18px; box-shadow:0 0 24px #2228; display:block; }
    .viewtxt { white-space:pre-wrap; text-align:left; margin:10px auto; width:90%; background:#181824; color:#eee; padding:14px; border-radius:9px;}
  </style>
</head>
<body>
  <img class="logo" src="/res/splash.jpg" alt="LoveByte Logo">
  <h2>File Manager</h2>
  <button class="btn" onclick="location.href='/lb'">&larr; Home</button>
)rawliteral";

        // List images
        html += "<h3>Images</h3><table class='filetbl'><tr><th>File</th><th>Size</th><th>Action</th></tr>";
        File idir = SD_MMC.open("/images");
        if (idir) {
            while (true) {
                File f = idir.openNextFile();
                if (!f) break;
                if (f.isDirectory()) { f.close(); continue; }
                String name = String(f.name());
                if (name.startsWith("/images/")) name = name.substring(8); // Trim prefix
                html += "<tr><td><a href='/lb/fileman/view?type=image&file=" + name + "'>" + name + "</a></td>";
                html += "<td>" + humanSize(f.size()) + "</td>";
                html += "<td>";
                html += "<a class='btn' href='/lb/fileman/view?type=image&file=" + name + "'>View</a> ";
                html += "<a class='btn' href='/images/" + name + "' download>Download</a> ";
                html += "<a class='btn delbtn' href='/lb/fileman/delete?type=image&file=" + name + "' onclick=\"return confirm('Delete this image?');\">Delete</a>";
                html += "</td></tr>";
                f.close();
            }
            idir.close();
        } else {
            html += "<tr><td colspan='3'><i>No images found.</i></td></tr>";
        }
        html += "</table>";

        // List messages
        html += "<h3>Messages</h3><table class='filetbl'><tr><th>File</th><th>Size</th><th>Action</th></tr>";
        File mdir = SD_MMC.open("/messages");
        if (mdir) {
            while (true) {
                File f = mdir.openNextFile();
                if (!f) break;
                if (f.isDirectory()) { f.close(); continue; }
                String name = String(f.name());
                if (name.startsWith("/messages/")) name = name.substring(10); // Trim prefix
                html += "<tr><td><a href='/lb/fileman/view?type=text&file=" + name + "'>" + name + "</a></td>";
                html += "<td>" + humanSize(f.size()) + "</td>";
                html += "<td>";
                html += "<a class='btn' href='/lb/fileman/view?type=text&file=" + name + "'>View</a> ";
                html += "<a class='btn' href='/messages/" + name + "' download>Download</a> ";
                html += "<a class='btn delbtn' href='/lb/fileman/delete?type=text&file=" + name + "' onclick=\"return confirm('Delete this file?');\">Delete</a>";
                html += "</td></tr>";
                f.close();
            }
            mdir.close();
        } else {
            html += "<tr><td colspan='3'><i>No message files found.</i></td></tr>";
        }
        html += "</table>";

        html += "<p style='margin-top:2em;color:#ccc;font-size:.95em;'>&copy; Darkone83</p>";
        html += "</body></html>";
        request->send(200, "text/html", html);
    });

    // --- Download image file ---
    server.on("^\\/images\\/(.+)$", HTTP_GET, [](AsyncWebServerRequest* request) {
        String fname = "/" + request->pathArg(0);
        File file = SD_MMC.open("/images" + fname);
        if (!file || file.isDirectory()) {
            request->send(404, "text/plain", "Image not found");
            return;
        }
        request->send(file, fname, String(), true);
    });

    // --- Download message file ---
    server.on("^\\/messages\\/(.+)$", HTTP_GET, [](AsyncWebServerRequest* request) {
        String fname = "/" + request->pathArg(0);
        File file = SD_MMC.open("/messages" + fname);
        if (!file || file.isDirectory()) {
            request->send(404, "text/plain", "Message file not found");
            return;
        }
        request->send(file, fname, String(), true);
    });

    // --- Themed file view ---
    server.on("/lb/fileman/view", HTTP_GET, [](AsyncWebServerRequest* request) {
        String type = request->hasParam("type") ? request->getParam("type")->value() : "";
        String file = request->hasParam("file") ? request->getParam("file")->value() : "";
        String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>LoveByte File Viewer</title>
  <meta name="viewport" content="width=360, initial-scale=1">
  <style>
    body { background:#101016; color:#fff; font-family:sans-serif; text-align:center; margin:0; }
    .logo { width:128px; margin:32px auto 20px auto; display:block; border-radius:16px; box-shadow:0 0 24px #2228; }
    .btn { display:inline-block; margin:8px 4px; padding:12px 22px; background:#ff69b4; color:#fff; border:none; border-radius:10px; font-size:1.1em; font-weight:bold; cursor:pointer; transition:background .2s; box-shadow:0 2px 10px #0004;}
    .btn:hover { background:#f032a8; }
    .viewimg { max-width:90vw; max-height:60vh; margin:8px auto; border-radius:18px; box-shadow:0 0 24px #2228; display:block; }
    .viewtxt { white-space:pre-wrap; text-align:left; margin:10px auto; width:90%; background:#181824; color:#eee; padding:14px; border-radius:9px;}
  </style>
</head>
<body>
  <img class="logo" src="/res/splash.jpg" alt="LoveByte Logo">
)rawliteral";
        if (type == "image") {
            html += "<h3>Image: " + file + "</h3>";
            html += "<img class='viewimg' src='/images/" + file + "'>";
        } else if (type == "text") {
            File f = SD_MMC.open("/messages/" + file);
            html += "<h3>Text File: " + file + "</h3>";
            if (!f || f.isDirectory()) {
                html += "<div class='viewtxt'>Unable to open file.</div>";
            } else {
                String content;
                while (f.available()) content += (char)f.read();
                html += "<div class='viewtxt'>" + content + "</div>";
                f.close();
            }
        }
        html += "<br><button class='btn' onclick='history.back()'>&larr; Back</button>";
        html += "<p style='margin-top:2em;color:#ccc;font-size:.95em;'>&copy; Darkone83</p></body></html>";
        request->send(200, "text/html", html);
    });

    // --- Delete file (GET for simplicity) ---
    server.on("/lb/fileman/delete", HTTP_GET, [](AsyncWebServerRequest* request) {
        String type = request->hasParam("type") ? request->getParam("type")->value() : "";
        String file = request->hasParam("file") ? request->getParam("file")->value() : "";
        String path = (type == "image" ? "/images/" : "/messages/") + file;
        bool ok = SD_MMC.remove(path);
        String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>LoveByte File Delete</title>
  <meta name="viewport" content="width=360, initial-scale=1">
  <style>
    body { background:#101016; color:#fff; font-family:sans-serif; text-align:center; margin:0; }
    .logo { width:128px; margin:32px auto 20px auto; display:block; border-radius:16px; box-shadow:0 0 24px #2228; }
    .btn { display:inline-block; margin:8px 4px; padding:12px 22px; background:#ff69b4; color:#fff; border:none; border-radius:10px; font-size:1.1em; font-weight:bold; cursor:pointer; transition:background .2s; box-shadow:0 2px 10px #0004;}
    .btn:hover { background:#f032a8; }
    .viewtxt { white-space:pre-wrap; text-align:center; margin:20px auto; width:90%; background:#181824; color:#eee; padding:18px; border-radius:9px;}
  </style>
</head>
<body>
  <img class="logo" src="/res/splash.jpg" alt="LoveByte Logo">
  <h2>Delete File</h2>
)rawliteral";
        if (ok)
            html += "<div class='viewtxt' style='color:#aef;'>Deleted: " + file + "</div>";
        else
            html += "<div class='viewtxt' style='color:#faa;'>Failed to delete: " + file + "</div>";
        html += "<br><button class='btn' onclick='location.href=\"/lb/fileman\"'>Back to File Manager</button>";
        html += "<p style='margin-top:2em;color:#ccc;font-size:.95em;'>&copy; Darkone83</p></body></html>";
        request->send(200, "text/html", html);
    });
}
