#include "driver/gpio.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BUZZER_PIN GPIO_NUM_21
#define BUZZER_CHANNEL LEDC_CHANNEL_1
#define BUZZER_TIMER LEDC_TIMER_1

typedef struct {
    const int* pattern;
    int patternLength;
    int repeatCount;
} buzz_pattern_params_t;

static TaskHandle_t buzzerTaskHandle = NULL;

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

void stop_buzzer() {
    if (buzzerTaskHandle != NULL) {
        // Stop the task
        vTaskDelete(buzzerTaskHandle);
        buzzerTaskHandle = NULL;
    }
    
    // Reset the buzzer
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);
}

void buzz_pattern(const int* pattern, int patternLength) {
    for (int i = 0; i < patternLength; i++) {
        if (pattern[i] > 0) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, 4095); // 50% duty cycle
            ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);
            vTaskDelay(pdMS_TO_TICKS(pattern[i]));
        } else {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, 0);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);
            vTaskDelay(pdMS_TO_TICKS(-pattern[i]));
        }
    }
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);
}

void buzz_pattern_task(void* pvParameters) {
    buzz_pattern_params_t* params = (buzz_pattern_params_t*)pvParameters;
    int repeatsLeft = params->repeatCount;
    
    while (repeatsLeft != 0) {
        buzz_pattern(params->pattern, params->patternLength);
        if (repeatsLeft > 0) {
            repeatsLeft--;
        }
    }
    
    free(params);
    buzzerTaskHandle = NULL;
    vTaskDelete(NULL);
}

void launch_buzz_pattern(const int* pattern, int patternLength, int repeatCount) {
    if (buzzerTaskHandle != NULL) {
        stop_buzzer();
    }

    buzz_pattern_params_t* params = malloc(sizeof(buzz_pattern_params_t));
    params->pattern = pattern;
    params->patternLength = patternLength;
    params->repeatCount = repeatCount;

    xTaskCreate(buzz_pattern_task, "buzz_pattern_task", 2048, params, 5, &buzzerTaskHandle);
}
