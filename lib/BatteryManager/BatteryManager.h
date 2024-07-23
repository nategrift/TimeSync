#ifndef BATTERY_MANAGER_H
#define BATTERY_MANAGER_H

#include "esp_adc_cal.h"
#include <cstdint>

class BatteryManager {
public:
    BatteryManager();
    ~BatteryManager();

    void init();
    float getBatteryVoltage();
    uint8_t getBatteryLevel();

private:
    static const adc1_channel_t ADC_CHANNEL = ADC1_CHANNEL_0;
    static const uint32_t DEFAULT_VREF = 1100;
    static const float VOLTAGE_DIVIDER_RATIO;
    
    esp_adc_cal_characteristics_t* adc_chars;
    bool initialized;

    uint32_t readVoltage();
};

#endif // BATTERY_MANAGER_H
