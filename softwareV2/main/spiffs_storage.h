#pragma once
#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize SD card storage at the given base path
esp_err_t spiffs_storage_init(const char *base_path);

// Deinitialize SD card storage
esp_err_t spiffs_storage_deinit(void);

// Check if storage is mounted
bool spiffs_storage_is_mounted(void);

#ifdef __cplusplus
}
#endif

