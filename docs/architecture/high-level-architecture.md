# High Level Architecture

## Technical Summary

**Current State**: SalesTag firmware is implemented as a simplified ESP32-S3 diagnostic build focusing on core hardware validation. The codebase provides a foundation for the full IoT system described in the PRD but currently operates as a standalone recording device.

## Firmware Architecture Integration

This shard represents the **System Architecture Layer** - defining how components interact and the overall firmware structure.

### Layered Architecture Pattern

```c
// softwareV2/main/include/architecture.h
#ifndef ARCHITECTURE_H
#define ARCHITECTURE_H

// Architecture layers (bottom to top)
typedef enum {
    LAYER_HAL = 0,        // Hardware Abstraction Layer
    LAYER_DRIVERS,        // Device drivers (SD, GPIO, ADC)
    LAYER_SERVICES,       // Core services (recording, storage)
    LAYER_APPLICATION,    // Business logic
    LAYER_INTERFACE,      // User interface (LEDs, button)
    LAYER_COUNT
} firmware_layer_t;

// Component dependencies and initialization order
typedef struct {
    firmware_layer_t layer;
    const char* name;
    esp_err_t (*init)(void);
    esp_err_t (*configure)(void);
    esp_err_t (*start)(void);
    const char** dependencies;  // NULL-terminated list
} architecture_component_t;

#endif
```

### System Integration Pattern

```c
// softwareV2/main/architecture.c
static const architecture_component_t SYSTEM_COMPONENTS[] = {
    // HAL Layer
    {LAYER_HAL, "esp32_hal", esp32_hal_init, esp32_hal_config, esp32_hal_start, NULL},
    
    // Driver Layer  
    {LAYER_DRIVERS, "gpio_driver", gpio_init, gpio_config, gpio_start, (const char*[]){"esp32_hal", NULL}},
    {LAYER_DRIVERS, "spi_driver", spi_init, spi_config, spi_start, (const char*[]){"esp32_hal", NULL}},
    {LAYER_DRIVERS, "adc_driver", adc_init, adc_config, adc_start, (const char*[]){"esp32_hal", NULL}},
    
    // Service Layer
    {LAYER_SERVICES, "sd_storage", sd_storage_init, sd_storage_config, sd_storage_start, (const char*[]){"spi_driver", NULL}},
    {LAYER_SERVICES, "audio_service", audio_init, audio_config, audio_start, (const char*[]){"adc_driver", NULL}},
    {LAYER_SERVICES, "recorder", recorder_init, recorder_config, recorder_start, (const char*[]){"sd_storage", "audio_service", NULL}},
    
    // Application Layer
    {LAYER_APPLICATION, "state_machine", app_state_init, app_state_config, app_state_start, (const char*[]){"recorder", NULL}},
    
    // Interface Layer
    {LAYER_INTERFACE, "ui_manager", ui_init, ui_config, ui_start, (const char*[]){"gpio_driver", "state_machine", NULL}},
};

esp_err_t architecture_initialize(void) {
    // Initialize components in layer order with dependency checking
    for (firmware_layer_t layer = 0; layer < LAYER_COUNT; layer++) {
        for (int i = 0; i < COMPONENT_COUNT; i++) {
            if (SYSTEM_COMPONENTS[i].layer == layer) {
                ESP_ERROR_CHECK(SYSTEM_COMPONENTS[i].init());
                ESP_ERROR_CHECK(SYSTEM_COMPONENTS[i].configure());
                ESP_ERROR_CHECK(SYSTEM_COMPONENTS[i].start());
            }
        }
    }
    return ESP_OK;
}
```

## Actual Tech Stack (Current Implementation)

| Category           | Technology       | Version | Notes                              |
| ------------------ | ---------------- | ------- | ---------------------------------- |
| Hardware Platform | ESP32-S3 Mini    | -       | Dual-core with audio processing    |
| Firmware Framework | ESP-IDF          | 5.2.2   | FreeRTOS-based real-time OS        |
| Build System       | CMake            | 3.16+   | ESP-IDF standard build system      |
| Audio Processing   | ADC + Custom     | -       | Dual microphone via ADC channels   |
| Storage            | microSD (SPI)    | -       | FAT32 filesystem, 10MHz SPI        |
| File Format        | WAV              | -       | 16kHz/16-bit stereo PCM            |
| Hardware Interface | GPIO Direct      | -       | Button input, LED output           |

### Task Assignment to CPU Cores

```c
// CPU core assignment for ESP32-S3 dual-core architecture
typedef enum {
    CORE_PROTOCOL = 0,  // Core 0: Protocol stack (PRO_CPU)
    CORE_APP = 1        // Core 1: Application logic (APP_CPU)
} cpu_core_t;

// Task assignments for optimal performance
static const struct {
    const char* task_name;
    cpu_core_t assigned_core;
    UBaseType_t priority;
    size_t stack_size;
} TASK_ASSIGNMENTS[] = {
    {"audio_capture", CORE_APP, 10, 4096},      // High priority, dedicated core
    {"sd_writer", CORE_APP, 8, 3072},           // High priority for file I/O
    {"button_handler", CORE_PROTOCOL, 5, 2048}, // UI responsiveness
    {"led_manager", CORE_PROTOCOL, 3, 1024},    // Low priority status updates
    {"state_machine", CORE_PROTOCOL, 6, 3072},  // Core application logic
};
```

## Repository Structure Reality Check

- **Type**: Monorepo with firmware, documentation, and hardware design files
- **Build System**: ESP-IDF CMake with standard ESP32-S3 configuration
- **Notable**: Contains both simplified diagnostic build AND foundation for full system (WiFi/BLE modules present but unused)

## Architecture Evolution Plan

### Phase 1 (Current): Basic Hardware Control
- Simple main loop with button/LED testing
- SD card storage foundation
- GPIO hardware interfaces working

### Phase 2: Service Layer Integration  
- Implement full architecture pattern shown above
- Add audio capture and recording services
- Integrate state machine for recording workflow

### Phase 3: Advanced Features
- Add BLE communication service layer
- Implement power management service
- Add security and encryption services

### Phase 4: Professional Features
- OTA update service
- Device management and configuration
- Advanced audio processing pipeline
