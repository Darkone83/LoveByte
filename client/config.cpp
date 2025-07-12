#include "config.h"
#include <Preferences.h>
static DeviceConfig cfg;
static Preferences prefs;

void Config::begin() { load(); }

void Config::load() {
    prefs.begin("devcfg", true);
    cfg.deviceName     = prefs.getString("devname", "LoveByte");
    cfg.timezone       = prefs.getInt("tz", 0);
    cfg.lat            = prefs.getFloat("lat", 0.0);
    cfg.lon            = prefs.getFloat("lon", 0.0);
    cfg.weatherApiKey  = prefs.getString("weatherKey", "");
    cfg.weatherPostal  = prefs.getString("postal", "");
    cfg.weatherCountry = prefs.getString("country", "");
    cfg.serverAddress  = prefs.getString("server", "");      // <-- added
    cfg.lastNtpTime    = prefs.getString("ntp", "");
    prefs.end();
}

void Config::save() {
    prefs.begin("devcfg", false);
    prefs.putString("devname",    cfg.deviceName);
    prefs.putInt("tz",            cfg.timezone);
    prefs.putFloat("lat",         cfg.lat);
    prefs.putFloat("lon",         cfg.lon);
    prefs.putString("weatherKey", cfg.weatherApiKey);
    prefs.putString("postal",     cfg.weatherPostal);
    prefs.putString("country",    cfg.weatherCountry);
    prefs.putString("server",     cfg.serverAddress);        // <-- added
    prefs.putString("ntp",        cfg.lastNtpTime);
    prefs.end();
}

DeviceConfig& Config::get() { return cfg; }

void Config::setNtpTime(const String& iso8601) {
    cfg.lastNtpTime = iso8601;
    save();
}
