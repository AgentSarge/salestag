# Data Models and Current File Structure

## Firmware Integration Approach

This shard represents the **Data Management Layer** - how the firmware handles file systems, hardware configuration, and data structures for your SD card + button functionality.

### SD Card File System Integration

```c
// softwareV2/main/include/data_models.h
#ifndef DATA_MODELS_H
#define DATA_MODELS_H

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

// File system structure for SD card organization
typedef struct {
    const char* mount_point;     // "/sdcard"
    const char* recording_dir;   // "/sdcard/rec"
    const char* metadata_dir;    // "/sdcard/metadata"
    const char* config_dir;      // "/sdcard/config"
    uint32_t max_recordings;     // Directory file limit
    size_t max_file_size_mb;     // Per-recording size limit
} file_system_config_t;

// WAV file metadata structure
typedef struct {
    char filename[64];           // "recording_001.wav"
    uint32_t file_number;        // Sequential counter
    time_t timestamp;            // Unix timestamp
    uint32_t duration_ms;        // Recording length
    uint32_t sample_rate;        // 16000 Hz
    uint16_t bit_depth;          // 16 bits
    uint16_t channels;           // 2 (stereo)
    size_t file_size_bytes;      // Actual file size
    bool is_complete;            // Recording finished successfully
} recording_metadata_t;

#endif
```

### File System Management Implementation

```c
// softwareV2/main/data_models.c
static const file_system_config_t FS_CONFIG = {
    .mount_point = "/sdcard",
    .recording_dir = "/sdcard/rec",
    .metadata_dir = "/sdcard/metadata",
    .config_dir = "/sdcard/config",
    .max_recordings = 1000,
    .max_file_size_mb = 10
};

esp_err_t data_models_init(void) {
    ESP_LOGI(TAG, "Initializing data models and file system");
    
    // Create directory structure
    if (mkdir(FS_CONFIG.recording_dir, 0755) != 0 && errno != EEXIST) {
        ESP_LOGE(TAG, "Failed to create recording directory");
        return ESP_FAIL;
    }
    
    if (mkdir(FS_CONFIG.metadata_dir, 0755) != 0 && errno != EEXIST) {
        ESP_LOGE(TAG, "Failed to create metadata directory");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

esp_err_t get_next_recording_filename(char* filename, size_t filename_size) {
    uint32_t next_number = 1;
    
    // Find highest existing file number
    DIR* dir = opendir(FS_CONFIG.recording_dir);
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strstr(entry->d_name, "recording_") == entry->d_name) {
                uint32_t file_num = 0;
                if (sscanf(entry->d_name, "recording_%u.wav", &file_num) == 1) {
                    if (file_num >= next_number) {
                        next_number = file_num + 1;
                    }
                }
            }
        }
        closedir(dir);
    }
    
    snprintf(filename, filename_size, "%s/recording_%03u.wav", 
             FS_CONFIG.recording_dir, next_number);
    return ESP_OK;
}
```

## Audio File Storage (Current Implementation)

**SD Card Organization**:
```text
/sdcard/
├── rec/                     # Recording directory (WORKING)
│   ├── recording_001.wav    # WAV files with sequential naming
│   ├── recording_002.wav    # Button press creates new files
│   └── recording_XXX.wav    # Up to 999 recordings
├── metadata/                # Recording metadata (NEW)
│   ├── recording_001.json   # Per-file metadata
│   └── session_log.json     # Session tracking
└── config/                  # Device configuration (NEW)
    ├── device_settings.json # User preferences
    └── audio_calibration.dat # Microphone calibration
```

**WAV File Format** (Actual Implementation):
- Sample Rate: 16kHz
- Bit Depth: 16-bit
- Channels: 2 (stereo - dual microphone)
- Format: PCM uncompressed
- File naming: Sequential numbering (recording_XXX.wav)

### WAV File Header Structure

```c
// softwareV2/main/wav_writer.c integration
typedef struct {
    // RIFF header
    char riff_header[4];        // "RIFF"
    uint32_t wav_size;          // File size - 8
    char wave_header[4];        // "WAVE"
    
    // Format chunk
    char fmt_header[4];         // "fmt "
    uint32_t fmt_chunk_size;    // 16 for PCM
    uint16_t audio_format;      // 1 for PCM
    uint16_t num_channels;      // 2 for stereo
    uint32_t sample_rate;       // 16000
    uint32_t byte_rate;         // sample_rate * channels * (bits/8)
    uint16_t sample_alignment;  // channels * (bits/8)
    uint16_t bit_depth;         // 16
    
    // Data chunk
    char data_header[4];        // "data"
    uint32_t data_bytes;        // Number of bytes in data
} wav_header_t;

esp_err_t write_wav_header(FILE* file, uint32_t data_size) {
    wav_header_t header = {
        .riff_header = {'R', 'I', 'F', 'F'},
        .wav_size = data_size + sizeof(wav_header_t) - 8,
        .wave_header = {'W', 'A', 'V', 'E'},
        .fmt_header = {'f', 'm', 't', ' '},
        .fmt_chunk_size = 16,
        .audio_format = 1,
        .num_channels = 2,
        .sample_rate = 16000,
        .byte_rate = 16000 * 2 * 2,
        .sample_alignment = 4,
        .bit_depth = 16,
        .data_header = {'d', 'a', 't', 'a'},
        .data_bytes = data_size
    };
    
    return (fwrite(&header, sizeof(header), 1, file) == 1) ? ESP_OK : ESP_FAIL;
}
```

