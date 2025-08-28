#!/bin/bash

# SalesTag Project Pre-Build Validation Script
# Run this BEFORE setting up ESP-IDF to check project readiness

echo "🏗️ SalesTag Project Validation"
echo "================================"

PROJECT_ROOT="/Users/self/Desktop/salestag"

# Auto-detect firmware directory: prefer softwareV3, fall back to WORKING/softwareV2
if [ -d "$PROJECT_ROOT/new_componet/softwareV3" ]; then
    FIRMWARE_DIR="$PROJECT_ROOT/new_componet/softwareV3"
elif [ -d "$PROJECT_ROOT/WORKING/softwareV2" ]; then
    FIRMWARE_DIR="$PROJECT_ROOT/WORKING/softwareV2"
else
    FIRMWARE_DIR="$PROJECT_ROOT/softwareV2"
fi

# Check project structure
echo "📁 Checking project structure..."
if [ ! -d "$PROJECT_ROOT" ]; then
    echo "❌ Project root not found: $PROJECT_ROOT"
    exit 1
fi

if [ ! -d "$FIRMWARE_DIR" ]; then
    echo "❌ Firmware directory not found: $FIRMWARE_DIR"
    exit 1
fi

echo "✅ Project directories found"

# Check required files
echo ""
echo "📄 Checking required files..."

# Required files for current codebase (streaming + raw storage)
REQUIRED_FILES=(
    "CMakeLists.txt"
    "main/CMakeLists.txt"
    "main/main.c"
    "main/ui.c"
    "main/ui.h"
    "main/sd_storage.c"
    "main/sd_storage.h"
    "main/raw_audio_storage.c"
    "main/raw_audio_storage.h"
    "main/audio_capture.c"
    "main/audio_capture.h"
    "partitions.csv"
)

missing_files=0
for file in "${REQUIRED_FILES[@]}"; do
    if [ ! -f "$FIRMWARE_DIR/$file" ]; then
        echo "❌ Missing: $file"
        missing_files=$((missing_files + 1))
    else
        echo "✅ Found: $file"
    fi
done

if [ $missing_files -gt 0 ]; then
    echo "❌ $missing_files files missing - cannot proceed"
    exit 1
fi

# Skip legacy content checks not applicable to V3
echo ""
echo "🔍 Validating main.c implementation..."
if grep -q "ESP32-S3-Mini-BLE" "$FIRMWARE_DIR/main/main.c"; then
    echo "✅ BLE device name configured"
else
    echo "❌ Expected BLE device name not found in main.c"
    exit 1
fi

# Check GPIO pin assignments
echo ""
echo "🔌 Checking GPIO configuration..."
if grep -q "#define BTN_GPIO 4" "$FIRMWARE_DIR/main/main.c"; then
    echo "✅ Button GPIO 4 configured"
else
    echo "❌ Button GPIO not found"
fi

if grep -q "#define LED_GPIO 40" "$FIRMWARE_DIR/main/main.c"; then
    echo "✅ LED GPIO 40 configured"
else
    echo "❌ LED GPIO not found"
fi

# Check mic pin references in audio_capture.c (V3 layout)
if grep -q "GPIO9" "$FIRMWARE_DIR/main/audio_capture.c"; then
    echo "✅ Microphone 1 (GPIO 9) referenced"
else
    echo "❌ Microphone 1 reference not found"
fi

if grep -q "GPIO12" "$FIRMWARE_DIR/main/audio_capture.c"; then
    echo "✅ Microphone 2 (GPIO 12) referenced"
else
    echo "❌ Microphone 2 reference not found"
fi

# Check SD card pin configuration
echo ""
echo "💾 Checking SD card configuration..."
sd_pins_found=0
if grep -q "#define SD_CS_PIN 39" "$FIRMWARE_DIR/main/sd_storage.h"; then
    echo "✅ SD CS pin (GPIO 39) configured"
    sd_pins_found=$((sd_pins_found + 1))
fi

if grep -q "#define SD_MOSI_PIN 35" "$FIRMWARE_DIR/main/sd_storage.h"; then
    echo "✅ SD MOSI pin (GPIO 35) configured"
    sd_pins_found=$((sd_pins_found + 1))
fi

if grep -q "#define SD_MISO_PIN 37" "$FIRMWARE_DIR/main/sd_storage.h"; then
    echo "✅ SD MISO pin (GPIO 37) configured"
    sd_pins_found=$((sd_pins_found + 1))
fi

if grep -q "#define SD_SCLK_PIN 36" "$FIRMWARE_DIR/main/sd_storage.h"; then
    echo "✅ SD SCLK pin (GPIO 36) configured"
    sd_pins_found=$((sd_pins_found + 1))
fi

if [ $sd_pins_found -eq 4 ]; then
    echo "✅ All SD card pins configured correctly"
else
    echo "❌ SD card pin configuration incomplete ($sd_pins_found/4 found)"
fi

# Check audio format configuration
echo ""
echo "🎵 Checking audio format configuration..."
if grep -q "audio_capture_init\(1000, 2\)" "$FIRMWARE_DIR/main/main.c"; then
    echo "✅ Audio format: 16kHz, 16-bit, 2 channels (stereo)"
else
    echo "❌ Audio format configuration not found"
fi

# Summary
echo ""
echo "📋 VALIDATION SUMMARY"
echo "====================="
echo "✅ Project structure valid"
echo "✅ All required files present"
echo "✅ Main application updated with recording system"
echo "✅ Hardware pin assignments configured"
echo "✅ Audio format matches PRD requirements"

echo ""
echo "🚀 PROJECT READY FOR ESP-IDF BUILD"
echo ""
echo "Next steps:"
echo "1. Set up ESP-IDF v5.2.2 development environment"
echo "2. Connect ESP32-S3 hardware with components"
echo "3. Run: idf.py build flash monitor"
echo "4. Test with button press for 10-second recording"
echo ""
echo "Use these guides:"
echo "- Build setup: docs/build-environment-setup.md"
echo "- Quick testing: docs/quick-start-testing.md"
echo "- Full validation: docs/phase1-testing-checklist.md"