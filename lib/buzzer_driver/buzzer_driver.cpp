extern "C" {
    #include "driver/gpio.h"
    #include "driver/ledc.h"
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "esp_log.h"
    #include <string.h>
}

#include "ConfigManager.h"

#include "buzzer_driver.h"

#define BUZZER_PIN GPIO_NUM_21
#define BUZZER_CHANNEL LEDC_CHANNEL_1
#define BUZZER_TIMER LEDC_TIMER_1

static const char* TAG = "BuzzerDriver";

typedef struct {
    const int* pattern;
    int patternLength;
    int repeatCount;
} buzz_pattern_params_t;

static TaskHandle_t buzzerTaskHandle = NULL;
static buzz_pattern_params_t* currentBuzzParams = NULL;

void init_buzzer() {
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .timer_num = BUZZER_TIMER,
        .freq_hz = 4000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .gpio_num = BUZZER_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = BUZZER_CHANNEL,
        .timer_sel = BUZZER_TIMER,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&ledc_channel);
}

bool is_buzzer_task_running() {
    if (buzzerTaskHandle == NULL) {
        return false;
    }
    
    eTaskState taskState = eTaskGetState(buzzerTaskHandle);
    return (taskState == eRunning || taskState == eReady || taskState == eBlocked);
}

void stop_buzzer() {
    if (is_buzzer_task_running()) {
        vTaskDelete(buzzerTaskHandle);
    }
    buzzerTaskHandle = NULL;
    
    if (currentBuzzParams != NULL) {
        free(currentBuzzParams);
        currentBuzzParams = NULL;
    }
    
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);
}

void buzz_pattern(const int* pattern, int patternLength) {
    ESP_LOGI(TAG, "Buzzer Pattern launched");
    bool isMuted = ConfigManager::getConfigInt("General", "Mute") != 0;
    int volume = ConfigManager::getConfigInt("General", "Volume");
    
    if (isMuted) {
        ESP_LOGI(TAG, "Buzzer changed to muted");
        return;
    }

    for (int i = 0; i < patternLength; i++) {
        if (pattern[i] > 0) {
            ESP_LOGI(TAG, "Playing Pattern");
            int duty = (4095 * volume) / 10;  // Scale duty based on volume (1-10)
            ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, duty);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);
            vTaskDelay(pdMS_TO_TICKS(pattern[i]));
        } else {
            ESP_LOGI(TAG, "Pausing Pattern");
            ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, 0);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);
            vTaskDelay(pdMS_TO_TICKS(-pattern[i]));
        }
    }
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);
}

void buzz_pattern_task(void* pvParameters) {
    if (currentBuzzParams == NULL) {
        return;
    }

    int repeatsLeft = currentBuzzParams->repeatCount;
    while (repeatsLeft != 0) {
        buzz_pattern(currentBuzzParams->pattern, currentBuzzParams->patternLength);
        if (repeatsLeft > 0) {
            repeatsLeft--;
        }
    }
    
    stop_buzzer();
}

void launch_buzz_pattern(const int* pattern, int patternLength, int repeatCount) {
    bool isMuted = ConfigManager::getConfigInt("General", "Mute") != 0;
    
    if (isMuted) {
        return;
    }

    stop_buzzer();

    currentBuzzParams = (buzz_pattern_params_t*)malloc(sizeof(buzz_pattern_params_t));
    if (currentBuzzParams == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for buzz parameters");
        return;
    }

    // Allocate memory for the pattern copy
    int* patternCopy = (int*)malloc(patternLength * sizeof(int));
    if (patternCopy == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for pattern copy");
        free(currentBuzzParams);
        return;
    }

    // Copy the pattern
    memcpy(patternCopy, pattern, patternLength * sizeof(int));

    currentBuzzParams->pattern = patternCopy;
    currentBuzzParams->patternLength = patternLength;
    currentBuzzParams->repeatCount = repeatCount;

    xTaskCreate(buzz_pattern_task, "buzz_pattern_task", 4096, NULL, 5, &buzzerTaskHandle);
}

void beep(int ms_duration) {
    const int pattern[] = {
        ms_duration
    };
    
    launch_buzz_pattern(pattern, 1, 3);
}