# Source Tree and Module Organization

## Firmware Integration Approach

This shard represents the **Module Organization Layer** - how source code is structured and how modules integrate within the firmware build system.

### Modular Component Structure

```c
// softwareV2/main/include/module_system.h
#ifndef MODULE_SYSTEM_H
#define MODULE_SYSTEM_H

typedef enum {
    MODULE_STATUS_UNKNOWN = 0,
    MODULE_STATUS_FOUNDATION,  // Architecture defined, not implemented
    MODULE_STATUS_STUB,        // Placeholder implementation
    MODULE_STATUS_WORKING,     // Fully functional
    MODULE_STATUS_UNUSED       // Present but disabled
} module_status_t;

typedef struct {
    const char* module_name;
    const char* file_path;
    module_status_t status;
    bool build_enabled;
    const char** required_by;
    const char** dependencies;
} module_info_t;

#endif
```

### Build System Integration

```cmake
# softwareV2/CMakeLists.txt - Modular component enablement
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Module configuration - align with source tree status
option(ENABLE_MODULE_AUDIO_CAPTURE "Enable audio capture module" OFF)
option(ENABLE_MODULE_WIFI "Enable WiFi connectivity" OFF)
option(ENABLE_MODULE_BLE "Enable BLE communication" OFF)
option(ENABLE_MODULE_WEB_SERVER "Enable debug web server" OFF)

# Core modules (always enabled)
set(CORE_MODULES
    "main.c"           # Entry point and main loop
    "ui.c"            # Button/LED hardware interface (WORKING)
    "sd_storage.c"    # SD card management (WORKING)
)

# Foundation modules (architecture ready)
set(FOUNDATION_MODULES
    "recorder.c"      # Recording state machine (FOUNDATION)
    "wav_writer.c"    # WAV file generation (FOUNDATION)
)

# Conditional modules based on development phase
if(ENABLE_MODULE_AUDIO_CAPTURE)
    list(APPEND FOUNDATION_MODULES "audio_capture.c")
endif()

if(ENABLE_MODULE_WIFI)
    list(APPEND FOUNDATION_MODULES "wifi_manager.c")
endif()

# Fallback modules (when primary unavailable)
set(FALLBACK_MODULES
    "spiffs_storage.c"  # Internal flash when SD unavailable
)

project(salestag)
```

## Project Structure (Actual)

```text
salestag/
├── docs/                     # PRD and requirements documentation
│   ├── prd.md               # Comprehensive product requirements (Future vision)
│   ├── architecture/        # Sharded architecture documents
│   └── *.md                 # Various design documents
├── softwareV2/              # ESP32-S3 firmware (CURRENT IMPLEMENTATION)
│   ├── main/                # Core firmware modules
│   │   ├── include/         # Module header files
│   │   │   ├── module_system.h   # Module management (NEW)
│   │   │   ├── architecture.h    # System architecture (NEW)
│   │   │   └── entry_points.h    # Development navigation (NEW)
│   │   ├── main.c           # Simple button/LED test (ACTIVE BUILD)
│   │   ├── recorder.c       # Recording state machine (FOUNDATION)
│   │   ├── audio_capture.c  # Audio callback framework (STUB IMPLEMENTATION)
│   │   ├── sd_storage.c     # SD card management (WORKING)
│   │   ├── ui.c             # Button/LED hardware interface (WORKING)
│   │   ├── wav_writer.c     # WAV file generation (FOUNDATION)
│   │   ├── wifi_manager.c   # WiFi connectivity (PRESENT BUT UNUSED)
│   │   ├── web_server.c     # Debug web interface (PRESENT BUT UNUSED)
│   │   └── spiffs_storage.c # Internal flash storage (FALLBACK)
│   ├── CMakeLists.txt       # Build configuration
│   ├── sdkconfig            # ESP-IDF configuration (Auto-generated)
│   ├── partitions.csv       # Flash memory layout
│   └── README.md            # Diagnostic build documentation
├── device_hardware_info/    # PCB design and component datasheets
├── requirements.txt         # Python dependencies (minimal)
└── .bmad-core/              # Development workflow tools
```

