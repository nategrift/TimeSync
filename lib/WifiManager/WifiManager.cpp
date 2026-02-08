#include "WifiManager.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_heap_caps.h"
#include <cstring>
#include <algorithm>

static const char* TAG = "WifiManager";

bool WifiManager::wifiOn = false;
bool WifiManager::scanning = false;
std::vector<NetworkInfo> WifiManager::networks;

static esp_netif_t* sta_netif = nullptr;

bool WifiManager::turnOn() {
    if (wifiOn) {
        ESP_LOGI(TAG, "WiFi already on");
        return true;
    }
    
    // Log heap status before WiFi init
    size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    size_t largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
    ESP_LOGI(TAG, "Turning WiFi on - Free heap: %d, Largest block: %d", free_heap, largest_block);
    
    // WiFi needs ~60-80KB of contiguous memory
    if (largest_block < 65536) {
        ESP_LOGE(TAG, "Not enough contiguous memory for WiFi! Need 64KB, have %d", largest_block);
        return false;
    }
    
    // Initialize netif only once
    esp_err_t ret = esp_netif_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "netif init failed: %s", esp_err_to_name(ret));
        return false;
    }

    // Create event loop only once
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "event loop failed: %s", esp_err_to_name(ret));
        return false;
    }

    // Create STA netif only if it doesn't exist
    if (!sta_netif) {
        sta_netif = esp_netif_create_default_wifi_sta();
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "wifi init failed: %s", esp_err_to_name(ret));
        return false;
    }

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &eventHandler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &eventHandler, NULL, NULL);
    
    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "set mode failed: %s", esp_err_to_name(ret));
        return false;
    }

    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "wifi start failed: %s", esp_err_to_name(ret));
        return false;
    }
    
    wifiOn = true;
    ESP_LOGI(TAG, "WiFi on - ready for scanning");
    return true;
}

bool WifiManager::turnOff() {
    if (!wifiOn) return true;
    
    ESP_LOGI(TAG, "Turning WiFi off");
    
    if (isConnected()) disconnect();
    
    esp_wifi_stop();
    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, NULL);
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, NULL);
    esp_wifi_deinit();
    
    // Don't destroy netif or event loop - they can be reused
    
    networks.clear();
    wifiOn = false;
    scanning = false;
    return true;
}

bool WifiManager::isOn() {
    return wifiOn;
}

void WifiManager::startScan() {
    if (!wifiOn) {
        ESP_LOGE(TAG, "WiFi not on");
        return;
    }
    if (scanning) {
        ESP_LOGI(TAG, "Scan already running");
        return;
    }
    
    scanning = true;
    ESP_LOGI(TAG, "Starting scan...");
    
    xTaskCreatePinnedToCore([](void* p) {
        wifi_scan_config_t cfg = {};
        cfg.scan_type = WIFI_SCAN_TYPE_ACTIVE;
        cfg.scan_time.active.min = 100;
        cfg.scan_time.active.max = 300;
        
        esp_err_t err = esp_wifi_scan_start(&cfg, true);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Scan start failed: %s", esp_err_to_name(err));
            scanning = false;
            vTaskDelete(NULL);
            return;
        }
        
        uint16_t count = 0;
        esp_wifi_scan_get_ap_num(&count);
        ESP_LOGI(TAG, "Found %d APs", count);
        
        networks.clear();
        
        if (count > 0) {
            uint16_t max = 20;
            uint16_t num = (count < max) ? count : max;
            wifi_ap_record_t* records = new wifi_ap_record_t[num];
            
            if (esp_wifi_scan_get_ap_records(&num, records) == ESP_OK) {
                for (uint16_t i = 0; i < num; i++) {
                    NetworkInfo info;
                    info.ssid = std::string((char*)records[i].ssid);
                    info.rssi = records[i].rssi;
                    info.authMode = records[i].authmode;
                    if (!info.ssid.empty()) {
                        networks.push_back(info);
                        ESP_LOGI(TAG, "  %s (%d dBm)", info.ssid.c_str(), info.rssi);
                    }
                }
            }
            delete[] records;
        }
        
        ESP_LOGI(TAG, "Scan complete: %d networks", networks.size());
        scanning = false;
        vTaskDelete(NULL);
    }, "wifi_scan", 4096, nullptr, 5, nullptr, 1);
}

std::vector<NetworkInfo> WifiManager::getScannedNetworks() {
    return networks;
}

