# Appendix - Essential Commands and Troubleshooting

## Firmware Integration Approach

This shard represents the **Debugging and Operations Layer** - practical commands and troubleshooting procedures to maintain your working SD card + button functionality and debug issues during development.

### Debug Command Integration

```c
// softwareV2/main/include/debug_commands.h
#ifndef DEBUG_COMMANDS_H
#define DEBUG_COMMANDS_H

typedef enum {
    DEBUG_LEVEL_NONE = 0,
    DEBUG_LEVEL_ERROR,
    DEBUG_LEVEL_WARN,
    DEBUG_LEVEL_INFO,
    DEBUG_LEVEL_DEBUG,
    DEBUG_LEVEL_VERBOSE
} debug_level_t;

typedef struct {
    const char* command;
    const char* description;
    esp_err_t (*handler)(int argc, char** argv);
    bool requires_hardware;
} debug_command_t;

// Runtime debugging system
extern const debug_command_t DEBUG_COMMANDS[];
extern const size_t DEBUG_COMMANDS_COUNT;

#endif
```

### Firmware Debug Commands

```c
// softwareV2/main/debug_commands.c - Built-in debugging
esp_err_t debug_cmd_test_button_sd(int argc, char** argv) {
    ESP_LOGI(TAG, "Testing button → SD card integration...");
    ESP_ERROR_CHECK(test_button_sd_integration());
    ESP_LOGI(TAG, "✅ Button → SD test completed");
    return ESP_OK;
}

esp_err_t debug_cmd_list_sd_files(int argc, char** argv) {
    ESP_LOGI(TAG, "Listing SD card files...");
    
    DIR* dir = opendir("/sdcard/rec");
    if (dir == NULL) {
        ESP_LOGE(TAG, "Failed to open recording directory");
        return ESP_FAIL;
    }
    
    struct dirent* entry;
    int file_count = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".wav") != NULL) {
            ESP_LOGI(TAG, "📁 %s", entry->d_name);
            file_count++;
        }
    }
    closedir(dir);
    
    ESP_LOGI(TAG, "Total WAV files: %d", file_count);
    return ESP_OK;
}

esp_err_t debug_cmd_hardware_status(int argc, char** argv) {
    ESP_LOGI(TAG, "Hardware Status Report:");
    ESP_LOGI(TAG, "  Button (GPIO %d): %s", BUTTON_GPIO, 
             gpio_get_level(BUTTON_GPIO) ? "Released" : "Pressed");
    ESP_LOGI(TAG, "  LED (GPIO %d): %s", LED_GPIO,
             gpio_get_level(LED_GPIO) ? "ON" : "OFF");
    
    // SD card status
    struct stat st;
    if (stat("/sdcard", &st) == 0) {
        ESP_LOGI(TAG, "  SD Card: ✅ Mounted");
    } else {
        ESP_LOGE(TAG, "  SD Card: ❌ Not accessible");
    }
    
    // Memory status
    ESP_LOGI(TAG, "  Free heap: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "  Min free heap: %d bytes", esp_get_minimum_free_heap_size());
    
    return ESP_OK;
}

const debug_command_t DEBUG_COMMANDS[] = {
    {"test_button_sd", "Test button → SD card file creation", debug_cmd_test_button_sd, true},
    {"list_files", "List WAV files on SD card", debug_cmd_list_sd_files, true},
    {"hw_status", "Show hardware component status", debug_cmd_hardware_status, true},
    {"reset_system", "Soft reset the system", debug_cmd_reset_system, false},
};

const size_t DEBUG_COMMANDS_COUNT = sizeof(DEBUG_COMMANDS) / sizeof(debug_command_t);
```

## Development Commands

### Essential Build Commands (TESTED)

```bash
# Core development workflow (proven with SD card + button)
cd softwareV2

# Clean build (recommended between major changes)
idf.py clean

# Configure target (ESP32-S3 specific)
idf.py set-target esp32s3

# Build with current diagnostic profile
idf.py build

# Flash to device (tested with button/SD functionality)
idf.py flash

# Monitor serial output (view button press events and file creation)
idf.py monitor

# Combined workflow (build + flash + monitor)
idf.py build flash monitor

# Exit monitor: Ctrl+] (Linux/Mac) or Ctrl+T, Ctrl+X (Windows)
```

