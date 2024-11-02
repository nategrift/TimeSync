#include "WifiManager.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "nvs_flash.h"
#include <cstring>
#include <algorithm>

static const char* TAG = "WifiManager";

bool WifiManager::initialized = false;

void WifiManager::init() {
    if (initialized) {
        ESP_LOGI(TAG, "WiFi already initialized");
        return;
    }
    
    ESP_LOGI(TAG, "Initializing WiFi");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    initialized = true;
}

bool WifiManager::turnOn() {
    if (!initialized) {
        init();  // Initialize when actually needed
    }

    ESP_LOGI(TAG, "Turning WiFi on");
    
    // Move WiFi initialization here
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifiEventHandler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifiEventHandler, NULL, NULL));
    
    esp_err_t ret = esp_wifi_start();
    return (ret == ESP_OK);
}

bool WifiManager::turnOff() {
    ESP_LOGI(TAG, "Turning WiFi off");
    
    // First disconnect if connected
    if (isConnected()) {
        disconnect();
    }
    
    // Stop WiFi
    esp_err_t ret = esp_wifi_stop();
    if (ret != ESP_OK) {
        return false;
    }
    
    // Deinitialize WiFi
    ret = esp_wifi_deinit();
    if (ret != ESP_OK) {
        return false;
    }
    
    // Delete default event loop
    ret = esp_event_loop_delete_default();
    if (ret != ESP_OK) {
        return false;
    }
    
    // Deinitialize TCP/IP adapter
    esp_netif_deinit();
    
    initialized = false;
    return true;
}

void WifiManager::deinit() {
    if (!initialized) {
        return;
    }
    
    ESP_LOGI(TAG, "Deinitializing WiFi");
    
    // Turn off WiFi if it's on
    if (isOn()) {
        turnOff();
    }
    
    // Clean up NVS
    nvs_flash_deinit();
    
    initialized = false;
}

bool WifiManager::isOn() {
    return initialized;
}

bool WifiManager::connect() {
    if (!isOn()) {
        if (!turnOn()) {
            return false;
        }
    }

    std::string ssid = ConfigManager::getConfigString("Network", "SSID");
    std::string password = ConfigManager::getConfigString("Network", "Password");

    // Check if SSID or password is too short
    if (ssid.length() < 1 || password.length() < 8) {
        ESP_LOGE(TAG, "SSID or password is too short");
        return false;
    }

    wifi_config_t wifi_config = {};
    std::memcpy(wifi_config.sta.ssid, ssid.c_str(), std::min<size_t>(ssid.length(), sizeof(wifi_config.sta.ssid)));
    std::memcpy(wifi_config.sta.password, password.c_str(), std::min<size_t>(password.length(), sizeof(wifi_config.sta.password)));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    esp_err_t ret = esp_wifi_connect();

    return (ret == ESP_OK);
}

bool WifiManager::disconnect() {
    esp_err_t ret = esp_wifi_disconnect();
    return (ret == ESP_OK);
}

bool WifiManager::isConnected() {
    wifi_ap_record_t ap_info;
    esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);
    return (ret == ESP_OK);
}

int WifiManager::getSignalStrength() {
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        return ap_info.rssi;
    }
    return 0;
}

std::string WifiManager::getIpAddress() {
    esp_netif_ip_info_t ip_info;
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
        char ip_str[16];
        snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));
        return std::string(ip_str);
    }
    return "";
}

void WifiManager::wifiEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "WiFi disconnected. Attempting to reconnect...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));
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
    
    // Set headers
    esp_http_client_set_header(client, "Accept", "application/json");

    // Open connection (write_len = 0 for read-only)
    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }

    // Fetch response headers
    int content_length = esp_http_client_fetch_headers(client);
    if (content_length < 0) {
        ESP_LOGE(TAG, "HTTP client fetch headers failed");
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    // Read response in chunks
    char buffer[512];
    response.clear();
    int read_len;
    
    while ((read_len = esp_http_client_read(client, buffer, sizeof(buffer)-1)) > 0) {
        buffer[read_len] = 0;
        response += buffer;
    }

    if (response.empty()) {
        ESP_LOGE(TAG, "Empty response received");
        err = ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "Response length: %d", response.length());
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return err;
}

bool WifiManager::fetchWorldTime(std::string& errorMsg, time_t& time) {
    if (!isConnected()) {
        if (!connect()) {
            errorMsg = "Error: WiFi not connected";
            return false;
        }
    }

    std::string response;
    esp_err_t err = httpGet("http://worldtimeapi.org/api/ip", response);
    if (err == ESP_OK) {
        ESP_LOGI("WifiManager", "RESPONSE: %s", response.c_str());
        
        // Parse unixtime
        size_t pos = response.find("\"unixtime\"");
        if (pos == std::string::npos) {
            errorMsg = "Error: Could not find unixtime key";
            return false;
        }

        pos = response.find(":", pos);
        if (pos == std::string::npos) {
            errorMsg = "Error: Malformed JSON response";
            return false;
        }

        size_t start = pos + 1;
        size_t end = response.find(",", start);
        std::string unixtime = response.substr(start, end - start);

        // Convert to time_t
        time = std::stoll(unixtime);
        
        ESP_LOGI("WifiManager", "Parsed time: %lld", (long long)time);
        return true;
    } else {
        errorMsg = "Error fetching world time: " + std::to_string(err) + " " + response;
        return false;
    }
}


