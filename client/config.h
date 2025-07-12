#pragma once
#include <Arduino.h>

struct DeviceConfig {
    String deviceName;
    int timezone;
    float lat;
    float lon;
    String weatherPostal;
    String weatherCountry;
    String weatherApiKey;
    String serverAddress;    // <-- Added for server string
    String lastNtpTime;
};

namespace Config {
    void begin();
    void save();
    void load();
    DeviceConfig& get();
    void setNtpTime(const String& iso8601);
}
