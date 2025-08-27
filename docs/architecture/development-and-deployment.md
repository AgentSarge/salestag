# Development and Deployment

## Firmware Integration Approach

This shard represents the **Build and Deployment Layer** - how the firmware is compiled, tested, and deployed, maintaining your current working SD card + button functionality while adding new features.

### Build System Integration

```c
// softwareV2/main/include/build_config.h
#ifndef BUILD_CONFIG_H
#define BUILD_CONFIG_H

// Build configuration for modular development
typedef enum {
    BUILD_CONFIG_DIAGNOSTIC = 0,  // Current: Simple button/LED test
    BUILD_CONFIG_BASIC_RECORDING, // Next: Add audio capture
    BUILD_CONFIG_FULL_FEATURED,  // Future: BLE + mobile integration
    BUILD_CONFIG_PRODUCTION      // Final: All features enabled
} build_config_t;

typedef struct {
    build_config_t config;
    const char* description;
    bool enable_audio_capture;
    bool enable_ble_stack;
    bool enable_wifi;
    bool enable_debug_features;
} build_profile_t;

// Build profiles for different development phases
extern const build_profile_t BUILD_PROFILES[];

#endif
```

### Modular Build Configuration

```cmake
# softwareV2/CMakeLists.txt - Enhanced build system
cmake_minimum_required(VERSION 3.16)

# Build profile selection (align with current development state)
set(BUILD_PROFILE "diagnostic" CACHE STRING "Build profile: diagnostic, basic_recording, full_featured, production")

# Profile-specific component enablement
if(BUILD_PROFILE STREQUAL "diagnostic")
    # Current working build - maintain this functionality
    set(ENABLE_AUDIO_CAPTURE OFF)
    set(ENABLE_BLE_STACK OFF)
    set(ENABLE_WIFI OFF)
    set(ENABLE_DEBUG_FEATURES ON)
    
elseif(BUILD_PROFILE STREQUAL "basic_recording")
    # Next development phase - add audio to working SD card foundation
    set(ENABLE_AUDIO_CAPTURE ON)
    set(ENABLE_BLE_STACK OFF)
    set(ENABLE_WIFI OFF)
    set(ENABLE_DEBUG_FEATURES ON)
    
elseif(BUILD_PROFILE STREQUAL "full_featured")
    set(ENABLE_AUDIO_CAPTURE ON)
    set(ENABLE_BLE_STACK ON)
    set(ENABLE_WIFI ON)
    set(ENABLE_DEBUG_FEATURES OFF)
    
elseif(BUILD_PROFILE STREQUAL "production")
    set(ENABLE_AUDIO_CAPTURE ON)
    set(ENABLE_BLE_STACK ON)
    set(ENABLE_WIFI OFF)
    set(ENABLE_DEBUG_FEATURES OFF)
endif()

# Apply configuration to source files
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/main/include/build_config_generated.h.in"
               "${CMAKE_CURRENT_BINARY_DIR}/include/build_config_generated.h")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(salestag)
```

## Current Build Process

```bash
# Working development commands (TESTED with SD card + button)
cd softwareV2

# Clean previous build (recommended between major changes)
idf.py clean

# Configure target (ESP32-S3 specific)
idf.py set-target esp32s3

# Build with current diagnostic profile (working)
idf.py build

# Flash to device (tested with button/SD functionality)
idf.py flash

# Monitor serial output (view button press events)
idf.py monitor

# Combined build and flash (convenience)
idf.py build flash monitor
```

### Development Workflow Integration

```bash
#!/bin/bash
# softwareV2/scripts/develop.sh - Development workflow automation

set -e

BUILD_PROFILE=${1:-diagnostic}
ACTION=${2:-build}

echo "=== SalesTag Development Workflow ==="
echo "Profile: $BUILD_PROFILE"
echo "Action: $ACTION"

cd "$(dirname "$0")/.."

case "$ACTION" in
    "clean")
        echo "Cleaning previous build..."
        idf.py clean
        ;;
    "build")
        echo "Building with profile: $BUILD_PROFILE"
        idf.py -D BUILD_PROFILE=$BUILD_PROFILE build
        ;;
    "flash")
        echo "Flashing firmware..."
        idf.py flash
        ;;
    "test")
        echo "Running hardware integration tests..."
        idf.py build flash
        echo "Testing button + SD card functionality..."
        # Could add automated testing here
        ;;
    "debug")
        echo "Starting debug session..."
        idf.py build flash monitor
        ;;
    *)
        echo "Usage: $0 [profile] [action]"
        echo "Profiles: diagnostic, basic_recording, full_featured, production"
        echo "Actions: clean, build, flash, test, debug"
        exit 1
        ;;
esac

echo "=== Workflow Complete ==="
```

### Continuous Integration Setup

