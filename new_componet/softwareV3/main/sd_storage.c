#include "sd_storage.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_timer.h"
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

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
    
    // SPI bus configuration - optimized for SDXC compatibility
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
    
    ESP_LOGI(TAG, "SPI bus initialized successfully");
    
    // REMOVED: Hardware reset attempt - use simple approach like minimal test
    
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
    ESP_LOGI(TAG, "Mounting SD card with write access...");
    
    // SD card host configuration - EXACT same as working minimal test
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SD_SPI_HOST;
    host.max_freq_khz = 1000;  // Same speed as working minimal test
    
    // SD card slot configuration
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS_PIN;
    slot_config.host_id = SD_SPI_HOST;
    slot_config.gpio_cd = -1;  // No card detect pin
    slot_config.gpio_wp = -1;  // No write protect pin - explicitly disable write protection
    
    // Mount configuration - EXACT same as working minimal test
    esp_vfs_fat_mount_config_t mount_config = {
        .max_files = 5,
        .allocation_unit_size = 512,  // EXACT same as working minimal test
        .format_if_mount_failed = false,
        .disk_status_check_enable = false,
    };
    
    // Single attempt: Mount with explicit allocation unit size - EXACTLY as before
    esp_err_t ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &s_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "SD card mounted successfully");
    
    // Simple write test (same as working minimal test)
    ESP_LOGI(TAG, "Testing write access after mount...");
    FILE *test_file = fopen("/sdcard/a.txt", "wb");
    if (test_file) {
        size_t written = fwrite("hello from sd_storage\n", 1, 22, test_file);
        fclose(test_file);
        if (written > 0) {
            ESP_LOGI(TAG, "✅ Write access confirmed - removing test file");
            unlink("/sdcard/a.txt");
            return ESP_OK;
        }
    }
    
    ESP_LOGE(TAG, "Write test failed - SD card mount has issues");
    return ESP_FAIL;
    
    // Test 1: Try to create a simple file in the root (THIS CODE IS NOW SKIPPED)
    const char* test_paths[] = {
        "/sdcard/mount_test.txt",
        "/sdcard/test.txt", 
        "/sdcard/rec/test.txt"
    };
    
    bool write_success = false;
    for (int i = 0; i < 3; i++) {
        ESP_LOGI(TAG, "Testing write to: %s", test_paths[i]);
        
        // First check if directory exists and get its permissions
        struct stat st;
        if (stat("/sdcard", &st) == 0) {
            ESP_LOGI(TAG, "Mount point /sdcard exists, permissions: %lu", st.st_mode & 0777);
            
            // Check if directory is writable
            if (access("/sdcard", W_OK) == 0) {
                ESP_LOGI(TAG, "Mount point /sdcard is writable");
            } else {
                ESP_LOGE(TAG, "Mount point /sdcard is NOT writable (errno: %d)", errno);
                // Try a different approach - check if it's a filesystem issue
                ESP_LOGI(TAG, "Attempting alternative write test...");
                continue;
            }
        } else {
            ESP_LOGE(TAG, "Mount point /sdcard does not exist (errno: %d)", errno);
            continue;
        }
        
        // Try to create the test file
        FILE *test_file = fopen(test_paths[i], "w");
        if (test_file) {
            int write_result = fprintf(test_file, "Mount test successful at %s\n", test_paths[i]);
            if (write_result > 0) {
                fclose(test_file);
                ESP_LOGI(TAG, "Write access confirmed at %s - removing test file", test_paths[i]);
                unlink(test_paths[i]);
                write_success = true;
                break;
            } else {
                ESP_LOGE(TAG, "fprintf failed at %s (errno: %d)", test_paths[i], errno);
                fclose(test_file);
            }
        } else {
            ESP_LOGE(TAG, "Failed to open %s for writing (errno: %d)", test_paths[i], errno);
        }
    }
    
    if (write_success) {
        return ESP_OK;
    }
    
    // If all write tests failed, just return error (no complex remount logic)
    ESP_LOGE(TAG, "All write tests failed - SD card has issues");
    return ESP_FAIL;
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

esp_err_t sd_storage_power_cycle(void) {
    ESP_LOGI(TAG, "=== SD Card Power Cycle ===");
    
    // Step 1: Unmount if currently mounted
    if (s_mounted) {
        ESP_LOGI(TAG, "Unmounting SD card...");
        sd_unmount_fatfs();
        s_mounted = false;
        s_status = SD_STATUS_UNMOUNTED;
    }
    
    // Step 2: Free SPI bus
    ESP_LOGI(TAG, "Freeing SPI bus...");
    esp_err_t ret = spi_bus_free(SD_SPI_HOST);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to free SPI bus: %s", esp_err_to_name(ret));
    }
    
    // Step 3: Wait for SD card to reset
    ESP_LOGI(TAG, "Waiting for SD card to reset...");
    vTaskDelay(pdMS_TO_TICKS(1000)); // Wait 1 second for SD card to reset
    
    // Step 4: Reinitialize SPI bus
    ESP_LOGI(TAG, "Reinitializing SPI bus...");
    ret = sd_spi_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reinitialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Step 5: Remount SD card
    ESP_LOGI(TAG, "Remounting SD card...");
    ret = sd_mount_fatfs();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to remount SD card: %s", esp_err_to_name(ret));
        s_status = SD_STATUS_ERROR;
        return ret;
    }
    
    s_status = SD_STATUS_MOUNTED;
    s_mounted = true;
    
    // Step 6: Test write access
    ESP_LOGI(TAG, "Testing write access after power cycle...");
    ret = sd_storage_test_write_access();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Write access test failed after power cycle: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "=== SD Card Power Cycle Complete ===");
    return ESP_OK;
}

esp_err_t sd_storage_test_write_access(void) {
    ESP_LOGI(TAG, "Testing SD card write access...");
    
    // Try multiple test files with retry logic
    const char* test_files[] = {
        "/sdcard/power_cycle_test1.txt",
        "/sdcard/power_cycle_test2.txt",
        "/sdcard/power_cycle_test3.txt"
    };
    
    bool write_success = false;
    int successful_tests = 0;
    
    for (int i = 0; i < 3; i++) {
        ESP_LOGI(TAG, "Testing write to: %s", test_files[i]);
        
        // Remove any existing file
        unlink(test_files[i]);
        
        // Try to create and write to test file
        FILE *test_file = fopen(test_files[i], "w");
        if (test_file) {
            int write_result = fprintf(test_file, "Power cycle test %d successful at %lu\n", 
                                     i + 1, (unsigned long)esp_timer_get_time() / 1000);
            if (write_result > 0) {
                fclose(test_file);
                ESP_LOGI(TAG, "Write test %d successful", i + 1);
                successful_tests++;
                write_success = true;
                
                // Clean up test file
                unlink(test_files[i]);
            } else {
                fclose(test_file);
                ESP_LOGE(TAG, "Write test %d failed - fprintf returned %d", i + 1, write_result);
            }
        } else {
            ESP_LOGE(TAG, "Write test %d failed - fopen failed (errno: %d)", i + 1, errno);
        }
        
        // Small delay between tests
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    if (write_success) {
        ESP_LOGI(TAG, "SD card write access confirmed (%d/%d tests passed)", successful_tests, 3);
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "SD card write access failed (0/%d tests passed)", 3);
        return ESP_FAIL;
    }
}
