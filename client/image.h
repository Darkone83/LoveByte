#ifndef IMAGE_H
#define IMAGE_H

#include <Arduino.h>
#include <vector>

// --- Global GIF state flags ---
// Set true to interrupt current GIF loop
extern volatile bool g_gifStop;

// True if a GIF is currently playing
extern volatile bool g_gifActive;

// Name of currently queued or playing GIF, or "" if none
extern String g_currentGif;

// Call this from your main loop for non-blocking GIF playback.
// Will advance frames if a GIF is active or start new GIF if needed.
void ImageHandler_updateGif();

namespace ImageHandler {
    // Always download from server, then display (JPG or GIF)
    bool receive(const String& filename, const String& serverAddr);

    // Display image/GIF from SD (does not download)
    bool display(const String& filename);

    // Always download, then display (alias for receive)
    bool displayWithDownload(const String& filename, const String& serverAddr);

    // List all images (JPG/JPEG/GIF) in /images, returns just filename (no /images/ prefix)
    std::vector<String> getAllFilenames();

    // Remove all files in /images
    void clearAll();

    // Show "Incoming LoveByte!" popup on the display
    void showIncomingNotification();

    // Optionally, allow main/UI to force-stop any running GIF playback and cleanup
    void stopGifPlayback();
}

#endif