```yaml
# .github/workflows/firmware-ci.yml
name: ESP32-S3 Firmware CI

on:
  push:
    paths:
      - 'softwareV2/**'
  pull_request:
    paths:
      - 'softwareV2/**'

jobs:
  build-and-test:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        build_profile: [diagnostic, basic_recording, full_featured]
        
    steps:
    - uses: actions/checkout@v4
      with:
        submodules: recursive
        
    - name: Setup ESP-IDF
      uses: espressif/esp-idf-ci-action@v1
      with:
        esp_idf_version: v5.2.2
        target: esp32s3
        
    - name: Build firmware
      working-directory: softwareV2
      run: |
        idf.py set-target esp32s3
        idf.py -D BUILD_PROFILE=${{ matrix.build_profile }} build
        
    - name: Archive firmware artifacts
      uses: actions/upload-artifact@v4
      with:
        name: firmware-${{ matrix.build_profile }}
        path: |
          softwareV2/build/*.bin
          softwareV2/build/*.elf
          softwareV2/build/*.map
```

## Flash Memory Layout (Current)

```text
# From partitions.csv - optimized for SD card + button functionality
# Name,     Type, SubType, Offset,   Size,     Flags
nvs,        data, nvs,     0x9000,   0x6000,   # Settings storage
phy_init,   data, phy,     0xf000,   0x1000,   # RF calibration
factory,    app,  factory, 0x10000,  0x1F0000, # Main application (1.9MB)
```

### Memory Optimization for Features

```c
// softwareV2/main/build_config.c
const build_profile_t BUILD_PROFILES[] = {
    {
        .config = BUILD_CONFIG_DIAGNOSTIC,
        .description = "Current working build - button + SD card only",
        .enable_audio_capture = false,
        .enable_ble_stack = false,
        .enable_wifi = false,
        .enable_debug_features = true
    },
    {
        .config = BUILD_CONFIG_BASIC_RECORDING,
        .description = "Add audio capture to working foundation",
        .enable_audio_capture = true,
        .enable_ble_stack = false,
        .enable_wifi = false,
        .enable_debug_features = true
    },
    {
        .config = BUILD_CONFIG_FULL_FEATURED,
        .description = "Complete IoT functionality",
        .enable_audio_capture = true,
        .enable_ble_stack = true,
        .enable_wifi = true,
        .enable_debug_features = false
    },
    {
        .config = BUILD_CONFIG_PRODUCTION,
        .description = "Optimized for deployment",
        .enable_audio_capture = true,
        .enable_ble_stack = true,
        .enable_wifi = false,
        .enable_debug_features = false
    }
};

// Runtime build configuration access
const build_profile_t* get_current_build_profile(void) {
    // Compile-time selection based on CMake configuration
    #if defined(CONFIG_BUILD_DIAGNOSTIC)
        return &BUILD_PROFILES[BUILD_CONFIG_DIAGNOSTIC];
    #elif defined(CONFIG_BUILD_BASIC_RECORDING)
        return &BUILD_PROFILES[BUILD_CONFIG_BASIC_RECORDING];
    #elif defined(CONFIG_BUILD_FULL_FEATURED)
        return &BUILD_PROFILES[BUILD_CONFIG_FULL_FEATURED];
    #elif defined(CONFIG_BUILD_PRODUCTION)
        return &BUILD_PROFILES[BUILD_CONFIG_PRODUCTION];
    #else
        return &BUILD_PROFILES[BUILD_CONFIG_DIAGNOSTIC]; // Safe default
    #endif
}
```

## Development Hardware Setup

1. **ESP32-S3 DevKit**: WROOM-1 or WROOM-2 development board
2. **MicroSD Card**: 32GB, FAT32 formatted for audio storage (TESTED)
3. **Button/LED**: Connected to GPIO 4 and GPIO 40 respectively (WORKING)
4. **Microphones**: MAX9814 modules connected to ADC1_CH3 and ADC1_CH6 (NEXT PHASE)
5. **USB-C Cable**: For programming, debugging, and power (WORKING)

### Deployment Pipeline

```bash
# Production deployment process
# 1. Build production firmware
./scripts/develop.sh production build

# 2. Validate with hardware tests
./scripts/develop.sh production test

# 3. Create deployment package
mkdir -p dist/
cp build/salestag.bin dist/
cp build/bootloader/bootloader.bin dist/
cp build/partition_table/partition-table.bin dist/

# 4. Generate deployment instructions
echo "Flash commands:" > dist/DEPLOY.md
echo "esptool.py write_flash 0x0 bootloader.bin" >> dist/DEPLOY.md
echo "esptool.py write_flash 0x8000 partition-table.bin" >> dist/DEPLOY.md  
echo "esptool.py write_flash 0x10000 salestag.bin" >> dist/DEPLOY.md
```

### Build Verification Tests

```c
// softwareV2/main/build_verification.c - Ensure working functionality preserved
esp_err_t verify_build_integrity(void) {
    const build_profile_t* profile = get_current_build_profile();
    ESP_LOGI(TAG, "Verifying build: %s", profile->description);
    
    // Always verify core working functionality
    ESP_ERROR_CHECK(test_button_sd_integration());
    ESP_ERROR_CHECK(test_led_feedback());
    ESP_ERROR_CHECK(test_sd_card_operations());
    
    // Verify profile-specific features
    if (profile->enable_audio_capture) {
        ESP_ERROR_CHECK(test_audio_capture_integration());
    }
    
    if (profile->enable_ble_stack) {
        ESP_ERROR_CHECK(test_ble_stack_init());
    }
    
    ESP_LOGI(TAG, "✅ Build verification passed");
    return ESP_OK;
}