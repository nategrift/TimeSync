#include "WifiDebug.h"


#include "WifiManager.h"
#include "app_screen.h"
#include "ui_components.h"
#include "VibrationDriver.h"
#include <sys/time.h>

WifiDebug::WifiDebug(AppManager& manager) 
    : IApp("WifiDebug"), appManager(manager), screenObj(NULL), textArea(NULL), refreshBtn(NULL) {}

WifiDebug::~WifiDebug() {}

void WifiDebug::refresh_event_handler(lv_event_t* e) {
    WifiDebug* wifiDebug = (WifiDebug*)lv_event_get_user_data(e);
    VibrationDriver::quickVibration(100);
    wifiDebug->updateDisplay();
}

void WifiDebug::launch() {
    screenObj = get_app_container(appManager);

    // Create title
    lv_obj_t* titleLabel = lv_label_create(screenObj);
    lv_label_set_text(titleLabel, "WiFi Debug");
    lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 10);

    // Style for title
    static lv_style_t style_title;
    lv_style_init(&style_title);
    lv_style_set_text_color(&style_title, lv_color_hex(0x00FF00));
    lv_obj_add_style(titleLabel, &style_title, 0);

    // Create scrollable text area
    textArea = lv_textarea_create(screenObj);
    lv_obj_set_size(textArea, 220, 160);
    lv_obj_align(textArea, LV_ALIGN_TOP_MID, 0, 40);
    lv_textarea_set_text(textArea, "Loading...");
    lv_obj_add_flag(textArea, LV_OBJ_CLASS_EDITABLE_FALSE);

    // Style for text area
    static lv_style_t style_ta;
    lv_style_init(&style_ta);
    lv_style_set_text_color(&style_ta, lv_color_hex(0xFFFFFF));
    lv_style_set_bg_color(&style_ta, lv_color_hex(0x000000));
    lv_obj_add_style(textArea, &style_ta, 0);

    // Create refresh button
    refreshBtn = get_button(screenObj, "Refresh");
    lv_obj_align(refreshBtn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(refreshBtn, refresh_event_handler, LV_EVENT_CLICKED, this);

    // Initial display update
    updateDisplay();
}

void WifiDebug::updateDisplay() {
    std::string info;
    
    // WiFi Status
    info += "WiFi Power: " + std::string(WifiManager::isOn() ? "ON" : "OFF") + "\n";
    info += "Connected: " + std::string(WifiManager::isConnected() ? "YES" : "NO") + "\n";
    info += "SSID: " + ConfigManager::getConfigString("Network", "SSID") + "\n\n";

    // Add WiFi technical details
    uint8_t protocol;
    wifi_bandwidth_t bandwidth;
    uint8_t primary;
    wifi_second_chan_t second;
    wifi_country_t country;
    uint8_t mac[6];
    
    if (esp_wifi_get_protocol(WIFI_IF_STA, &protocol) == ESP_OK) {
        info += "Protocol: " + std::to_string(protocol) + "\n";
    }
    
    if (esp_wifi_get_bandwidth(WIFI_IF_STA, &bandwidth) == ESP_OK) {
        info += "Bandwidth: " + std::to_string(bandwidth) + "\n";
    }
    
    if (esp_wifi_get_channel(&primary, &second) == ESP_OK) {
        info += "Primary Channel: " + std::to_string(primary) + "\n";
        // Convert second channel enum to readable string
        std::string secondChanStr;
        switch (second) {
            case WIFI_SECOND_CHAN_NONE:
                secondChanStr = "None";
                break;
            case WIFI_SECOND_CHAN_ABOVE:
                secondChanStr = "Above";
                break;
            case WIFI_SECOND_CHAN_BELOW:
                secondChanStr = "Below";
                break;
            default:
                secondChanStr = "Unknown";
        }
        info += "Secondary Channel: " + secondChanStr + "\n";
    }
    
    if (esp_wifi_get_country(&country) == ESP_OK) {
        info += "Country: " + std::string(country.cc) + "\n";
    }
    
    if (esp_wifi_get_mac(WIFI_IF_STA, mac) == ESP_OK) {
        char mac_str[18];
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        info += "MAC: " + std::string(mac_str) + "\n\n";
    }

    if (WifiManager::isConnected()) {
        info += "IP: " + WifiManager::getIpAddress() + "\n";
        info += "Signal: " + std::to_string(WifiManager::getSignalStrength()) + " dBm\n\n";
    }

    if (textArea && lv_obj_is_valid(textArea)) {
        lv_textarea_set_text(textArea, info.c_str());
    }
}

void WifiDebug::close() {
    // Cleanup will be handled by the framework
}

void WifiDebug::backgroundActivity() {
    // Not used
}