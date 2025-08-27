# Quick Reference - Key Files and Entry Points

## Critical Files for Understanding the System

- **Main Entry**: `softwareV2/main/main.c` - Simple button/LED test implementation
- **Configuration**: `softwareV2/CMakeLists.txt`, `softwareV2/sdkconfig`
- **Core Audio Logic**: `softwareV2/main/recorder.c`, `softwareV2/main/audio_capture.c`
- **Storage Management**: `softwareV2/main/sd_storage.c`, `softwareV2/main/wav_writer.c`
- **Hardware Interface**: `softwareV2/main/ui.c` (button/LED control)
- **Project Documentation**: `softwareV2/README.md`, `docs/prd.md`

## Firmware Integration Approach

This shard represents the **Navigation Layer** - how developers quickly find and understand code entry points.

### File Organization Pattern

```c
// softwareV2/main/include/entry_points.h
#ifndef ENTRY_POINTS_H
#define ENTRY_POINTS_H

// Central registry of all firmware modules
typedef struct {
    const char* module_name;
    const char* file_path;
    const char* primary_function;
    bool is_implemented;
    bool is_tested;
} module_entry_t;

// Quick reference lookup table
extern const module_entry_t MODULE_REGISTRY[];
extern const size_t MODULE_COUNT;

#endif
```

### Development Workflow Integration

```c
// softwareV2/main/entry_points.c
const module_entry_t MODULE_REGISTRY[] = {
    {"main", "main/main.c", "app_main", true, true},
    {"recorder", "main/recorder.c", "recorder_init", true, false},
    {"audio_capture", "main/audio_capture.c", "audio_init", false, false},
    {"sd_storage", "main/sd_storage.c", "sd_storage_init", true, true},
    {"ui", "main/ui.c", "ui_init", true, true},
    {"wav_writer", "main/wav_writer.c", "wav_writer_init", true, false},
};

const size_t MODULE_COUNT = sizeof(MODULE_REGISTRY) / sizeof(module_entry_t);
```

### Build System Integration

```cmake
# CMakeLists.txt - Component registration
set(COMPONENT_SRCS 
    "main.c"
    "entry_points.c"
    "recorder.c"       # State machine foundation
    "audio_capture.c"  # Stub implementation
    "sd_storage.c"     # Working implementation  
    "ui.c"            # Working implementation
    "wav_writer.c"    # Foundation implementation
)

# Enable/disable components based on development phase
if(CONFIG_ENABLE_AUDIO_RECORDING)
    list(APPEND COMPONENT_SRCS "audio_processor.c")
endif()

if(CONFIG_ENABLE_BLE_STACK)
    list(APPEND COMPONENT_SRCS "ble_manager.c")
endif()
```

## Current vs. Planned Architecture Gap

The PRD describes an integrated ecosystem with BLE mobile app communication, but the **current implementation** is a simplified diagnostic build focusing on:

- Basic button press detection (GPIO 4)
- LED status indication (GPIO 40) 
- SD card storage preparation
- Foundation audio recording modules

**Missing from current implementation**:
- BLE communication stack
- Mobile companion app
- WiFi connectivity (intentionally removed in diagnostic build)
- Advanced power management
- Secure pairing and encryption

## Developer Onboarding Sequence

1. **Start Here**: `main.c` - Understand current simple test loop
2. **Hardware Layer**: `ui.c` + `sd_storage.c` - Working components
3. **Audio Foundation**: `recorder.c` + `wav_writer.c` - Architecture ready
4. **Future Integration**: `audio_capture.c` - Needs implementation
5. **Configuration**: `CMakeLists.txt` + `sdkconfig` - Build customization
