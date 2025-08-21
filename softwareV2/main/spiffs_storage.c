#include "spiffs_storage.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "driver/gpio.h"
#include <errno.h>
#include <string.h>

static const char *TAG = "sd_storage";
static sdmmc_card_t *s_card = NULL;
static bool s_mounted = false;

esp_err_t spiffs_storage_init(const char *base_path) {
    ESP_LOGI(TAG, "Initializing SD card storage at %s", base_path);
    
    // SD card configuration
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    
    // Use SPI mode for better compatibility
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = 35,  // GPIO35 - MOSI (from hardware docs)
        .miso_io_num = 37,  // GPIO37 - MISO (from hardware docs)
        .sclk_io_num = 36,  // GPIO36 - SCLK (from hardware docs)
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    
    // Initialize SPI bus
    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // SD card slot configuration
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = 39;  // GPIO39 - CS (from hardware docs)
    slot_config.host_id = SPI2_HOST;
    
    // Mount SD card
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    
    ret = esp_vfs_fat_sdspi_mount(base_path, &host, &slot_config, &mount_config, &s_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
        return ret;
    }
    
    s_mounted = true;
    
    // Create /rec directory if it doesn't exist
    char rec_path[64];
    snprintf(rec_path, sizeof(rec_path), "%s/rec", base_path);
    
    // Use FATFS mkdir function
    FRESULT fr = f_mkdir(rec_path);
    if (fr != FR_OK && fr != FR_EXIST) {
        ESP_LOGW(TAG, "Failed to create /rec directory: %d", fr);
        // Continue anyway - directory might already exist
    }
    
    ESP_LOGI(TAG, "SD card mounted successfully at %s", base_path);
    if (s_card) {
        ESP_LOGI(TAG, "SD card size: %llu MB", 
                 (uint64_t)(s_card->csd.capacity * s_card->csd.sector_size) / (1024 * 1024));
    }
    
    return ESP_OK;
}

esp_err_t spiffs_storage_deinit(void) {
    if (s_mounted && s_card) {
        esp_vfs_fat_sdcard_unmount("/sdcard", s_card);
        s_card = NULL;
        s_mounted = false;
        ESP_LOGI(TAG, "SD card unmounted");
    }
    return ESP_OK;
}

bool spiffs_storage_is_mounted(void) {
    return s_mounted;
}

