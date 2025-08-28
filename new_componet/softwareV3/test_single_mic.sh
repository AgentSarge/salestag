#!/bin/bash

# SalesTag Single MAX9814 Microphone Test Script
# This script validates the single microphone implementation

echo "🎤 SalesTag Single MAX9814 Microphone Test"
echo "=========================================="

# Check if we're in the right directory
if [ ! -f "main/main.c" ]; then
    echo "❌ Error: Please run this script from the softwareV3 directory"
    exit 1
fi

echo ""
echo "🔍 Checking configuration..."

# Check audio capture configuration
echo "📋 Audio Capture Configuration:"
if grep -q "audio_capture_init(16000, 1)" main/main.c; then
    echo "✅ Single microphone configured (16kHz, mono)"
else
    echo "❌ Audio configuration not found or incorrect"
fi

# Check ADC channel configuration
echo ""
echo "🔌 ADC Configuration:"
if grep -q "ADC_CHANNEL_3.*GPIO 9" main/audio_capture.c; then
    echo "✅ Single ADC channel configured (GPIO 9)"
else
    echo "❌ ADC channel configuration not found"
fi

# Check MAX9814 settings
echo ""
echo "🎛️ MAX9814 Configuration:"
if grep -q "MAX9814_GAIN_DB.*40.0f" main/audio_capture.c; then
    echo "✅ 40dB gain setting configured"
else
    echo "❌ MAX9814 gain setting not found"
fi

if grep -q "MAX9814_AGC_ENABLED.*true" main/audio_capture.c; then
    echo "✅ AGC enabled"
else
    echo "❌ AGC configuration not found"
fi

# Check for any dual microphone references
echo ""
echo "🔍 Checking for dual microphone references:"
dual_refs=$(grep -i "dual\|stereo\|two.*mic" main/*.c | wc -l)
if [ $dual_refs -eq 0 ]; then
    echo "✅ No dual microphone references found"
else
    echo "⚠️  Found $dual_refs dual microphone references:"
    grep -i "dual\|stereo\|two.*mic" main/*.c
fi

# Check build configuration
echo ""
echo "🔨 Build Configuration:"
if [ -f "sdkconfig" ]; then
    echo "✅ SDK configuration file found"
    if grep -q "CONFIG_SOC_ADC_DIGI_CONTROLLER_NUM=2" sdkconfig; then
        echo "✅ ADC controller configured"
    else
        echo "❌ ADC controller configuration not found"
    fi
else
    echo "❌ SDK configuration file not found"
fi

# Check CMakeLists.txt
echo ""
echo "📦 Build System:"
if [ -f "CMakeLists.txt" ]; then
    echo "✅ CMakeLists.txt found"
fi
if [ -f "main/CMakeLists.txt" ]; then
    echo "✅ Main CMakeLists.txt found"
    if grep -q "audio_capture" main/CMakeLists.txt; then
        echo "✅ Audio capture component included"
    else
        echo "❌ Audio capture component not found in main/CMakeLists.txt"
    fi
else
    echo "❌ Main CMakeLists.txt not found"
fi

echo ""
echo "🎯 Hardware Connection Checklist:"
echo "=================================="
echo "MAX9814 Module -> ESP32-S3:"
echo "  VCC -----------> 3.3V"
echo "  GND -----------> GND"
echo "  OUT -----------> GPIO 9 (ADC1_CH3)"
echo "  GAIN ----------> 3.3V (40dB gain)"
echo "  SHDN ----------> 3.3V (enabled)"
echo ""

echo "📊 Expected ADC Readings:"
echo "========================="
echo "Silence:      ~1550 (1.25V DC bias)"
echo "Quiet speech: 1200-1900"
echo "Normal speech: 800-2300"
echo "Loud speech:   400-2700"
echo ""

echo "🚀 Next Steps:"
echo "=============="
echo "1. Connect MAX9814 module per wiring diagram above"
echo "2. Build and flash: idf.py build flash"
echo "3. Monitor output: idf.py monitor"
echo "4. Test recording with button press"
echo "5. Check ADC values in serial monitor"
echo ""

echo "✅ Single MAX9814 microphone implementation ready for testing!"