## Hardware Pin Configuration (Actual)

```c
// softwareV2/main/include/hardware_config.h
#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

// GPIO pin assignments (from sd_storage.h and main.c)
#define BUTTON_GPIO    4     // Button input with pullup
#define LED_GPIO       40    // Status LED output

// SD Card SPI interface (WORKING)
#define SD_CS_PIN      39    // SD card chip select
#define SD_MOSI_PIN    35    // SD card SPI MOSI
#define SD_MISO_PIN    37    // SD card SPI MISO  
#define SD_SCLK_PIN    36    // SD card SPI clock

// Audio input pins (FOUNDATION)
#define MIC1_ADC_CH    3     // GPIO 9 - Microphone 1 (ADC1_CH3)
#define MIC2_ADC_CH    6     // GPIO 12 - Microphone 2 (ADC1_CH6)

// Hardware configuration validation
typedef struct {
    gpio_num_t pin;
    gpio_mode_t mode;
    gpio_pull_mode_t pull;
    const char* description;
    bool is_tested;
} pin_config_t;

// Pin configuration registry
static const pin_config_t PIN_REGISTRY[] = {
    {BUTTON_GPIO, GPIO_MODE_INPUT, GPIO_PULLUP_ENABLE, "User button", true},
    {LED_GPIO, GPIO_MODE_OUTPUT, GPIO_FLOATING, "Status LED", true},
    {SD_CS_PIN, GPIO_MODE_OUTPUT, GPIO_FLOATING, "SD card CS", true},
    {SD_MOSI_PIN, GPIO_MODE_OUTPUT, GPIO_FLOATING, "SD card MOSI", true},
    {SD_MISO_PIN, GPIO_MODE_INPUT, GPIO_FLOATING, "SD card MISO", true},
    {SD_SCLK_PIN, GPIO_MODE_OUTPUT, GPIO_FLOATING, "SD card CLK", true},
    {GPIO_NUM_9, GPIO_MODE_DISABLE, GPIO_FLOATING, "Microphone 1", false},
    {GPIO_NUM_12, GPIO_MODE_DISABLE, GPIO_FLOATING, "Microphone 2", false},
};

#endif
```

### Hardware Initialization Integration

```c
// softwareV2/main/hardware_config.c
esp_err_t hardware_config_init(void) {
    ESP_LOGI(TAG, "Initializing hardware configuration");
    
    // Configure all GPIO pins according to registry
    for (int i = 0; i < sizeof(PIN_REGISTRY)/sizeof(pin_config_t); i++) {
        const pin_config_t* pin = &PIN_REGISTRY[i];
        
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << pin->pin),
            .mode = pin->mode,
            .pull_up_en = (pin->pull == GPIO_PULLUP_ENABLE),
            .pull_down_en = (pin->pull == GPIO_PULLDOWN_ENABLE),
            .intr_type = GPIO_INTR_DISABLE
        };
        
        esp_err_t ret = gpio_config(&io_conf);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure %s (GPIO %d): %s", 
                     pin->description, pin->pin, esp_err_to_name(ret));
            return ret;
        }
        
        ESP_LOGI(TAG, "Configured %s (GPIO %d) - Tested: %s", 
                 pin->description, pin->pin, pin->is_tested ? "YES" : "NO");
    }
    
    return ESP_OK;
}
```

## Button Press to File Creation Workflow

```c
// Integration with current SD card + button functionality
void button_press_handler(void* arg) {
    ESP_LOGI(TAG, "Button pressed - creating test file");
    
    // Get next filename
    char filename[128];
    if (get_next_recording_filename(filename, sizeof(filename)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to generate filename");
        return;
    }
    
    // Create test WAV file with current button press
    FILE* file = fopen(filename, "wb");
    if (file) {
        // Write minimal WAV header for test file
        write_wav_header(file, 0);  // Empty audio data for now
        fclose(file);
        
        ESP_LOGI(TAG, "Created test file: %s", filename);
        
        // Flash LED to indicate success
        gpio_set_level(LED_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(200));
        gpio_set_level(LED_GPIO, 0);
    } else {
        ESP_LOGE(TAG, "Failed to create file: %s", filename);
    }
}