### Enhanced Development Commands

```bash
# Development script usage (from enhanced architecture)
./scripts/develop.sh diagnostic build    # Current working build
./scripts/develop.sh diagnostic flash    # Flash current build
./scripts/develop.sh diagnostic test     # Test button + SD functionality
./scripts/develop.sh diagnostic debug    # Interactive debugging session

# Hardware validation commands
idf.py build flash && echo "Testing button press..." && sleep 5
idf.py monitor | grep "Button\|SD\|LED"  # Filter relevant log output

# SD card file verification
ls -la /path/to/sd/card/rec/              # Check files on mounted SD card
```

### Debug Information Collection

```bash
# System information gathering
echo "=== ESP32-S3 System Info ===" > debug_report.txt
idf.py --version >> debug_report.txt
git rev-parse HEAD >> debug_report.txt
date >> debug_report.txt

# Build information
echo "=== Build Information ===" >> debug_report.txt
idf.py size >> debug_report.txt

# Flash and monitor with logging
idf.py build flash monitor 2>&1 | tee firmware_debug.log
```

## Hardware Debugging

### GPIO Troubleshooting

```c
// Hardware debugging functions built into firmware
void debug_gpio_state(void) {
    ESP_LOGI(TAG, "=== GPIO Debug Information ===");
    ESP_LOGI(TAG, "Button GPIO %d: Level=%d, Mode=%s", 
             BUTTON_GPIO, gpio_get_level(BUTTON_GPIO),
             "INPUT_PULLUP");
    ESP_LOGI(TAG, "LED GPIO %d: Level=%d, Mode=%s",
             LED_GPIO, gpio_get_level(LED_GPIO), 
             "OUTPUT");
             
    // Test GPIO responsiveness
    ESP_LOGI(TAG, "Testing LED toggle...");
    for (int i = 0; i < 5; i++) {
        gpio_set_level(LED_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(LED_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    ESP_LOGI(TAG, "LED test complete");
}
```

**Button Issues**: 
```bash
# Check button wiring and pull-up resistor
idf.py monitor
# Look for: "Button configured - GPIO[4] with pullup"
# Test: Press button, should see "Button pressed" message
```

**LED Problems**: 
```bash
# Verify LED connection and current limiting resistor
# In monitor, LED should flash after button press
# Check GPIO 40 connection and power
```

**SD Card Failures**: 
```bash
# Verify SPI wiring and 3.3V power
# Check FAT32 format: 
diskutil list                           # macOS
sudo fdisk -l                          # Linux  
# Format if needed:
sudo mkfs.fat -F32 /dev/sdX1           # Linux
diskutil eraseDisk FAT32 SDCARD /dev/diskN  # macOS
```

### SPI Bus Debugging

```c
// SPI debugging for SD card troubleshooting
void debug_spi_sd_card(void) {
    ESP_LOGI(TAG, "=== SD Card SPI Debug ===");
    ESP_LOGI(TAG, "CS Pin %d: %d", SD_CS_PIN, gpio_get_level(SD_CS_PIN));
    ESP_LOGI(TAG, "MOSI Pin %d: %d", SD_MOSI_PIN, gpio_get_level(SD_MOSI_PIN));
    ESP_LOGI(TAG, "MISO Pin %d: %d", SD_MISO_PIN, gpio_get_level(SD_MISO_PIN));
    ESP_LOGI(TAG, "SCLK Pin %d: %d", SD_SCLK_PIN, gpio_get_level(SD_SCLK_PIN));
    
    // Attempt SD card reinitialization
    esp_err_t result = sd_storage_reinitialize();
    ESP_LOGI(TAG, "SD reinit result: %s", esp_err_to_name(result));
}
```

## Expected Serial Output (Working System)

### Normal Boot Sequence

```text
=== SalesTag Firmware v2.0 - Starting ===
I (145) boot: ESP-IDF v5.2.2
I (289) main_task: Calling app_main()
I (295) salestag: === SalesTag Simple Test - Button + LED Only ===
I (302) salestag: BOOT: Starting simple button test...
I (308) gpio: GPIO[4] configured as input with pullup
I (314) gpio: GPIO[40] configured as output
I (319) salestag: GPIO configured - Button: GPIO[4], LED: GPIO[40]
I (326) salestag: Button initial level: 1
I (330) sd_storage: Mounting SD card...
I (442) sd_storage: SD card mounted successfully
I (447) sd_storage: Creating directory: /sdcard/rec
I (453) salestag: === System Ready ===
I (457) salestag: Press button to turn LED ON, release to turn OFF
```

