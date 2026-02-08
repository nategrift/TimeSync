#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <string>
#include <vector>
#include "esp_wifi.h"
#include "ConfigManager.h"
#include <time.h>

struct NetworkInfo {
    std::string ssid;
    int8_t rssi;
    wifi_auth_mode_t authMode;
    
    std::string getAuthModeString() const {
        switch (authMode) {
            case WIFI_AUTH_OPEN: return "Open";
            case WIFI_AUTH_WEP: return "WEP";
            case WIFI_AUTH_WPA_PSK: return "WPA";
            case WIFI_AUTH_WPA2_PSK: return "WPA2";
            case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
            case WIFI_AUTH_WPA3_PSK: return "WPA3";
            case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
            default: return "Unknown";
        }
    }
    
    std::string getSignalQuality() const {
        if (rssi >= -50) return "Excellent";
        if (rssi >= -60) return "Good";
        if (rssi >= -70) return "Fair";
        return "Weak";
    }
    
    bool isOpen() const { return authMode == WIFI_AUTH_OPEN; }
};

class WifiManager {
public:
    // Power control
    static bool turnOn();
    static bool turnOff();
    static bool isOn();
    
    // Scanning
    static void startScan();
    static std::vector<NetworkInfo> getScannedNetworks();
    static bool isScanInProgress();
    
    // Connection (uses saved config)
    static bool connect();
    static bool disconnect();
    static bool isConnected();
    
    // Network info
    static int getSignalStrength();
    static std::string getIpAddress();
    static std::string getConnectedSSID();
    
    // Utilities
    static bool fetchWorldTime(std::string& errorMsg, time_t& time);
    
    // Sleep/Wake
    static void prepareForSleep();
    static void resumeFromSleep();

private:
    static bool wifiOn;
    static bool scanning;
    static std::vector<NetworkInfo> networks;
    
    static void eventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    static esp_err_t httpGet(const char* url, std::string& response);
};

#endif
