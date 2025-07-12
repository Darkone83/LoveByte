#pragma once
#include <Arduino.h>
#include <vector>

// --- Message struct for LoveByte with LED/heartbeat support ---
struct Message {
    String text;
    String sender;
    String timeReceived;
    String weather;
    String city;
    String country;
    int tempF = 0;
    String filename; // for reference

    // LED/static color control
    bool useLedColor = false;     // If true, set LED to ledColor
    uint32_t ledColor = 0;        // RGB value (0xFF69B4, etc)

    // Heartbeat (pulse) control
    bool useHeartbeat = false;    // If true, trigger heartbeat
    uint32_t heartbeatColor = 0;  // RGB value for heartbeat
    uint8_t heartbeatPulses = 0;  // Number of pulses

    // --- PATCH: If you need a pretty print helper ---
    String prettyTime(const String& in);
};

namespace MessageHandler {
    // Store a message; fetches weather data automatically.
    bool receive(const String& text, const String& sender, const String& timeReceived);

    // Overload: Support ledColor and heartbeat features (matches your .cpp)
    bool receive(
        const String& text,
        const String& sender,
        const String& timeReceived,
        uint32_t ledColor,
        bool useLedColor,
        bool useHeartbeat,
        uint32_t heartbeatColor,
        uint8_t heartbeatPulses
    );

    // Load a message by index or filename.
    bool load(size_t idx, Message& out);
    bool load(const String& filename, Message& out);

    // Load the latest message.
    bool latest(Message& out);

    // Total message count.
    size_t count();

    // Format a message for display.
    String formatForDisplay(const Message&);

    // Remove a message by index or filename.
    bool remove(size_t idx);
    bool remove(const String& filename);

    // List filenames into provided array; returns actual count.
    void listFilenames(String* arr, size_t max, size_t& actual);

    // Remove all messages.
    void clearAll();

    // Show "Incoming LoveByte" notification (calls external implementation).
    void showIncomingNotification();

    // Get all saved message filenames as a vector.
    std::vector<String> getAllFilenames();

    // Load and display the Nth message.
    bool showMessageOnDisplay(size_t idx);

    // Load and display message by filename.
    bool showMessageOnDisplay(const String& filename);
}
