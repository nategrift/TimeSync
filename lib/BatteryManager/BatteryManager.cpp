#include "BatteryManager.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"

static const char* TAG = "BatteryManager";
const float BatteryManager::VOLTAGE_DIVIDER_RATIO = 3.0;

BatteryManager::BatteryManager() : adc_chars(nullptr), initialized(false) {}

BatteryManager::~BatteryManager() {
    if (adc_chars) {
        free(adc_chars);
    }
}

void BatteryManager::init() {
    if (initialized) return;

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN_DB_11);
    adc_chars = (esp_adc_cal_characteristics_t*)calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
    initialized = true;
}

uint32_t BatteryManager::readVoltage() {
    if (!initialized) {
        init();
    }

    uint32_t adc_reading = adc1_get_raw(ADC_CHANNEL);
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    return voltage;
}

float BatteryManager::getBatteryVoltage() {
    uint32_t raw_voltage = readVoltage();
    float battery_voltage = raw_voltage * VOLTAGE_DIVIDER_RATIO;
    float battery_voltage_in_volts = battery_voltage / 1000.0; // Convert to volts

    ESP_LOGI(TAG, "Battery Voltage: %.2fV", battery_voltage_in_volts);

    return battery_voltage_in_volts;
}

uint8_t BatteryManager::getBatteryLevel() {
    float battery_voltage = getBatteryVoltage();
    uint8_t battery_level = 0;


    
    if (battery_voltage >= 4.2) {
        battery_level = 100;
    } else if (battery_voltage <= 3.0) {
        battery_level = 0;
    } else {
        battery_level = (int)((battery_voltage - 3.0) * 100.0 / (4.0 - 3.0));
    }

    return battery_level;
}

bool BatteryManager::getBatteryCharging() {
    float battery_voltage = getBatteryVoltage();
    if (battery_voltage >= 4.2) {
        return true;
    }

    return false;
}
