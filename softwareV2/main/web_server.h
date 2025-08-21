#pragma once
#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize web server
esp_err_t web_server_init(void);

// Start web server
esp_err_t web_server_start(void);

// Stop web server
esp_err_t web_server_stop(void);

// Deinitialize web server
esp_err_t web_server_deinit(void);

#ifdef __cplusplus
}
#endif
