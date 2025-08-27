#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ui_button_callback_t)(bool pressed, uint32_t ts_ms, void *ctx);

esp_err_t ui_init(int button_gpio, int led_gpio, int debounce_ms);
void ui_set_led(bool on);
void ui_set_button_callback(ui_button_callback_t cb, void *ctx);
void ui_deinit(void);

#ifdef __cplusplus
}
#endif