### Button Press and File Creation

```text
I (15234) salestag: Button pressed - creating test file
I (15241) data_models: Next filename: /sdcard/rec/recording_001.wav
I (15289) wav_writer: WAV header written successfully
I (15295) salestag: Created test file: /sdcard/rec/recording_001.wav
I (15301) salestag: LED feedback complete
```

### Error Conditions and Recovery

```text
# SD Card Mount Failure
E (456) sd_storage: Failed to mount SD card (ESP_ERR_NOT_FOUND)
W (462) salestag: SD card unavailable, using internal storage fallback
I (469) spiffs_storage: SPIFFS mounted successfully

# Button Hardware Issue  
W (12345) ui: Button not responding, enabling automatic test mode
I (12352) salestag: Automatic test recording every 30 seconds
```

## Troubleshooting Workflow

### Systematic Problem Resolution

```c
// Automated troubleshooting workflow
esp_err_t run_troubleshooting_sequence(void) {
    ESP_LOGI(TAG, "Starting systematic troubleshooting...");
    
    // Level 1: Basic system health
    ESP_LOGI(TAG, "Level 1: System Health Check");
    ESP_ERROR_CHECK(verify_build_integrity());
    ESP_ERROR_CHECK(validate_hardware_connections());
    
    // Level 2: Component-specific tests
    ESP_LOGI(TAG, "Level 2: Component Testing");
    ESP_ERROR_CHECK(test_gpio_functionality());
    ESP_ERROR_CHECK(test_spi_communication());
    ESP_ERROR_CHECK(test_filesystem_operations());
    
    // Level 3: Integration workflow
    ESP_LOGI(TAG, "Level 3: Integration Workflow");
    ESP_ERROR_CHECK(test_button_to_file_creation());
    ESP_ERROR_CHECK(test_led_feedback_timing());
    
    ESP_LOGI(TAG, "✅ Troubleshooting sequence completed");
    return ESP_OK;
}
```

### Quick Diagnostic Commands

```bash
# Quick health check (30 seconds)
echo "Quick diagnostic starting..." && \
idf.py monitor | timeout 30s grep -E "(Button|SD|LED|ERROR|WARNING)" && \
echo "Diagnostic complete - check output above"

# SD card file count check
ls /path/to/sd/card/rec/*.wav 2>/dev/null | wc -l

# Memory usage check during operation
idf.py monitor | grep -E "(heap|memory|alloc)"
```

### Common Issue Resolution

**Issue**: Button press not detected
```bash
# Solution 1: Check hardware
# Verify GPIO 4 connection and pullup resistor (10kΩ)

# Solution 2: Test in firmware
# Look for "Button configured" message in monitor output
# If missing, check gpio_config() calls
```

**Issue**: SD card files not created
```bash
# Solution 1: Check SD card format
diskutil info /dev/diskN | grep "File System"  # Should show "FAT32"

# Solution 2: Check permissions and space
df -h /path/to/sd/card                         # Check available space
ls -la /path/to/sd/card/rec/                  # Check directory access
```

**Issue**: LED not responding
```bash
# Solution 1: Check hardware
# Verify GPIO 40 connection and current limiting resistor (220Ω)

# Solution 2: Manual test
# In monitor, should see "LED feedback complete" after button press
```

### Advanced Debugging

```c
// Performance monitoring during operation
void monitor_performance(void) {
    static uint32_t last_heap = 0;
    uint32_t current_heap = esp_get_free_heap_size();
    
    if (last_heap != 0) {
        int32_t heap_change = current_heap - last_heap;
        ESP_LOGI(TAG, "Heap: %d bytes (change: %+d)", current_heap, heap_change);
    }
    
    last_heap = current_heap;
    
    // Task stack usage
    UBaseType_t stack_watermark = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGI(TAG, "Stack high water mark: %d words", stack_watermark);
}