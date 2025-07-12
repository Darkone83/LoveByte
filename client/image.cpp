#include "image.h"
#include "settings.h"
#include <SD_MMC.h>
#include <AnimatedGIF.h>
#include <HTTPClient.h>
#include <WiFiClient.h>

extern LGFX display;

// --- Global GIF state ---
volatile bool g_gifStop = false;
volatile bool g_gifActive = false;
String g_currentGif;

// --- Private: GIF playback state ---
static uint8_t* gifBuffer = nullptr;
static size_t gifSize = 0;
static AnimatedGIF gif;
static bool gifNeedsInit = false;

struct RAMGIFHandle { uint8_t *data; size_t size; size_t pos; };
void *GIFOpenRAM(const char *, int32_t *pSize) {
    RAMGIFHandle *h = new RAMGIFHandle{gifBuffer, gifSize, 0};
    *pSize = gifSize;
    return h;
}
void GIFCloseRAM(void *handle) { delete static_cast<RAMGIFHandle*>(handle); }
int32_t GIFReadRAM(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
    RAMGIFHandle *h = static_cast<RAMGIFHandle*>(pFile->fHandle);
    int32_t avail = h->size - h->pos;
    int32_t n = (iLen < avail) ? iLen : avail;
    if (n > 0) {
        memcpy(pBuf, h->data + h->pos, n);
        h->pos += n;
        pFile->iPos = h->pos;
    }
    return n;
}
int32_t GIFSeekRAM(GIFFILE *pFile, int32_t iPosition) {
    RAMGIFHandle *h = static_cast<RAMGIFHandle*>(pFile->fHandle);
    if (iPosition >= 0 && (size_t)iPosition < h->size) {
        h->pos = iPosition;
        pFile->iPos = iPosition;
        return iPosition;
    }
    return -1;
}
void GIFDraw(GIFDRAW *pDraw) {
    if (!pDraw->pPixels || !pDraw->pPalette) return;
    int16_t y = pDraw->iY + pDraw->y;
    if (y < 0 || y >= ::display.height() || pDraw->iX >= ::display.width() || pDraw->iWidth < 1) return;
    int x_offset = (::display.width() - pDraw->iWidth) / 2;
    int y_offset = (::display.height() - pDraw->iHeight) / 2;
    static uint16_t lineBuffer[480];
    for (int x = 0; x < pDraw->iWidth; x++) {
        lineBuffer[x] = pDraw->pPalette[pDraw->pPixels[x]];
    }
    ::display.pushImage(x_offset + pDraw->iX, y_offset + y, pDraw->iWidth, 1, lineBuffer);
}

static bool downloadImageToSD(const String& serverAddr, const String& remoteFilename) {
    String url = "http://" + serverAddr + ":6969/images/" + remoteFilename;
    String sdPath = "/images/" + remoteFilename;
    Serial.printf("[ImageDL] Downloading from: %s\n", url.c_str());

    if (!SD_MMC.exists("/images")) {
        Serial.println("[ImageDL] /images/ does not exist. Creating...");
        if (!SD_MMC.mkdir("/images")) {
            Serial.println("[ImageDL] Failed to create /images/ directory!");
            return false;
        }
    }

    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    Serial.printf("[ImageDL] HTTP GET returned: %d\n", httpCode);
    if (httpCode == 200) {
        File file = SD_MMC.open(sdPath, FILE_WRITE);
        if (!file) {
            Serial.printf("[ImageDL] SD open failed for: %s\n", sdPath.c_str());
            http.end();
            return false;
        }
        WiFiClient* stream = http.getStreamPtr();
        uint8_t buf[2048];
        int total = 0;
        while (http.connected() && (stream->available() > 0 || http.connected())) {
            int avail = stream->available();
            if (avail) {
                int read = stream->readBytes(buf, (avail > sizeof(buf)) ? sizeof(buf) : avail);
                if (read > 0) {
                    file.write(buf, read);
                    total += read;
                }
            } else {
                delay(1);
            }
        }
        file.flush();
        file.close();

        File check = SD_MMC.open(sdPath, FILE_READ);
        if (!check || check.size() == 0) {
            Serial.printf("[ImageDL] Written file missing or size==0! SD_MMC.exists=%d\n", SD_MMC.exists(sdPath.c_str()));
            if (check) check.close();
            http.end();
            return false;
        }
        check.close();
        http.end();
        return true;
    } else {
        Serial.printf("[ImageDL] HTTP failed, code: %d for %s\n", httpCode, url.c_str());
        http.end();
        return false;
    }
}

void ImageHandler::stopGifPlayback() {
    g_gifStop = true;
    if (g_gifActive) {
        gif.close();
        g_gifActive = false;
    }
    if (gifBuffer) {
        heap_caps_free(gifBuffer);
        gifBuffer = nullptr;
    }
    g_currentGif = "";
    gifNeedsInit = false;
}

static void stopGifIfActive() {
    ImageHandler::stopGifPlayback();
}

