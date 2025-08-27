# SalesTag Build Environment Setup Guide

## Overview

This guide provides step-by-step instructions for setting up the ESP-IDF development environment to build and flash the SalesTag firmware to ESP32-S3 hardware.

## Prerequisites

- **Operating System**: macOS, Linux, or Windows
- **Hardware**: ESP32-S3 development board with USB-C connection
- **Storage**: At least 2GB free disk space for ESP-IDF
- **Python**: Version 3.8 or newer

## Step 1: Install ESP-IDF v5.2.2

### macOS Installation

```bash
# 1. Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 2. Install dependencies
brew install cmake ninja dfu-util

# 3. Create ESP development directory
mkdir -p ~/esp
cd ~/esp

# 4. Clone ESP-IDF (specific version for SalesTag)
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
git checkout v5.2.2
git submodule update --init --recursive

# 5. Install ESP-IDF tools
./install.sh esp32s3

# 6. Set up environment variables
. ./export.sh

# 7. Add to shell profile for permanent setup
echo 'alias get_idf=". $HOME/esp/esp-idf/export.sh"' >> ~/.zshrc
```

### Linux Installation

```bash
# 1. Update package list and install dependencies
sudo apt-get update
sudo apt-get install git wget flex bison gperf python3 python3-pip python3-setuptools cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

# 2. Create ESP development directory
mkdir -p ~/esp
cd ~/esp

# 3. Clone ESP-IDF (specific version for SalesTag)
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
git checkout v5.2.2
git submodule update --init --recursive

# 4. Install ESP-IDF tools
./install.sh esp32s3

# 5. Set up environment variables
. ./export.sh

# 6. Add to shell profile for permanent setup
echo 'alias get_idf=". $HOME/esp/esp-idf/export.sh"' >> ~/.bashrc
```

### Windows Installation

```cmd
# 1. Download and install ESP-IDF Tools Installer from:
# https://dl.espressif.com/dl/esp-idf/

# 2. During installation, select:
#    - ESP-IDF version: v5.2.2
#    - Target: ESP32-S3
#    - Download server: Espressif

# 3. After installation, use "ESP-IDF Command Prompt" for all commands
```

## Step 2: Verify ESP-IDF Installation

```bash
# 1. Source the environment (do this in every new terminal)
get_idf
# or manually:
. ~/esp/esp-idf/export.sh

# 2. Verify installation
idf.py --version
# Expected output: ESP-IDF v5.2.2

# 3. Check available targets
idf.py --list-targets
# Should include: esp32s3

# 4. Test with hello world
cd ~/esp
cp -r esp-idf/examples/get-started/hello_world .
cd hello_world
idf.py set-target esp32s3
idf.py build
```

**Expected Output:**
```
Project build complete. To flash, run:
  idf.py flash
```

## Step 3: Hardware Setup

### ESP32-S3 Development Board Preparation

1. **Board Selection**: Use ESP32-S3-DevKitC-1 or compatible board
2. **USB Connection**: Connect via USB-C cable (data cable, not charge-only)
3. **Driver Installation**: Install USB-to-UART bridge drivers if needed

### Hardware Connections for SalesTag

Based on `softwareV2/main/sd_storage.h` and GPIO configuration:

| Component | ESP32-S3 GPIO | Connection Notes |
|-----------|---------------|------------------|
| Button | GPIO 4 | Connect button between GPIO 4 and GND, internal pullup enabled |
| LED | GPIO 40 | Connect LED + resistor (220Ω) between GPIO 40 and GND |
| Microphone 1 (MIC1) | GPIO 9 (ADC1_CH3) | MAX9814 output to GPIO 9 |
| Microphone 2 (MIC2) | GPIO 12 (ADC1_CH6) | MAX9814 output to GPIO 12 |
| SD Card CS | GPIO 39 | SD card chip select |
| SD Card MOSI | GPIO 35 | SD card SPI MOSI |
| SD Card MISO | GPIO 37 | SD card SPI MISO |
| SD Card SCLK | GPIO 36 | SD card SPI clock |
| SD Card VCC | 3.3V | Power supply |
| SD Card GND | GND | Ground |

### Microphone Wiring (MAX9814)

```
MAX9814 Module -> ESP32-S3
VCC -----------> 3.3V
GND -----------> GND
OUT -----------> GPIO 9 (MIC1) or GPIO 12 (MIC2)
GAIN ----------> Optional gain control (leave floating for default)
```

### SD Card Module Wiring

```
SD Card Module -> ESP32-S3
VCC -----------> 3.3V
GND -----------> GND
CS ------------> GPIO 39
MOSI ----------> GPIO 35
MISO ----------> GPIO 37
SCLK ----------> GPIO 36
```

## Step 4: Build SalesTag Firmware

```bash
# 1. Navigate to SalesTag project
cd /path/to/salestag/softwareV2

# 2. Source ESP-IDF environment
get_idf

# 3. Set target to ESP32-S3
idf.py set-target esp32s3

# 4. Build the project
idf.py build

# 5. Check build output
# Look for: "Project build complete. To flash, run: idf.py flash"
```

### Build Troubleshooting

**Common Issues and Solutions:**

1. **"idf.py: command not found"**
   ```bash
   # Solution: Source the ESP-IDF environment
   . ~/esp/esp-idf/export.sh
   ```

2. **"Target 'esp32s3' not supported"**
   ```bash
   # Solution: Ensure ESP-IDF v5.2.2 is installed
   idf.py --version
   ```

3. **"Missing CMakeLists.txt"**
   ```bash
   # Solution: Ensure you're in the softwareV2 directory
   cd /path/to/salestag/softwareV2
   ls CMakeLists.txt  # Should exist
   ```

