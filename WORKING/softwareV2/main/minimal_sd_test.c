#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "driver/spi_master.h"
#include "esp_timer.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include "esp_vfs_fat.h"
#include "ff.h"

static const char *TAG = "minimal_sd_test";

// SD card configuration - same as your current setup
#define SD_MOUNT_POINT "/sdcard"
#define SD_SPI_HOST SPI2_HOST
#define SD_CS_PIN 39
#define SD_MOSI_PIN 35
#define SD_MISO_PIN 37
#define SD_SCLK_PIN 36

// Test configuration
#define TEST_SPI_SPEED_KHZ 1000  // Conservative 1MHz

static sdmmc_card_t *s_card = NULL;

static void probe_paths(void) {
    ESP_LOGI(TAG, "\n🔍 === PATH VALIDATION PROBE ===");
    
    const char *p1 = "/sdcard/a.txt";
    ESP_LOGI(TAG, "Probing path: %s", p1);

    // FatFs direct - tells us the exact cause
    ESP_LOGI(TAG, "\n📁 FatFs Direct API Test:");
    FIL f;
    FRESULT fr = f_open(&f, "0:/a.txt", FA_CREATE_ALWAYS | FA_WRITE);
    ESP_LOGI(TAG, "  f_open(\"0:/a.txt\") -> FRESULT %d", fr);
    
    // Decode FRESULT for clarity
    const char* fresult_names[] = {
        "FR_OK", "FR_DISK_ERR", "FR_INT_ERR", "FR_NOT_READY", "FR_NO_FILE",
        "FR_NO_PATH", "FR_INVALID_NAME", "FR_DENIED", "FR_EXIST", "FR_INVALID_OBJECT",
        "FR_WRITE_PROTECTED", "FR_INVALID_DRIVE", "FR_NOT_ENABLED", "FR_NO_FILESYSTEM",
        "FR_MKFS_ABORTED", "FR_TIMEOUT", "FR_LOCKED", "FR_NOT_ENOUGH_CORE", "FR_TOO_MANY_OPEN_FILES"
    };
    
    if (fr < sizeof(fresult_names)/sizeof(fresult_names[0])) {
        ESP_LOGI(TAG, "  FRESULT meaning: %s", fresult_names[fr]);
    }
    
    if (fr == FR_OK) {
        ESP_LOGI(TAG, "  ✅ FatFs file opened successfully!");
        
        UINT bw;
        const char *msg = "hello from FatFs\n";
        fr = f_write(&f, msg, strlen(msg), &bw);
        ESP_LOGI(TAG, "  f_write -> FRESULT %d, wrote %u bytes", fr, (unsigned)bw);
        
        if (fr == FR_OK && bw > 0) {
            ESP_LOGI(TAG, "  ✅ FatFs write successful!");
        } else {
            ESP_LOGE(TAG, "  ❌ FatFs write failed");
        }
        
        f_close(&f);
    } else {
        ESP_LOGE(TAG, "  ❌ FatFs file open failed!");
    }

    // POSIX via VFS
    ESP_LOGI(TAG, "\n📄 POSIX VFS API Test:");
    ESP_LOGI(TAG, "  Exact path string: \"%s\" (length: %d)", p1, (int)strlen(p1));
    
    FILE *fp = fopen(p1, "wb");
    if (!fp) {
        ESP_LOGE(TAG, "  ❌ fopen(%s, \"wb\") failed, errno=%d (%s)", p1, errno, strerror(errno));
        
        // Try different modes
        const char* modes[] = {"w", "a", "w+"};
        for (int i = 0; i < 3; i++) {
            fp = fopen(p1, modes[i]);
            if (fp) {
                ESP_LOGI(TAG, "  ✅ fopen with mode \"%s\" works!", modes[i]);
                fclose(fp);
                break;
            } else {
                ESP_LOGE(TAG, "  ❌ fopen mode \"%s\" failed: errno=%d", modes[i], errno);
            }
        }
    } else {
        ESP_LOGI(TAG, "  ✅ POSIX fopen successful!");
        
        const char *msg = "hello from POSIX\n";
        size_t n = fwrite(msg, 1, strlen(msg), fp);
        ESP_LOGI(TAG, "  fwrite -> %u bytes written", (unsigned)n);
        
        if (n > 0) {
            ESP_LOGI(TAG, "  ✅ POSIX write successful!");
        } else {
            ESP_LOGE(TAG, "  ❌ POSIX write failed");
        }
        
        fclose(fp);
    }

    // Try a tiny dir
    ESP_LOGI(TAG, "\n📂 Directory Creation Test:");
    const char* dir_path = "/sdcard/t";
    ESP_LOGI(TAG, "  Creating directory: %s", dir_path);
    
    int r = mkdir(dir_path, 0777);
    ESP_LOGI(TAG, "  mkdir(%s, 0777) -> %d, errno=%d (%s)", dir_path, r, errno, 
             r == 0 ? "SUCCESS" : strerror(errno));
    
    if (r == 0) {
        ESP_LOGI(TAG, "  ✅ Directory creation successful!");
        
        // Try creating a file in the directory
        const char* subfile = "/sdcard/t/b.txt";
        fp = fopen(subfile, "wb");
        if (fp) {
            ESP_LOGI(TAG, "  ✅ File in subdirectory works!");
            fclose(fp);
        } else {
            ESP_LOGE(TAG, "  ❌ File in subdirectory failed: errno=%d", errno);
        }
    } else if (errno == EEXIST) {
        ESP_LOGI(TAG, "  ℹ️  Directory already exists");
    }
    
    ESP_LOGI(TAG, "=== END PATH PROBE ===\n");
}



