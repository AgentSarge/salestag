#include "sd_storage.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

static const char *TAG = "sd_storage";

// Global state
static sdmmc_card_t *s_card = NULL;
static bool s_mounted = false;
static sd_status_t s_status = SD_STATUS_UNMOUNTED;
static uint64_t s_total_bytes = 0;
static uint64_t s_free_bytes = 0;

// Internal function declarations
static esp_err_t sd_spi_init(void);
static esp_err_t sd_spi_deinit(void);
static esp_err_t sd_mount_fatfs(void);
static esp_err_t sd_unmount_fatfs(void);

esp_err_t sd_storage_init(void) {
    ESP_LOGI(TAG, "Initializing SD card storage");
    
    // Initialize SPI bus for SD card
    esp_err_t ret = sd_spi_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        s_status = SD_STATUS_ERROR;
        return ret;
    }
    
    // Mount FATFS on SD card
    ret = sd_mount_fatfs();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
        s_status = SD_STATUS_ERROR;
        // Don't return error - we'll try fallback
    } else {
        s_status = SD_STATUS_MOUNTED;
        s_mounted = true;
        
        // Get card information
        if (s_card) {
            s_total_bytes = (uint64_t)s_card->csd.capacity * s_card->csd.sector_size;
            ESP_LOGI(TAG, "SD card mounted: %llu bytes total", s_total_bytes);
        }
        
        // Create recording directory
        ret = sd_storage_create_rec_dir();
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to create recording directory: %s", esp_err_to_name(ret));
        }
    }
    
    return ESP_OK; // Always return OK to allow fallback
}

esp_err_t sd_storage_deinit(void) {
    ESP_LOGI(TAG, "Deinitializing SD card storage");
    
    if (s_mounted) {
        sd_unmount_fatfs();
        s_mounted = false;
    }
    
    sd_spi_deinit();
    s_status = SD_STATUS_UNMOUNTED;
    s_total_bytes = 0;
    s_free_bytes = 0;
    
    return ESP_OK;
}

esp_err_t sd_storage_get_info(sd_info_t *info) {
    if (!info) {
        return ESP_ERR_INVALID_ARG;
    }
    
    info->status = s_status;
    info->is_mounted = s_mounted;
    info->total_bytes = s_total_bytes;
    info->free_bytes = s_free_bytes;
    
    return ESP_OK;
}

bool sd_storage_is_available(void) {
    return (s_status == SD_STATUS_MOUNTED) && s_mounted;
}

esp_err_t sd_storage_create_rec_dir(void) {
    if (!s_mounted) {
        ESP_LOGW(TAG, "Cannot create directory - SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Creating recording directory: %s", SD_REC_DIR);
    
    // Create directory with mkdir
    int ret = mkdir(SD_REC_DIR, 0755);
    if (ret == 0) {
        ESP_LOGI(TAG, "Recording directory created successfully");
        return ESP_OK;
    } else if (errno == EEXIST) {
        ESP_LOGI(TAG, "Recording directory already exists");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Failed to create recording directory: %s", strerror(errno));
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

const char* sd_storage_get_rec_path(void) {
    return SD_REC_DIR;
}

esp_err_t sd_storage_fallback_to_internal(void) {
    ESP_LOGW(TAG, "Falling back to internal storage - SD card unavailable");
    // This would integrate with internal flash storage
    // For now, just log the fallback
    return ESP_OK;
}

// Private functions

static esp_err_t sd_spi_init(void) {
    ESP_LOGI(TAG, "Initializing SPI bus for SD card");
    
    // SPI bus configuration
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_MOSI_PIN,
        .miso_io_num = SD_MISO_PIN,
        .sclk_io_num = SD_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    
    // Initialize SPI bus
    esp_err_t ret = spi_bus_initialize(SD_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // SD card host configuration
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SD_SPI_HOST;
    
    // SD card slot configuration
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS_PIN;
    slot_config.host_id = SD_SPI_HOST;
    
    // Mount configuration
    esp_vfs_fat_mount_config_t mount_config = {
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
        .format_if_mount_failed = false,
        .disk_status_check_enable = false,
    };
    
    // Mount SD card using correct API
    ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &s_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "SD card SPI initialized successfully");
    return ESP_OK;
}

static esp_err_t sd_spi_deinit(void) {
    if (s_mounted) {
        sd_unmount_fatfs();
    }
    
    // Free SPI bus
    esp_err_t ret = spi_bus_free(SD_SPI_HOST);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to free SPI bus: %s", esp_err_to_name(ret));
    }
    
    return ESP_OK;
}

static esp_err_t sd_mount_fatfs(void) {
    // This is handled in sd_spi_init for SPI mode
    return ESP_OK;
}

static esp_err_t sd_unmount_fatfs(void) {
    if (s_mounted && s_card) {
        esp_err_t ret = esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, s_card);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to unmount SD card: %s", esp_err_to_name(ret));
        }
        s_card = NULL;
        s_mounted = false;
    }
    return ESP_OK;
}