4. **Component include errors**
   ```bash
   # Solution: Check all header files exist
   ls main/*.h
   # Should see: audio_capture.h, recorder.h, sd_storage.h, ui.h, etc.
   ```

## Step 5: Flash and Monitor

### Device Connection

1. **Connect ESP32-S3**: Use USB-C cable to connect board to computer
2. **Check Port**: Device should appear as `/dev/ttyUSB0` (Linux), `/dev/cu.usbserial-*` (macOS), or `COM*` (Windows)

```bash
# 1. Flash the firmware
idf.py flash

# Expected output:
# Connecting.......
# Chip is ESP32-S3 (revision v0.1)
# Successfully flashed

# 2. Monitor serial output
idf.py monitor

# Expected initial output:
# === SalesTag Audio Recording System v1.0 ===
# BOOT: Initializing components...
```

### Monitor Commands

While in monitor mode:
- **Ctrl+C**: Exit monitor
- **Ctrl+T, Ctrl+R**: Reset device
- **Ctrl+T, Ctrl+H**: Show help

### Flash Troubleshooting

**Common Issues:**

1. **"Failed to connect"**
   ```bash
   # Solution: Check USB connection and try reset
   # Hold BOOT button while connecting, then release
   idf.py flash
   ```

2. **"Permission denied on /dev/ttyUSB0"**
   ```bash
   # Linux solution: Add user to dialout group
   sudo usermod -a -G dialout $USER
   # Log out and back in
   ```

3. **"Port not found"**
   ```bash
   # Check available ports
   # Linux/macOS:
   ls /dev/tty*
   # Windows:
   # Check Device Manager
   ```

## Step 6: Prepare SD Card

```bash
# 1. Format SD card as FAT32
# Use system disk utility or command line:

# macOS:
diskutil list
diskutil eraseDisk FAT32 SALESTAG /dev/diskN  # Replace N with disk number

# Linux:
sudo mkfs.vfat -F 32 /dev/sdX1  # Replace X with device letter

# Windows:
# Use File Explorer -> Right-click -> Format -> FAT32
```

## Step 7: Development Workflow

### Daily Development Setup

```bash
# 1. Open terminal
# 2. Source ESP-IDF environment
get_idf

# 3. Navigate to project
cd /path/to/salestag/softwareV2

# 4. Make code changes
# 5. Build and flash
idf.py build flash monitor
```

### Quick Commands Reference

```bash
# Build only
idf.py build

# Flash only (after build)
idf.py flash

# Monitor only
idf.py monitor

# Clean build
idf.py clean

# Full rebuild
idf.py fullclean build

# Flash and monitor in one command
idf.py flash monitor

# Flash to specific port
idf.py -p /dev/ttyUSB0 flash monitor

# Monitor with custom baud rate
idf.py -p /dev/ttyUSB0 -b 115200 monitor
```

## Step 8: Development Environment Validation

### Pre-Testing Checklist

Before running the Phase 1 testing checklist:

- [ ] ESP-IDF v5.2.2 installed and sourced
- [ ] SalesTag project builds without errors
- [ ] ESP32-S3 board connected and recognized
- [ ] Hardware connections verified (button, LED, microphones, SD card)
- [ ] SD card formatted as FAT32 and inserted
- [ ] Serial monitor working and showing boot messages

### Build Environment Test

```bash
# Run this complete test sequence:

# 1. Clean and build
cd /path/to/salestag/softwareV2
idf.py clean
idf.py build

# 2. Flash and verify
idf.py flash monitor

# 3. Look for this output sequence:
# === SalesTag Audio Recording System v1.0 ===
# BOOT: Initializing components...
# NVS initialized
# UI initialized - button callback registered
# ...
# === Ready for Recording ===
```

## Troubleshooting Reference

### ESP-IDF Issues

| Problem | Symptom | Solution |
|---------|---------|----------|
| Environment not sourced | `idf.py: command not found` | Run `get_idf` or `. ~/esp/esp-idf/export.sh` |
| Wrong version | Build errors with new APIs | Ensure ESP-IDF v5.2.2: `git checkout v5.2.2` |
| Missing tools | Python errors during build | Run `./install.sh esp32s3` again |
| PATH issues | Tools not found | Re-run `./export.sh` |

### Hardware Issues

| Problem | Symptom | Solution |
|---------|---------|----------|
| Board not detected | Flash connection fails | Check USB cable, try different port |
| Permission errors | Access denied to port | Add user to dialout group (Linux) |
| Flash failures | Connection timeouts | Hold BOOT button during flash |
| Reset loops | Device keeps restarting | Check power supply, hardware connections |

### Build Issues

| Problem | Symptom | Solution |
|---------|---------|----------|
| Missing includes | Header file not found | Verify all `.h` files exist in `main/` |
| Component errors | Undefined references | Check `CMakeLists.txt` includes all components |
| Memory errors | Linker failures | Verify partition table and app size |
| Config issues | Build configuration errors | Run `idf.py menuconfig` to check settings |

## Next Steps

After successful build environment setup:

1. **Run Phase 1 Testing**: Use `docs/phase1-testing-checklist.md`
2. **Hardware Validation**: Complete all hardware connection tests
3. **Audio System Testing**: Verify microphone and recording functionality
4. **Performance Measurement**: Validate timing and quality requirements

## Support Resources

- **ESP-IDF Documentation**: https://docs.espressif.com/projects/esp-idf/en/v5.2.2/
- **ESP32-S3 Datasheet**: https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf
- **SalesTag Project Documentation**: `docs/` directory in this project
- **Hardware Design**: `device_hardware_info/` directory for PCB and component details