void test_spi_speed(int speed_khz, const char* speed_name) {
    ESP_LOGI(TAG, "\n=== Testing SPI Speed: %s ===", speed_name);
    
    // Clean up any previous mount
    if (s_card) {
        esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, s_card);
        s_card = NULL;
    }
    
    // Free SPI bus
    spi_bus_free(SD_SPI_HOST);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Initialize SPI bus
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_MOSI_PIN,
        .miso_io_num = SD_MISO_PIN,
        .sclk_io_num = SD_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    
    esp_err_t ret = spi_bus_initialize(SD_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "  ❌ SPI bus init failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "  ✅ SPI bus initialized");
    
    // SD card host configuration
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SD_SPI_HOST;
    host.max_freq_khz = speed_khz;
    
    // SD card slot configuration
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS_PIN;
    slot_config.host_id = SD_SPI_HOST;
    slot_config.gpio_cd = -1;  // No card detect
    slot_config.gpio_wp = -1;  // No write protect
    
    // Mount configuration
    esp_vfs_fat_mount_config_t mount_config = {
        .max_files = 5,
        .allocation_unit_size = 512,
        .format_if_mount_failed = false,  // Don't format - we want to preserve data
        .disk_status_check_enable = false,
    };
    
    // Try to mount
    ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &s_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "  ❌ Mount failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "  ✅ SD card mounted successfully");
    
    // Get card info
    if (s_card) {
        uint64_t card_size = (uint64_t)s_card->csd.capacity * s_card->csd.sector_size;
        ESP_LOGI(TAG, "  📊 Card size: %llu bytes (%.1f GB)", 
                 card_size, (double)card_size / (1024*1024*1024));
        ESP_LOGI(TAG, "  📊 Card name: %s", s_card->cid.name);
    }
    
    // Test basic path and filename validation
    ESP_LOGI(TAG, "  🔍 Testing path validation...");
    probe_paths();
    
    ESP_LOGI(TAG, "  📋 Path validation test complete for %s", speed_name);
    
    ESP_LOGI(TAG, "=== Speed Test Complete ===\n");
    vTaskDelay(pdMS_TO_TICKS(1000)); // Wait before next test
}

