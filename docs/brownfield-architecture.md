# SalesTag Brownfield Architecture Document

## Introduction

This document captures the **CURRENT STATE** of the SalesTag firmware codebase as of August 2025. SalesTag is a door-to-door sales conversation recording system built on ESP32-S3 hardware with dual microphone audio capture and SD card storage. This brownfield analysis documents what EXISTS today, including the simplified diagnostic build currently implemented.

### Document Scope

**Focused on current firmware implementation** - The codebase contains a working ESP32-S3 firmware foundation with basic audio recording, button control, and SD card storage. The PRD outlines a complete IoT ecosystem with mobile app and BLE communication, but the current implementation is a simplified diagnostic version focusing on core hardware functionality.

### Change Log

| Date       | Version | Description                           | Author    |
| ---------- | ------- | ------------------------------------- | --------- |
| 2025-08-21 | 1.0     | Initial brownfield analysis of ESP32-S3 firmware | Winston (Architect) |

## Quick Reference - Key Files and Entry Points

### Critical Files for Understanding the System

- **Main Entry**: `softwareV2/main/main.c` - Simple button/LED test implementation
- **Configuration**: `softwareV2/CMakeLists.txt`, `softwareV2/sdkconfig`
- **Core Audio Logic**: `softwareV2/main/recorder.c`, `softwareV2/main/audio_capture.c`
- **Storage Management**: `softwareV2/main/sd_storage.c`, `softwareV2/main/wav_writer.c`
- **Hardware Interface**: `softwareV2/main/ui.c` (button/LED control)
- **Project Documentation**: `softwareV2/README.md`, `docs/prd.md`

### Current vs. Planned Architecture Gap

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

## High Level Architecture

### Technical Summary

**Current State**: SalesTag firmware is implemented as a simplified ESP32-S3 diagnostic build focusing on core hardware validation. The codebase provides a foundation for the full IoT system described in the PRD but currently operates as a standalone recording device.

### Actual Tech Stack (Current Implementation)

| Category           | Technology       | Version | Notes                              |
| ------------------ | ---------------- | ------- | ---------------------------------- |
| Hardware Platform | ESP32-S3 Mini    | -       | Dual-core with audio processing    |
| Firmware Framework | ESP-IDF          | 5.2.2   | FreeRTOS-based real-time OS        |
| Build System       | CMake            | 3.16+   | ESP-IDF standard build system      |
| Audio Processing   | ADC + Custom     | -       | Dual microphone via ADC channels   |
| Storage            | microSD (SPI)    | -       | FAT32 filesystem, 10MHz SPI        |
| File Format        | WAV              | -       | 16kHz/16-bit stereo PCM            |
| Hardware Interface | GPIO Direct      | -       | Button input, LED output           |

### Repository Structure Reality Check

- **Type**: Monorepo with firmware, documentation, and hardware design files
- **Build System**: ESP-IDF CMake with standard ESP32-S3 configuration
- **Notable**: Contains both simplified diagnostic build AND foundation for full system (WiFi/BLE modules present but unused)

## Source Tree and Module Organization

### Project Structure (Actual)

```text
salestag/
├── docs/                     # PRD and requirements documentation
│   ├── prd.md               # Comprehensive product requirements (Future vision)
│   └── *.md                 # Various design documents
├── softwareV2/              # ESP32-S3 firmware (CURRENT IMPLEMENTATION)
│   ├── main/                # Core firmware modules
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

### Key Modules and Their Current State

- **Main Application**: `main.c` - **SIMPLE IMPLEMENTATION** - Basic GPIO button/LED test loop
- **Recorder Module**: `recorder.c` - **FOUNDATION ONLY** - State machine defined, ADC microphone setup, but audio capture is placeholder
- **Audio Capture**: `audio_capture.c` - **STUB IMPLEMENTATION** - Generates silence buffer, real microphone capture not implemented
- **Storage Management**: `sd_storage.c` - **WORKING** - SPI-based SD card mounting, directory creation, file management
- **UI Module**: `ui.c` - **WORKING** - GPIO button debouncing, LED control, callback system
- **WAV Writer**: `wav_writer.c` - **FOUNDATION** - WAV file header generation and streaming write capability

## Data Models and Current File Structure

### Audio File Storage (Current Implementation)

**SD Card Organization**:
```text
/sdcard/
└── rec/                     # Recording directory
    ├── recording_001.wav    # WAV files with sequential naming
    ├── recording_002.wav
    └── [timestamp metadata as future enhancement]