## Key Modules and Their Current State

```c
// softwareV2/main/module_registry.c - Runtime module tracking
const module_info_t MODULE_REGISTRY[] = {
    // Working modules (highest priority for SD card + button functionality)
    {"ui", "main/ui.c", MODULE_STATUS_WORKING, true, 
     (const char*[]){"main", "recorder", NULL}, 
     (const char*[]){"gpio_driver", NULL}},
     
    {"sd_storage", "main/sd_storage.c", MODULE_STATUS_WORKING, true,
     (const char*[]){"recorder", "wav_writer", NULL},
     (const char*[]){"spi_driver", NULL}},
     
    // Foundation modules (architecture ready)
    {"recorder", "main/recorder.c", MODULE_STATUS_FOUNDATION, true,
     (const char*[]){"main", NULL},
     (const char*[]){"ui", "sd_storage", "audio_capture", NULL}},
     
    {"wav_writer", "main/wav_writer.c", MODULE_STATUS_FOUNDATION, true,
     (const char*[]){"recorder", NULL},
     (const char*[]){"sd_storage", NULL}},
     
    // Stub implementations (needs development)
    {"audio_capture", "main/audio_capture.c", MODULE_STATUS_STUB, false,
     (const char*[]){"recorder", NULL},
     (const char*[]){"adc_driver", NULL}},
     
    // Unused modules (future features)
    {"wifi_manager", "main/wifi_manager.c", MODULE_STATUS_UNUSED, false,
     (const char*[]){"web_server", NULL},
     (const char*[]){NULL}},
     
    {"web_server", "main/web_server.c", MODULE_STATUS_UNUSED, false,
     (const char*[]){NULL},
     (const char*[]){"wifi_manager", NULL}},
     
    // Fallback modules
    {"spiffs_storage", "main/spiffs_storage.c", MODULE_STATUS_WORKING, false,
     (const char*[]){"recorder", NULL},
     (const char*[]){NULL}},
};
```

### Module Integration Priority

**Phase 1 (Current SD Card + Button Focus)**:
1. `ui.c` (WORKING) - Button press detection for file saving
2. `sd_storage.c` (WORKING) - File system for test file storage
3. `main.c` (SIMPLE) - Basic test loop integration

**Phase 2 (Recording Pipeline)**:
4. `recorder.c` (FOUNDATION) - State machine for recording workflow  
5. `wav_writer.c` (FOUNDATION) - File format generation
6. `audio_capture.c` (STUB→WORKING) - Real microphone implementation

**Phase 3 (Advanced Features)**:
7. `wifi_manager.c` (UNUSED→FOUNDATION) - Network connectivity
8. `web_server.c` (UNUSED→FOUNDATION) - Remote debugging
9. BLE communication module (NEW)

### Module Development Pattern

```c
// Example module implementation pattern
// softwareV2/main/template_module.c

#include "module_system.h"

static const char* TAG = "template_module";
static bool module_initialized = false;

esp_err_t template_module_init(void) {
    if (module_initialized) return ESP_OK;
    
    ESP_LOGI(TAG, "Initializing template module");
    // Module-specific initialization
    
    module_initialized = true;
    return ESP_OK;
}

esp_err_t template_module_configure(void) {
    if (!module_initialized) return ESP_ERR_INVALID_STATE;
    
    ESP_LOGI(TAG, "Configuring template module");
    // Module-specific configuration
    
    return ESP_OK;
}

esp_err_t template_module_start(void) {
    if (!module_initialized) return ESP_ERR_INVALID_STATE;
    
    ESP_LOGI(TAG, "Starting template module");
    // Start module operation
    
    return ESP_OK;
}
```
