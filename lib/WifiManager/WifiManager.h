#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <string>
#include "esp_wifi.h"
#include "ConfigManager.h"
#include <time.h>

class WifiManager {
private:
    static bool initialized;
public:
    static void init();
    static void deinit();
    static bool turnOn();
    static bool turnOff();
    static bool isOn();
    static bool connect();
    static bool disconnect();
    static bool isConnected();
    static int getSignalStrength();
    static std::string getIpAddress();
    static bool fetchWorldTime(std::string& errorMsg, time_t& time);

private:
    static void wifiEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    static esp_err_t httpGet(const char* url, std::string& response);
};

#endif // WIFIMANAGER_H

