#include "ui.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG_UI = "ui";
static int s_btn = -1, s_led = -1, s_db_ms = 50;  // Increased debounce to 50ms
static ui_button_callback_t s_cb = NULL;
static void *s_cb_ctx = NULL;
static TaskHandle_t s_poll = NULL;

static void ui_poll_task(void *arg){
    (void)arg;
    
    // Wait for GPIO to stabilize after system boot
    vTaskDelay(pdMS_TO_TICKS(100));
    
    ESP_LOGI(TAG_UI, "Button polling task started");
    ESP_LOGI(TAG_UI, "GPIO[%d] initial level: %d", s_btn, gpio_get_level(s_btn));
    
    // Simple button polling with debouncing (from working diagnostic build)
    bool last_button_state = false;  // Start with unpressed state
    TickType_t last_change = xTaskGetTickCount();
    
    // Add noise filtering - require multiple consecutive readings
    int consecutive_readings = 0;
    const int required_readings = 3;  // Require 3 consecutive readings to confirm state
    
    while (true) {
        // Read button state multiple times to filter noise
        int gpio_level = gpio_get_level(s_btn);
        bool current_button_state = (gpio_level == 0);  // pressed = LOW
        
        // Check for state change
        if (current_button_state != last_button_state) {
            TickType_t now = xTaskGetTickCount();
            
            // Add debug output for state changes
            ESP_LOGI(TAG_UI, "Button state change detected: GPIO=%d, current=%s, last=%s", 
                     gpio_level, current_button_state ? "PRESSED" : "UNPRESSED", 
                     last_button_state ? "PRESSED" : "UNPRESSED");
            
            // Debounce
            if ((now - last_change) >= pdMS_TO_TICKS(s_db_ms)) {
                // Additional noise filtering - require consecutive readings
                consecutive_readings++;
                if (consecutive_readings >= required_readings) {
                    last_button_state = current_button_state;
                    last_change = now;
                    consecutive_readings = 0;  // Reset counter
                    
                    ESP_LOGI(TAG_UI, "Button state changed to: %s (debounced + noise filtered)", current_button_state ? "PRESSED" : "UNPRESSED");
                    
                    if (s_cb) {
                        s_cb(current_button_state, (uint32_t)now, s_cb_ctx);
                    }
                } else {
                    ESP_LOGI(TAG_UI, "Button state change pending: %d/%d consecutive readings", consecutive_readings, required_readings);
                }
            } else {
                ESP_LOGI(TAG_UI, "Button state change ignored (debounce period)");
                consecutive_readings = 0;  // Reset counter on debounce
            }
        } else {
            // Reset consecutive counter when state is stable
            consecutive_readings = 0;
        }
        
        // Small delay to avoid busy waiting
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

esp_err_t ui_init(int button_gpio, int led_gpio, int debounce_ms){
    s_btn = button_gpio; s_led = led_gpio; s_db_ms = debounce_ms;
    
    // Button GPIO configuration
    gpio_config_t btn_config = {
        .pin_bit_mask = 1ULL << s_btn,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&btn_config));
    
    // LED GPIO configuration
    gpio_config_t led_config = {
        .pin_bit_mask = 1ULL << s_led,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&led_config));
    
    // Start with LED OFF
    gpio_set_level(s_led, 0);

    // Create button polling task
    BaseType_t ok = xTaskCreate(ui_poll_task, "ui_btn", 3072, NULL, 6, &s_poll);
    return ok == pdPASS ? ESP_OK : ESP_FAIL;
}

void ui_set_led(bool on) { 
    ESP_LOGI(TAG_UI, "ui_set_led called: on=%d, s_led=%d", on, s_led);
    if (s_led >= 0) {
        gpio_set_level(s_led, on ? 1 : 0);
        ESP_LOGI(TAG_UI, "GPIO[%d] set to %d", s_led, on ? 1 : 0);
    } else {
        ESP_LOGE(TAG_UI, "LED GPIO not initialized (s_led=%d)", s_led);
    }
}
void ui_set_button_callback(ui_button_callback_t cb, void *ctx){ s_cb = cb; s_cb_ctx = ctx; }
void ui_deinit(void){ if (s_poll){ vTaskDelete(s_poll); s_poll = NULL; } }

