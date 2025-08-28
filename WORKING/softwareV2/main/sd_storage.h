#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// SD card storage configuration
#define SD_MOUNT_POINT "/sdcard"
#define SD_REC_DIR "/sdcard/rec"
#define SD_SPI_HOST SPI2_HOST
#define SD_CS_PIN 39
#define SD_MOSI_PIN 35
#define SD_MISO_PIN 37
#define SD_SCLK_PIN 36
#define SD_SPI_FREQ_MHZ 10

// SD card status
typedef enum {
    SD_STATUS_UNMOUNTED = 0,
    SD_STATUS_MOUNTED,
    SD_STATUS_ERROR,
    SD_STATUS_NO_CARD
} sd_status_t;

// SD card information
typedef struct {
    uint64_t total_bytes;
    uint64_t free_bytes;
    bool is_mounted;
    sd_status_t status;
} sd_info_t;

// Initialize SD card storage
esp_err_t sd_storage_init(void);

// Deinitialize SD card storage
esp_err_t sd_storage_deinit(void);

// Get SD card status and information
esp_err_t sd_storage_get_info(sd_info_t *info);

// Check if SD card is available for recording
bool sd_storage_is_available(void);

// Create recording directory structure
esp_err_t sd_storage_create_rec_dir(void);

// Get the recording directory path
const char* sd_storage_get_rec_path(void);

// Graceful fallback when SD card unavailable
esp_err_t sd_storage_fallback_to_internal(void);

// Power cycle the SD card to reset its state
esp_err_t sd_storage_power_cycle(void);

// Test write access with retry logic
esp_err_t sd_storage_test_write_access(void);

#ifdef __cplusplus
}
#endif
