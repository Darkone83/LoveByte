#include "web_message.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include "config.h"
#include <HTTPClient.h>

static String htmlHeader() {
    return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>LoveByte Cloud Messenger</title>
  <meta name="viewport" content="width=360, initial-scale=1">
  <style>
    body { background:#101016; color:#fff; font-family:sans-serif; text-align:center; margin:0; }
    .section { background:#181825; margin:22px auto; max-width:340px; padding:20px 20px 12px 20px; border-radius:12px; box-shadow:0 2px 14px #0006; text-align:left;}
    h2 { text-align:center; margin-bottom:0.4em;}
    .btn { display:inline-block; margin:8px 2px; padding:10px 20px; background:#ff69b4; color:#fff; border:none; border-radius:8px; font-size:1.1em; font-weight:bold; letter-spacing:1px; cursor:pointer; transition:background .2s; box-shadow:0 2px 8px #0004;}
    .btn:disabled { opacity:0.5; background:#444;}
    .btn:hover { background:#f032a8; }
    label { font-weight:bold; }
    textarea { width:95%; min-height:60px; border-radius:6px; font-size:1.1em; margin:6px 0 10px 0; border:1px solid #555; }
    .result { color:#ffb; margin: 14px 0 0 0; font-size:1em;}
  </style>
</head>
<body>
  <h2>LoveByte Cloud Messenger</h2>
)rawliteral";
}

static String htmlFooter(const String& serverAddr, const String& deviceName) {
    String js;
    js += "<p style=\"margin-top:2em;color:#ccc;font-size:.9em;\">&copy; Darkone83</p>\n";
    js += "<script>\n";
    js += "document.addEventListener('DOMContentLoaded', function() {\n";
    js += "  const SERVER_HOST = '';\n"; // Proxy pattern: API calls go to this ESP32, not Python
    js += "  const DEVICE_ID = '" + deviceName + "';\n";

    // Color/heartbeat controls
    js += "  const ledCheck = document.getElementById('use_led');\n";
    js += "  const colorInput = document.getElementById('color');\n";
    js += "  const hbCheck = document.getElementById('use_hb');\n";
    js += "  const hbColorInput = document.getElementById('heartbeat_color');\n";
    js += "  const hbPulsesInput = document.getElementById('heartbeat_pulses');\n";
    js += "  ledCheck.onchange = function() { colorInput.disabled = !this.checked; };\n";
    js += "  hbCheck.onchange = function() {\n";
    js += "    hbColorInput.disabled = !this.checked;\n";
    js += "    hbPulsesInput.disabled = !this.checked;\n";
    js += "  };\n";
    js += "  colorInput.disabled = !ledCheck.checked;\n";
    js += "  hbColorInput.disabled = !hbCheck.checked;\n";
    js += "  hbPulsesInput.disabled = !hbCheck.checked;\n";

    // Message form submit with validation and sender set as DEVICE_ID
    js += "  document.getElementById('msgform').onsubmit = function(e){\n";
    js += "    e.preventDefault();\n";
    js += "    var recipient = document.getElementById('recipient').value.trim();\n";
    js += "    if(recipient) recipient = recipient.replace(/\\s+/g, '');\n";
    js += "    if(!recipient) { alert('Please enter a valid Target Device ID'); return; }\n";
    js += "    var msg = document.getElementById('msg').value.trim();\n";
    js += "    if(!msg) { alert('Message cannot be empty'); return; }\n";
    js += "    var useLed = ledCheck.checked;\n";
    js += "    var ledColor = colorInput.value.replace('#','').toUpperCase();\n";
    js += "    var useHb = hbCheck.checked;\n";
    js += "    var hbColor = hbColorInput.value.replace('#','').toUpperCase();\n";
    js += "    var pulses = parseInt(hbPulsesInput.value, 10) || 0;\n";
    js += "    var payload = {\n";
    js += "      recipient: recipient,\n";
    js += "      text: msg,\n";
    js += "      sender: DEVICE_ID,\n";
    js += "      time: new Date().toISOString().slice(0,19).replace('T',' '),\n";
    js += "      weather: '', city: '', country: '', tempF: 0,\n";
    js += "      ledColor: ledColor,\n";
    js += "      useLedColor: useLed,\n";
    js += "      useHeartbeat: useHb,\n";
    js += "      heartbeatColor: hbColor,\n";
    js += "      heartbeatPulses: pulses\n";
    js += "    };\n";
    js += "    console.log('Payload to send:', payload);\n";
    js += "    var result = document.getElementById('msg_result');\n";
    js += "    result.innerText = \"Sending...\";\n";
    js += "    fetch('/api/push', {\n"; // API request goes to ESP32, which proxies
    js += "      method:'POST',\n";
    js += "      headers:{'Content-Type':'application/json'},\n";
    js += "      body:JSON.stringify(payload)\n";
    js += "    })\n";
    js += "    .then(r => r.ok ? r.json() : r.text().then(txt => { throw txt; }))\n";
    js += "    .then(j => {\n";
    js += "      console.log('Response:', j);\n";
    js += "      result.innerText = (j.status === 'queued') ? 'Message sent!' : ('Failed: '+(j.error||'Unknown'));\n";
    js += "    })\n";
    js += "    .catch(e => { result.innerText = 'Error: ' + e; });\n";
    js += "  };\n";

    // Image form submits directly to python server as before
    js += "  document.getElementById('imgform').onsubmit = function(e){\n";
    js += "    e.preventDefault();\n";
    js += "    var recipient = document.getElementById('recipient_img').value.trim();\n";
    js += "    if(recipient) recipient = recipient.replace(/\\s+/g, '');\n";
    js += "    if (!recipient) { alert('Please enter a valid Target Device ID'); return; }\n";
    js += "    var fileInput = document.getElementById('file');\n";
    js += "    var result = document.getElementById('img_result');\n";
    js += "    if (!fileInput.files.length) {\n";
    js += "      result.innerText = \"Recipient and file required.\";\n";
    js += "      return;\n";
    js += "    }\n";
    js += "    var fd = new FormData();\n";
    js += "    fd.append('file', fileInput.files[0]);\n";
    js += "    fd.append('recipient', recipient);\n";
    js += "    result.innerText = \"Sending...\";\n";
    js += "    fetch('http://" + serverAddr + ":6969/api/upload_image', {\n";
    js += "      method: 'POST', body: fd\n";
    js += "    })\n";
    js += "    .then(r => r.ok ? r.json() : r.text().then(txt => { throw txt; }))\n";
    js += "    .then(j => {\n";
    js += "      console.log('Image upload response:', j);\n";
    js += "      result.innerText = (j.status === 'uploaded') ? 'Image sent!' : ('Failed: '+(j.error||j.result));\n";
    js += "    })\n";
    js += "    .catch(e => { result.innerText = 'Error: ' + e; });\n";
    js += "  };\n";

    js += "});\n";
    js += "</script>\n</body>\n</html>\n";
    return js;
}

void setupMessagePageRoutes(AsyncWebServer& server) {
    server.on("/lb/cloud", HTTP_GET, [](AsyncWebServerRequest* request){
        String deviceName = Config::get().deviceName;
        String serverAddr = Config::get().serverAddress;
        if (serverAddr.startsWith("http://"))
            serverAddr = serverAddr.substring(7);
        else if (serverAddr.startsWith("https://"))
            serverAddr = serverAddr.substring(8);

        String html = htmlHeader();

        // Message sending section
        html += "<div class='section'><form id='msgform'>";
        html += "<label>Target Device ID:</label><br>";
        html += "<input type='text' id='recipient' value='" + deviceName + "' style='width:92%;margin-bottom:8px'><br>";
        html += "<label>Message:</label><br>";
        html += "<textarea id='msg' placeholder='Type your message here'></textarea><br>";
        html += "<label><input type='checkbox' id='use_led'> Use LED Color </label>";
        html += "<input type='color' id='color' value='#ffffff' disabled><br>";
        html += "<label><input type='checkbox' id='use_hb'> Use Heartbeat </label>";
        html += "<input type='color' id='heartbeat_color' value='#ff69b4' disabled>";
        html += "<label style='margin-left:12px'>Pulses:</label>";
        html += "<input type='number' id='heartbeat_pulses' min='1' max='10' value='3' disabled style='width:46px'><br>";
        html += "<button class='btn' type='submit'>Send Message</button>";
        html += "<div class='result' id='msg_result'></div>";
        html += "</form></div>";

        // Image/GIF sending section
        html += "<div class='section'><form id='imgform' enctype='multipart/form-data'>";
        html += "<label>Target Device ID:</label><br>";
        html += "<input type='text' id='recipient_img' value='" + deviceName + "' style='width:92%;margin-bottom:8px'><br>";
        html += "<label>Send Image/GIF:</label><br>";
        html += "<input type='file' name='file' id='file' accept='image/jpeg,image/png,image/gif' required><br><br>";
        html += "<button class='btn' type='submit'>Send Image/GIF</button>";
        html += "<div class='result' id='img_result'></div>";
        html += "</form></div>";

        html += htmlFooter(serverAddr, deviceName);
        request->send(200, "text/html", html);
    });

    // --- Proxy /api/push to Python server ---
    server.on("/api/push", HTTP_POST, [](AsyncWebServerRequest *request){},
      NULL,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
          String serverAddr = Config::get().serverAddress; // Python server IP, no http://
          String url = "http://" + serverAddr + ":6969/api/push";
          String payload((char*)data, len);

          HTTPClient http;
          http.begin(url);
          http.addHeader("Content-Type", "application/json");
          int httpCode = http.POST(payload);

          String response = http.getString();
          String type = http.header("Content-Type");
          if (type.length() == 0) type = "application/json";

          http.end();

          request->send(httpCode, type, response);
      }
    );
}