void test_power_and_timing() {
    ESP_LOGI(TAG, "\n=== Testing Power and Timing Issues ===");
    
    // Test with different delays and power cycles
    for (int cycle = 0; cycle < 3; cycle++) {
        ESP_LOGI(TAG, "Power cycle test %d/3", cycle + 1);
        
        // Clean shutdown
        if (s_card) {
            esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, s_card);
            s_card = NULL;
        }
        spi_bus_free(SD_SPI_HOST);
        
        // Wait for power cycle
        ESP_LOGI(TAG, "  Waiting for power stabilization...");
        vTaskDelay(pdMS_TO_TICKS(2000)); // 2 second power cycle
        
        // Reinitialize with slow, stable settings
        spi_bus_config_t bus_cfg = {
            .mosi_io_num = SD_MOSI_PIN,
            .miso_io_num = SD_MISO_PIN,
            .sclk_io_num = SD_SCLK_PIN,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 512, // Smaller transfers
        };
        
        esp_err_t ret = spi_bus_initialize(SD_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "  ❌ Power cycle %d: SPI init failed", cycle + 1);
            continue;
        }
        
        // Very conservative mount settings
        sdmmc_host_t host = SDSPI_HOST_DEFAULT();
        host.slot = SD_SPI_HOST;
        host.max_freq_khz = 400; // Very slow
        
        sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
        slot_config.gpio_cs = SD_CS_PIN;
        slot_config.host_id = SD_SPI_HOST;
        slot_config.gpio_cd = -1;
        slot_config.gpio_wp = -1;
        
        esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 3,
            .allocation_unit_size = 4096,  // FIXED: Match CONFIG_FATFS_SECTOR_4096
            .format_if_mount_failed = false,
            .disk_status_check_enable = false,
        };
        
        ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &s_card);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "  ❌ Power cycle %d: Mount failed - %s", cycle + 1, esp_err_to_name(ret));
            continue;
        }
        
        ESP_LOGI(TAG, "  ✅ Power cycle %d: Mount successful", cycle + 1);
        
        // Test immediate write
        char filename[64];
        snprintf(filename, sizeof(filename), "/sdcard/power_cycle_test_%d.txt", cycle + 1);
        FILE* f = fopen(filename, "w");
        if (f) {
            fprintf(f, "Power cycle test %d successful\n", cycle + 1);
            fclose(f);
            ESP_LOGI(TAG, "  ✅ Power cycle %d: Write test passed", cycle + 1);
        } else {
            ESP_LOGE(TAG, "  ❌ Power cycle %d: Write test failed (errno: %d)", cycle + 1, errno);
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void test_filesystem_operations() {
    ESP_LOGI(TAG, "\n=== Testing Filesystem Operations ===");
    
    // Test various filesystem operations that might reveal issues
    
    // 1. Check mount point permissions
    struct stat st;
    if (stat(SD_MOUNT_POINT, &st) == 0) {
        ESP_LOGI(TAG, "✅ Mount point exists, permissions: 0%o", (unsigned int)(st.st_mode & 0777));
    } else {
        ESP_LOGE(TAG, "❌ Mount point stat failed: %s", strerror(errno));
    }
    
    // 2. Test write permissions
    if (access(SD_MOUNT_POINT, W_OK) == 0) {
        ESP_LOGI(TAG, "✅ Mount point has write permissions");
    } else {
        ESP_LOGE(TAG, "❌ Mount point lacks write permissions: %s", strerror(errno));
    }
    
    // 3. Test different file sizes
    const size_t test_sizes[] = {10, 100, 1024, 4096, 10240};
    const int num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);
    
    for (int i = 0; i < num_sizes; i++) {
        char filename[64];
        snprintf(filename, sizeof(filename), "/sdcard/size_test_%zu.txt", test_sizes[i]);
        
        FILE* f = fopen(filename, "w");
        if (f) {
            // Write data of specific size
            for (size_t j = 0; j < test_sizes[i]; j++) {
                fputc('A' + (j % 26), f);
            }
            fclose(f);
            ESP_LOGI(TAG, "✅ Size test passed: %zu bytes", test_sizes[i]);
        } else {
            ESP_LOGE(TAG, "❌ Size test failed: %zu bytes (errno: %d)", test_sizes[i], errno);
        }
    }
    
    // 4. Test flush operations
    ESP_LOGI(TAG, "Testing filesystem flush...");
    FILE* flush_test = fopen("/sdcard/flush_test.txt", "w");
    if (flush_test) {
        fprintf(flush_test, "Testing flush operations\n");
        fflush(flush_test); // Force write buffer flush
        fclose(flush_test);
        ESP_LOGI(TAG, "✅ Flush test passed");
    } else {
        ESP_LOGE(TAG, "❌ Flush test failed (errno: %d)", errno);
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "\n");
    ESP_LOGI(TAG, "╔════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║        MINIMAL SD CARD TEST            ║");
    ESP_LOGI(TAG, "║     Isolating mount/write issues       ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    
    ESP_LOGI(TAG, "Hardware Configuration:");
    ESP_LOGI(TAG, "  CS: GPIO[%d]", SD_CS_PIN);
    ESP_LOGI(TAG, "  MOSI: GPIO[%d]", SD_MOSI_PIN);
    ESP_LOGI(TAG, "  MISO: GPIO[%d]", SD_MISO_PIN);
    ESP_LOGI(TAG, "  SCLK: GPIO[%d]", SD_SCLK_PIN);
    ESP_LOGI(TAG, "  SPI Host: SPI2_HOST");
    ESP_LOGI(TAG, "");
    
    // Test with conservative SPI speed and analyze filesystem
    ESP_LOGI(TAG, "Starting path/filename validation at 1MHz (conservative speed)...");
    test_spi_speed(1000, "1MHz (Conservative)");
    
    ESP_LOGI(TAG, "\n");
    ESP_LOGI(TAG, "╔════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║        DIAGNOSIS COMPLETE              ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    
    // Show final recommendations for filename/path issues
    ESP_LOGI(TAG, "💡 NEXT STEPS TO FIX ERRNO 22 (EINVAL):");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "1. 📝 Check FatFs configuration:");
    ESP_LOGI(TAG, "   • Run 'idf.py menuconfig'");
    ESP_LOGI(TAG, "   • Go to Component config → FAT Filesystem support");
    ESP_LOGI(TAG, "   • Enable 'Long filename support'");
    ESP_LOGI(TAG, "   • Set 'Max LFN' to 255");
    ESP_LOGI(TAG, "   • Keep 'LFN working buffer' on heap");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "2. 🔧 Try basic paths first:");
    ESP_LOGI(TAG, "   • Use '/sdcard/a.txt' instead of complex names");
    ESP_LOGI(TAG, "   • Use 'wb' mode instead of complex modes");
    ESP_LOGI(TAG, "   • Avoid subdirectories until basic files work");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "3. 🧪 If still failing, reformat SD card:");
    ESP_LOGI(TAG, "   • Use f_mkfs() to create ESP32-compatible filesystem");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "💡 Check the probe results above for specific error details!");
    
    // Keep running and show periodic status with simple path tests
    int counter = 0;
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(15000)); // 15 second interval
        counter++;
        
        ESP_LOGI(TAG, "\n⏰ Status check #%d - Testing simple file operations...", counter);
        
        // Test simple file with basic name
        FILE* status_test = fopen("/sdcard/a.txt", "wb");
        if (status_test) {
            size_t written = fwrite("OK\n", 1, 3, status_test);
            fclose(status_test);
            if (written > 0) {
                ESP_LOGI(TAG, "  ✅ Simple file write OK (errno would be fixed!)");
            } else {
                ESP_LOGE(TAG, "  ❌ Write failed but fopen worked (unusual)");
            }
        } else {
            ESP_LOGE(TAG, "  ❌ Still failing: fopen('/sdcard/a.txt', 'wb') errno=%d (%s)", 
                     errno, strerror(errno));
            ESP_LOGI(TAG, "  💡 Recommendation: Check FatFs long filename settings in menuconfig");
        }
        
        // Quick FatFs test
        FIL f;
        FRESULT fr = f_open(&f, "0:/b.txt", FA_CREATE_ALWAYS | FA_WRITE);
        if (fr == FR_OK) {
            f_close(&f);
            ESP_LOGI(TAG, "  ✅ FatFs direct API works - issue is in VFS layer");
        } else {
            ESP_LOGE(TAG, "  ❌ FatFs also failing: FRESULT=%d", fr);
        }
    }
}
