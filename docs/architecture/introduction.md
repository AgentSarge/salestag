# Introduction

This document captures the **CURRENT STATE** of the SalesTag firmware codebase as of August 2025. SalesTag is a door-to-door sales conversation recording system built on ESP32-S3 hardware with dual microphone audio capture and SD card storage. This brownfield analysis documents what EXISTS today, including the simplified diagnostic build currently implemented.

## Document Scope

**Focused on current firmware implementation** - The codebase contains a working ESP32-S3 firmware foundation with basic audio recording, button control, and SD card storage. The PRD outlines a complete IoT ecosystem with mobile app and BLE communication, but the current implementation is a simplified diagnostic version focusing on core hardware functionality.

## Firmware Integration Strategy

This shard represents the **Foundation Layer** in the firmware architecture. Integration approach:

### Core Initialization Sequence
```c
// main.c integration point
void app_main(void) {
    // 1. System initialization
    ESP_LOGI(TAG, "=== SalesTag Firmware v2.0 - Starting ===");
    
    // 2. Hardware abstraction layer init
    hal_init();
    
    // 3. Component initialization based on this architecture
    component_manager_init();
    
    // 4. Start application state machine
    application_start();
}
```

### Component Registration Pattern
```c
// Each architecture shard becomes a firmware component
typedef struct {
    const char* name;
    esp_err_t (*init)(void);
    esp_err_t (*start)(void);
    esp_err_t (*stop)(void);
} component_t;

// Register components from each architecture shard
static const component_t components[] = {
    {"quick_reference", quick_ref_init, quick_ref_start, quick_ref_stop},
    {"high_level_arch", arch_init, arch_start, arch_stop},
    {"source_tree", source_tree_init, source_tree_start, source_tree_stop},
    // ... each shard becomes a component
};
```

### Development Priority
- **Phase 1**: Foundation (this shard) + Core Hardware (GPIO, SD card)
- **Phase 2**: Audio capture + Recording state machine  
- **Phase 3**: BLE communication + Mobile integration
- **Phase 4**: Advanced features + Professional deployment

## Change Log

| Date       | Version | Description                           | Author    |
| ---------- | ------- | ------------------------------------- | --------- |
| 2025-08-21 | 1.0     | Initial brownfield analysis of ESP32-S3 firmware | Winston (Architect) |
| 2025-08-22 | 1.1     | Added firmware integration strategy for component-based development | Winston (Architect) |
