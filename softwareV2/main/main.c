#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

static const char *TAG = "salestag-simple";

#define BTN_GPIO 4
#define LED_GPIO 40
#define DEBOUNCE_MS 50

void app_main(void) {
    ESP_LOGI(TAG, "=== SalesTag Simple Test - Button + LED Only ===");
    ESP_LOGI(TAG, "BOOT: Starting simple button test...");
    
    // Configure button GPIO
    gpio_config_t btn_config = {
        .pin_bit_mask = 1ULL << BTN_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&btn_config);
    
    // Configure LED GPIO
    gpio_config_t led_config = {
        .pin_bit_mask = 1ULL << LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&led_config);
    
    // Start with LED OFF
    gpio_set_level(LED_GPIO, 0);
    
    ESP_LOGI(TAG, "GPIO configured - Button: GPIO[%d], LED: GPIO[%d]", BTN_GPIO, LED_GPIO);
    ESP_LOGI(TAG, "Button initial level: %d", gpio_get_level(BTN_GPIO));
    ESP_LOGI(TAG, "=== System Ready ===");
    ESP_LOGI(TAG, "Press button to turn LED ON, release to turn OFF");
    
    // Simple button polling in main loop
    bool last_button_state = false;
    TickType_t last_change = xTaskGetTickCount();
    
    while (true) {
        // Read button state
        bool current_button_state = (gpio_get_level(BTN_GPIO) == 0);  // pressed = LOW
        
        // Check for state change
        if (current_button_state != last_button_state) {
            TickType_t now = xTaskGetTickCount();
            
            // Debounce
            if ((now - last_change) >= pdMS_TO_TICKS(DEBOUNCE_MS)) {
                last_button_state = current_button_state;
                last_change = now;
                
                if (current_button_state) {
                    ESP_LOGI(TAG, "BTN_DOWN - Button pressed");
                    gpio_set_level(LED_GPIO, 1);  // LED ON
                } else {
                    ESP_LOGI(TAG, "BTN_UP - Button released");
                    gpio_set_level(LED_GPIO, 0);  // LED OFF
                }
            }
        }
        
        // Small delay to avoid busy waiting
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