bool WifiManager::isScanInProgress() {
    return scanning;
}

bool WifiManager::connect() {
    if (!wifiOn) {
        ESP_LOGE(TAG, "WiFi not on");
            return false;
    }

    std::string ssid = ConfigManager::getConfigString("Network", "SSID");
    std::string password = ConfigManager::getConfigString("Network", "Password");

    if (ssid.empty()) {
        ESP_LOGE(TAG, "No network configured");
        return false;
    }

    ESP_LOGI(TAG, "Connecting to: %s", ssid.c_str());
    
    if (isConnected()) disconnect();
    
    wifi_config_t cfg = {};
    std::memcpy(cfg.sta.ssid, ssid.c_str(), std::min(ssid.length(), sizeof(cfg.sta.ssid)));
    
    if (!password.empty()) {
        std::memcpy(cfg.sta.password, password.c_str(), std::min(password.length(), sizeof(cfg.sta.password)));
        cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    } else {
        cfg.sta.threshold.authmode = WIFI_AUTH_OPEN;
    }
    cfg.sta.pmf_cfg.capable = true;
    
    esp_wifi_set_config(WIFI_IF_STA, &cfg);
    esp_err_t ret = esp_wifi_connect();
    
    return ret == ESP_OK;
}

bool WifiManager::disconnect() {
    return esp_wifi_disconnect() == ESP_OK;
}

bool WifiManager::isConnected() {
    wifi_ap_record_t info;
    return esp_wifi_sta_get_ap_info(&info) == ESP_OK;
}

int WifiManager::getSignalStrength() {
    wifi_ap_record_t info;
    if (esp_wifi_sta_get_ap_info(&info) == ESP_OK) {
        return info.rssi;
    }
    return 0;
}

std::string WifiManager::getIpAddress() {
    esp_netif_ip_info_t ip_info;
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
        char ip[16];
        snprintf(ip, sizeof(ip), IPSTR, IP2STR(&ip_info.ip));
        return std::string(ip);
    }
    return "";
}

std::string WifiManager::getConnectedSSID() {
    wifi_ap_record_t info;
    if (esp_wifi_sta_get_ap_info(&info) == ESP_OK) {
        return std::string((char*)info.ssid);
    }
    return "";
}

void WifiManager::eventHandler(void* arg, esp_event_base_t base, int32_t id, void* data) {
    if (base == WIFI_EVENT) {
        if (id == WIFI_EVENT_STA_DISCONNECTED) {
            ESP_LOGI(TAG, "Disconnected");
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

esp_err_t WifiManager::httpGet(const char* url, std::string& response) {
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 20000,
        .buffer_size = 2048
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Accept", "application/json");

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        esp_http_client_cleanup(client);
        return err;
    }

    int len = esp_http_client_fetch_headers(client);
    if (len < 0) {
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    char buffer[512];
    response.clear();
    int read_len;
    while ((read_len = esp_http_client_read(client, buffer, sizeof(buffer)-1)) > 0) {
        buffer[read_len] = 0;
        response += buffer;
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return response.empty() ? ESP_FAIL : ESP_OK;
}

bool WifiManager::fetchWorldTime(std::string& errorMsg, time_t& time) {
    if (!isConnected()) {
        errorMsg = "Not connected";
            return false;
    }

    std::string response;
    if (httpGet("http://worldtimeapi.org/api/ip", response) != ESP_OK) {
        errorMsg = "HTTP request failed";
            return false;
        }

    size_t pos = response.find("\"unixtime\"");
        if (pos == std::string::npos) {
        errorMsg = "Invalid response";
        return false;
    }
    
    pos = response.find(":", pos);
    size_t end = response.find(",", pos);
    std::string unixtime = response.substr(pos + 1, end - pos - 1);
    time = std::stoll(unixtime);
    
    return true;
}

void WifiManager::prepareForSleep() {
    ESP_LOGI(TAG, "Preparing for sleep");
    if (isConnected()) disconnect();
    if (wifiOn) turnOff();
}

void WifiManager::resumeFromSleep() {
    ESP_LOGI(TAG, "Resuming from sleep");
    if (ConfigManager::getConfigInt("Network", "Enabled")) {
        std::string ssid = ConfigManager::getConfigString("Network", "SSID");
        if (!ssid.empty()) {
            turnOn();
            connect();
    }
}
}
