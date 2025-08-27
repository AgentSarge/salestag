# Integration Points and External Dependencies

## Firmware Integration Approach

This shard represents the **Dependency Management Layer** - how hardware components and software libraries integrate with the firmware, building from your working SD card + button foundation.

### Hardware Abstraction Layer

```c
// softwareV2/main/include/integration_points.h
#ifndef INTEGRATION_POINTS_H
#define INTEGRATION_POINTS_H

// Hardware component status tracking
typedef enum {
    HW_STATUS_NOT_PRESENT = 0,
    HW_STATUS_PRESENT_NOT_TESTED,
    HW_STATUS_STUBBED,
    HW_STATUS_WORKING,
    HW_STATUS_FAILED
} hardware_status_t;

typedef struct {
    const char* component_name;
    const char* interface_type;
    hardware_status_t status;
    esp_err_t (*init_func)(void);
    esp_err_t (*test_func)(void);
    const char* notes;
} hardware_component_t;

// Software dependency management
typedef struct {
    const char* library_name;
    const char* version;
    bool is_required;
    bool is_available;
    esp_err_t (*init_func)(void);
} software_dependency_t;

#endif
```

### Component Integration Registry

```c
// softwareV2/main/integration_points.c
static const hardware_component_t HARDWARE_REGISTRY[] = {
    // Working components (highest priority - build upon these)
    {
        .component_name = "ESP32-S3 Mini",
        .interface_type = "Core MCU",
        .status = HW_STATUS_WORKING,
        .init_func = esp32_init,
        .test_func = esp32_self_test,
        .notes = "Dual-core, 240MHz, built-in BLE"
    },
    {
        .component_name = "MicroSD Card",
        .interface_type = "SPI", 
        .status = HW_STATUS_WORKING,
        .init_func = sd_storage_init,
        .test_func = sd_storage_test,
        .notes = "10MHz SPI, FAT32 formatted - TESTED with button file creation"
    },
    {
        .component_name = "Button/LED",
        .interface_type = "GPIO",
        .status = HW_STATUS_WORKING,
        .init_func = ui_init,
        .test_func = ui_test,
        .notes = "GPIO 4 input, GPIO 40 output - TESTED in current build"
    },
    
    // Components needing development
    {
        .component_name = "MAX9814 Mics",
        .interface_type = "ADC",
        .status = HW_STATUS_STUBBED,
        .init_func = audio_capture_init,
        .test_func = audio_capture_test,
        .notes = "ADC channels configured, no real capture - NEXT PRIORITY"
    },
    {
        .component_name = "USB-C Charging",
        .interface_type = "USB",
        .status = HW_STATUS_PRESENT_NOT_TESTED,
        .init_func = usb_charging_init,
        .test_func = usb_charging_test,
        .notes = "Hardware design complete - needs testing"
    }
};

static const software_dependency_t SOFTWARE_REGISTRY[] = {
    // Core dependencies (working)
    {"ESP-IDF", "5.2.2", true, true, esp_idf_init},
    {"FreeRTOS", "10.4.3", true, true, freertos_init},
    {"FAT filesystem", "ESP-IDF", true, true, fatfs_init},
    {"SPI Driver", "ESP-IDF", true, true, spi_init},
    {"GPIO Driver", "ESP-IDF", true, true, gpio_init},
    
    // Development dependencies (stubbed/missing)
    {"ADC Driver", "ESP-IDF", true, false, adc_continuous_init},
    {"BLE Stack", "ESP-IDF", false, false, ble_init},
    {"WiFi Stack", "ESP-IDF", false, false, wifi_init},
    
    // Future dependencies
    {"Encryption", "mbedTLS", false, false, crypto_init},
    {"OTA Updates", "ESP-IDF", false, false, ota_init}
};
```

## Hardware Dependencies (Current)

| Component      | Interface | Status         | Notes                           |
| -------------- | --------- | -------------- | ------------------------------- |
| ESP32-S3 Mini  | Core MCU  | ✅ Working     | Dual-core, 240MHz, built-in BLE |
| MicroSD Card   | SPI       | ✅ Working     | 10MHz SPI, FAT32 formatted     |
| MAX9814 Mics   | ADC       | ⚠️ Stubbed     | ADC channels configured, no real capture |
| Button/LED     | GPIO      | ✅ Working     | GPIO 4 input, GPIO 40 output   |
| USB-C Charging | USB       | 🔄 Not tested  | Hardware design complete        |

## Software Dependencies (Current Build)

- **ESP-IDF 5.2.2**: Core framework for ESP32-S3 development
- **FreeRTOS**: Real-time task management (included with ESP-IDF)
- **FAT filesystem**: SD card file operations
- **ADC Driver**: Microphone input (configured but not capturing)
- **SPI Driver**: SD card communication
- **GPIO Driver**: Button/LED hardware interface

## Missing Integrations (Per PRD)

- **BLE Stack**: ESP32-S3 Bluetooth Low Energy peripheral mode
- **React Native App**: Mobile companion for audio playback and device management
- **Supabase Backend**: Cloud storage and team collaboration features
- **OTA Updates**: Over-the-air firmware update capability
- **Encryption**: AES-256 for audio files and BLE communication