```

**WAV File Format** (Actual Implementation):
- Sample Rate: 16kHz
- Bit Depth: 16-bit
- Channels: 2 (stereo - dual microphone)
- Format: PCM uncompressed
- File naming: Sequential numbering (recording_XXX.wav)

### Hardware Pin Configuration (Actual)

```c
// GPIO pin assignments (from sd_storage.h and main.c)
#define BUTTON_GPIO    4     // Button input with pullup
#define LED_GPIO       40    // Status LED output
#define SD_CS_PIN      39    // SD card chip select
#define SD_MOSI_PIN    35    // SD card SPI MOSI
#define SD_MISO_PIN    37    // SD card SPI MISO  
#define SD_SCLK_PIN    36    // SD card SPI clock
#define MIC1_ADC_CH    3     // GPIO 9 - Microphone 1 (ADC1_CH3)
#define MIC2_ADC_CH    6     // GPIO 12 - Microphone 2 (ADC1_CH6)
```

## Technical Debt and Current Implementation Gaps

### Current Limitations (vs. PRD Requirements)

1. **Audio Capture**: ADC-based microphone reading is stubbed - generates silence instead of actual audio
2. **Recording Duration**: No 10-second automatic stop implementation in current main.c
3. **BLE Communication**: Complete BLE stack missing from current build
4. **Power Management**: Basic GPIO control only, no deep sleep implementation
5. **File Metadata**: WAV files lack timestamp and metadata structure defined in PRD
6. **Mobile Integration**: No companion app exists yet

### Implementation Gaps Requiring Development

1. **Real Audio Capture**: MAX9814 amplifier integration with ESP32-S3 ADC needs implementation
2. **BLE Peripheral Mode**: ESP32-S3 BLE stack configuration for mobile pairing
3. **Recording State Machine**: Current main.c is simple GPIO test, needs full recorder integration
4. **File Transfer Protocol**: BLE characteristic definition for audio file chunks
5. **Power Optimization**: Deep sleep modes and battery monitoring via ADC
6. **Security**: Audio file encryption and secure BLE pairing

### Working Components Ready for Integration

1. **SD Card Storage**: Fully functional SPI-based storage with FAT32 filesystem
2. **GPIO Interface**: Button debouncing and LED control working reliably
3. **WAV File Generation**: Header creation and streaming write capability implemented
4. **FreeRTOS Foundation**: Task management and queuing infrastructure in place
5. **Build System**: ESP-IDF toolchain configured with proper partition layout

## Integration Points and External Dependencies

### Hardware Dependencies (Current)

| Component      | Interface | Status         | Notes                           |
| -------------- | --------- | -------------- | ------------------------------- |
| ESP32-S3 Mini  | Core MCU  | ✅ Working     | Dual-core, 240MHz, built-in BLE |
| MicroSD Card   | SPI       | ✅ Working     | 10MHz SPI, FAT32 formatted     |
| MAX9814 Mics   | ADC       | ⚠️ Stubbed     | ADC channels configured, no real capture |
| Button/LED     | GPIO      | ✅ Working     | GPIO 4 input, GPIO 40 output   |
| USB-C Charging | USB       | 🔄 Not tested  | Hardware design complete        |

### Software Dependencies (Current Build)

- **ESP-IDF 5.2.2**: Core framework for ESP32-S3 development
- **FreeRTOS**: Real-time task management (included with ESP-IDF)
- **FAT filesystem**: SD card file operations
- **ADC Driver**: Microphone input (configured but not capturing)
- **SPI Driver**: SD card communication
- **GPIO Driver**: Button/LED hardware interface

### Missing Integrations (Per PRD)

- **BLE Stack**: ESP32-S3 Bluetooth Low Energy peripheral mode
- **React Native App**: Mobile companion for audio playback and device management
- **Supabase Backend**: Cloud storage and team collaboration features
- **OTA Updates**: Over-the-air firmware update capability
- **Encryption**: AES-256 for audio files and BLE communication

## Development and Deployment

### Current Build Process

```bash
# Working development commands
cd softwareV2
idf.py set-target esp32s3        # Configure for ESP32-S3
idf.py build                     # Compile firmware  
idf.py flash                     # Program device via USB
idf.py monitor                   # View serial debug output
```

### Flash Memory Layout (Current)

```text
# From partitions.csv
Offset    Size      Purpose
0x9000    0x6000    NVS (settings storage)
0xf000    0x1000    PHY init data
0x10000   0x1F0000  Main application (1.9MB)
```

### Development Hardware Setup

1. **ESP32-S3 DevKit**: WROOM-1 or WROOM-2 development board
2. **MicroSD Card**: 32GB, FAT32 formatted for audio storage
3. **Button/LED**: Connected to GPIO 4 and GPIO 40 respectively
4. **Microphones**: MAX9814 modules connected to ADC1_CH3 and ADC1_CH6
5. **USB-C Cable**: For programming, debugging, and power

## Current vs. PRD Vision - Implementation Roadmap

### Phase 1: Current State (Diagnostic Build)
**Status**: ✅ **IMPLEMENTED**
- Basic GPIO button/LED test working
- SD card storage foundation complete
- ESP-IDF build system configured
- Hardware pin assignments defined

### Phase 2: Core Recording Functionality  
**Status**: 🔄 **IN DEVELOPMENT**
- Real audio capture from MAX9814 microphones
- 10-second recording chunks with automatic stop
- WAV file storage with timestamp metadata
- Recording state machine integration

### Phase 3: BLE Mobile Integration
**Status**: 📋 **PLANNED** (Per PRD)
- ESP32-S3 BLE peripheral implementation
- Secure device pairing with mobile app
- Audio file transfer protocol over BLE
- React Native companion app development

### Phase 4: Professional Deployment
**Status**: 📋 **PLANNED** (Per PRD) 
- Badge form factor hardware design
- Battery optimization and power management
- Enterprise device management features
- Field testing and validation

## Known Issues and Workarounds

### Current Implementation Issues

1. **Audio Capture Placeholder**: `audio_capture.c` generates silence - real microphone input needs ADC implementation
2. **No Recording Duration Control**: Current main.c doesn't implement 10-second auto-stop
3. **Missing BLE Stack**: Diagnostic build intentionally excludes wireless communication
4. **No File Metadata**: WAV files lack timestamp information required by PRD data schema

### Development Workarounds

- **SD Card Testing**: Use pre-formatted FAT32 card, device handles mounting automatically
- **Audio Testing**: Current silence generation allows testing WAV file structure
- **GPIO Validation**: Button press immediately toggles LED for hardware verification
- **Build Issues**: Use exact ESP-IDF v5.2.2 - newer versions may have compatibility issues

## Appendix - Essential Commands and Troubleshooting

### Development Commands

```bash
# Build and flash workflow
idf.py clean                     # Clean previous builds
idf.py set-target esp32s3        # Set target chip
idf.py menuconfig               # Configure build options (optional)
idf.py build                    # Compile firmware
idf.py -p /dev/ttyACM0 flash    # Flash to device (adjust port)
idf.py -p /dev/ttyACM0 monitor  # Monitor serial output
```

### Hardware Debugging

- **Button Issues**: Check GPIO 4 pullup resistor, verify 3.3V logic levels
- **LED Problems**: Confirm GPIO 40 connection, check current limiting resistor
- **SD Card Failures**: Verify SPI wiring, ensure FAT32 format, check 3.3V power
- **Serial Debug**: Use `ESP_LOGI(TAG, "message")` for debugging, view via `idf.py monitor`

### Expected Serial Output (Working System)

```text
=== SalesTag Simple Test - Button + LED Only ===
BOOT: Starting simple button test...
GPIO configured - Button: GPIO[4], LED: GPIO[40]
Button initial level: 1
=== System Ready ===
Press button to turn LED ON, release to turn OFF
```

This brownfield architecture document captures the **actual current state** of SalesTag firmware - a working foundation with basic hardware control that provides the building blocks for the complete IoT solution described in the PRD. The next development phase should focus on implementing real audio capture and integrating the existing recording state machine with the simplified main application.