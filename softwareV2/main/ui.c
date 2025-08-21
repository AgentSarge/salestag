#include "ui.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG_UI = "ui";
static int s_btn = -1, s_led = -1, s_db_ms = 10;
static ui_button_callback_t s_cb = NULL;
static void *s_cb_ctx = NULL;
static TaskHandle_t s_poll = NULL;

static void ui_poll_task(void *arg){
    (void)arg;
    // Button has pullup enabled, so unpressed = HIGH (1), pressed = LOW (0)
    bool stable = (gpio_get_level(s_btn) == 1);  // Start with unpressed state
    TickType_t last_change = xTaskGetTickCount();
    
    ESP_LOGI(TAG_UI, "Button polling task started. Initial button state: %s", stable ? "UNPRESSED" : "PRESSED");
    
    while (true){
        bool raw = (gpio_get_level(s_btn) == 0);  // pressed = LOW (0)
        TickType_t now = xTaskGetTickCount();
        
        if (raw != stable){
            if ((now - last_change) >= pdMS_TO_TICKS(s_db_ms)){
                stable = raw;
                ESP_LOGI(TAG_UI, "Button state changed to: %s", stable ? "PRESSED" : "UNPRESSED");
                if (s_cb){ s_cb(stable, (uint32_t)now, s_cb_ctx); }
                ESP_LOGD(TAG_UI, "%s", stable?"BTN_DOWN":"BTN_UP");
                last_change = now;
            }
        } else {
            // update timer baseline
            if ((now - last_change) > pdMS_TO_TICKS(s_db_ms)){
                last_change = now;
            }
        }
        
        // Yield more frequently to avoid watchdog
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

esp_err_t ui_init(int button_gpio, int led_gpio, int debounce_ms){
    s_btn = button_gpio; s_led = led_gpio; s_db_ms = debounce_ms;
    gpio_config_t in = {
        .pin_bit_mask = 1ULL << s_btn,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&in));
    gpio_config_t out = {
        .pin_bit_mask = 1ULL << s_led,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&out));
    gpio_set_level(s_led, 0);

    BaseType_t ok = xTaskCreate(ui_poll_task, "ui_btn", 3072, NULL, 6, &s_poll);
    return ok == pdPASS ? ESP_OK : ESP_FAIL;
}

void ui_set_led(bool on){ if (s_led >= 0) gpio_set_level(s_led, on ? 1 : 0); }
void ui_set_button_callback(ui_button_callback_t cb, void *ctx){ s_cb = cb; s_cb_ctx = ctx; }
void ui_deinit(void){ if (s_poll){ vTaskDelete(s_poll); s_poll = NULL; } }