bool ImageHandler::receive(const String& filename, const String& serverAddr) {
    stopGifIfActive();
    delay(5);

    String imagePath = "/images/" + filename;
    if (!downloadImageToSD(serverAddr, filename)) {
        ::display.fillScreen(TFT_BLACK);
        ::display.setTextColor(TFT_WHITE, TFT_BLACK);
        ::display.setTextSize(2);
        ::display.setCursor(8, ::display.height()/2-12);
        ::display.print("Image download failed");
        delay(1200);
        return false;
    }
    File check = SD_MMC.open(imagePath, FILE_READ);
    if (!check || check.size() == 0) {
        if (check) check.close();
        ::display.fillScreen(TFT_BLACK);
        ::display.setTextColor(TFT_WHITE, TFT_BLACK);
        ::display.setTextSize(2);
        ::display.setCursor(8, ::display.height()/2-12);
        ::display.print("Image download failed");
        delay(1200);
        return false;
    }
    check.close();

    ::display.fillScreen(TFT_BLACK);
    ::display.setTextColor(TFT_PINK, TFT_BLACK);
    ::display.setTextSize(2);
    ::display.setCursor((::display.width()-180)/2, ::display.height()/2-12);
    ::display.print("Incoming LoveByte!");
    delay(500);

    return ImageHandler::display(filename);
}

// --- Only stop GIF if switching to a different GIF or to a non-GIF ---
bool ImageHandler::display(const String& filename) {
    if (filename.endsWith(".gif")) {
        if (!g_gifActive || g_currentGif != filename) {
            stopGifIfActive();
            g_currentGif = filename;
            gifNeedsInit = true;
        }
        return true;
    } else {
        stopGifIfActive();
        g_currentGif = "";
        String path = "/images/" + filename;
        File jpgFile = SD_MMC.open(path, FILE_READ);
        if (jpgFile && jpgFile.size() > 0) {
            size_t jpgSize = jpgFile.size();
            uint8_t* jpgBuffer = (uint8_t*)heap_caps_malloc(jpgSize, MALLOC_CAP_SPIRAM);
            if (jpgBuffer) {
                size_t nRead = jpgFile.read(jpgBuffer, jpgSize);
                jpgFile.close();
                ::display.drawJpg(jpgBuffer, jpgSize, 0, 0, ::display.width(), ::display.height());
                heap_caps_free(jpgBuffer);
                return true;
            }
            jpgFile.close();
        }
        ::display.fillScreen(TFT_BLACK);
        ::display.setTextColor(TFT_WHITE, TFT_BLACK);
        ::display.setTextSize(2);
        ::display.setCursor(8, ::display.height()/2-12);
        ::display.print("Image not found");
        delay(900);
        return false;
    }
}

// --- Non-blocking GIF playback! Call this from loop() ---
void ImageHandler_updateGif() {
    static unsigned long lastFrame = 0;
    static int frameDelay = 0;

    if (!g_currentGif.endsWith(".gif")) return;

    if (gifNeedsInit) {
        // Do NOT stopGifIfActive() here!
        gifNeedsInit = false;

        String path = "/images/" + g_currentGif;
        File f = SD_MMC.open(path, FILE_READ);
        if (!f || f.size() == 0) {
            if (f) f.close();
            return;
        }
        gifSize = f.size();
        gifBuffer = (uint8_t*)heap_caps_malloc(gifSize, MALLOC_CAP_SPIRAM);
        if (gifBuffer) {
            size_t readLen = f.read(gifBuffer, gifSize);
            f.close();
            gif.begin(GIF_PALETTE_RGB565_BE);
            if (gif.open("", GIFOpenRAM, GIFCloseRAM, GIFReadRAM, GIFSeekRAM, GIFDraw)) {
                g_gifActive = true;
                g_gifStop = false;
                frameDelay = 0;
                lastFrame = millis();
            } else {
                heap_caps_free(gifBuffer);
                gifBuffer = nullptr;
                g_gifActive = false;
            }
        }
        return;
    }

    if (g_gifActive && !g_gifStop) {
        if (millis() - lastFrame >= (unsigned int)frameDelay) {
            if (!gif.playFrame(true, &frameDelay)) {
                gif.reset();
                gif.playFrame(true, &frameDelay);
            }
            lastFrame = millis();
            yield();
        }
    } else if (g_gifActive && g_gifStop) {
        gif.close();
        if (gifBuffer) {
            heap_caps_free(gifBuffer);
            gifBuffer = nullptr;
        }
        g_gifActive = false;
        g_currentGif = "";
        gifNeedsInit = false;
    }
}

std::vector<String> ImageHandler::getAllFilenames() {
    std::vector<String> files;
    File dir = SD_MMC.open("/images");
    if (!dir) return files;
    File entry;
    while ((entry = dir.openNextFile())) {
        String name = entry.name();
        if (name.endsWith(".jpg") || name.endsWith(".jpeg") || name.endsWith(".gif")) {
            files.push_back(name.substring(8));
        }
        entry.close();
    }
    dir.close();
    return files;
}

void ImageHandler::clearAll() {
    auto files = getAllFilenames();
    for (auto& f : files) {
        SD_MMC.remove("/images/" + f);
    }
}

void ImageHandler::showIncomingNotification() {
    ::display.fillScreen(TFT_BLACK);
    ::display.setTextColor(TFT_PINK, TFT_BLACK);
    ::display.setTextSize(2);
    ::display.setCursor((::display.width()-180)/2, (::display.height()/2)-12);
    ::display.print("Incoming LoveByte!");
    delay(500);
}
