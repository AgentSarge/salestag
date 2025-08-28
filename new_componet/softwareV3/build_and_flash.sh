#!/bin/bash

# ESP32 Build, Flash and Monitor Script
# Automates the complete workflow for ESP-IDF projects
#
# Usage: ./build_and_flash.sh
#
# This script will:
# 1. Navigate to the project directory
# 2. Set up ESP-IDF environment
# 3. Always perform full clean and build the project
# 4. Flash to the connected ESP32
# 5. Always start serial monitor (Ctrl+C to exit)

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration - Update these paths for your system
# Resolve the actual script location (handles symlinks)
SCRIPT_DIR="$(cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}" || echo "${BASH_SOURCE[0]}")")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"
ESP_IDF_EXPORT="/Users/self/esp/esp-idf/export.sh"
SERIAL_PORT="/dev/cu.usbmodem101"

echo -e "${BLUE}🚀 ESP32 Build & Flash Script${NC}"
echo -e "${BLUE}================================${NC}"
echo -e "${YELLOW}Configuration:${NC}"
echo -e "  Project: $PROJECT_DIR"
echo -e "  ESP-IDF: $ESP_IDF_EXPORT"
echo -e "  Serial:  $SERIAL_PORT"
echo ""

# Function to print status messages
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if ESP-IDF export script exists
check_esp_idf() {
    if [ ! -f "$ESP_IDF_EXPORT" ]; then
        print_error "ESP-IDF export script not found at: $ESP_IDF_EXPORT"
        print_error "Please update the ESP_IDF_EXPORT path in this script"
        exit 1
    fi
}

# Check if serial port exists
check_serial_port() {
    if [ ! -e "$SERIAL_PORT" ]; then
        print_warning "Serial port $SERIAL_PORT not found"
        print_warning "Device may not be connected or port name may be different"
        print_warning "Available ports:"
        ls /dev/cu.usbmodem* 2>/dev/null || ls /dev/tty.usbmodem* 2>/dev/null || echo "No USB serial devices found"
    fi
}

# Step 1: Navigate to project directory
step1_navigate() {
    print_status "Step 1: Navigating to project directory..."
    cd "$PROJECT_DIR"
    if [ $? -eq 0 ]; then
        print_status "✅ Successfully navigated to: $(pwd)"
    else
        print_error "Failed to navigate to project directory"
        exit 1
    fi
}

# Step 2: Set up ESP-IDF environment
step2_setup_environment() {
    print_status "Step 2: Setting up ESP-IDF environment..."
    if source "$ESP_IDF_EXPORT"; then
        print_status "✅ ESP-IDF environment activated"
        print_status "IDF_PATH: $IDF_PATH"
        print_status "IDF_PY: $IDF_PY"
    else
        print_error "Failed to source ESP-IDF export script"
        exit 1
    fi
}

# Step 3: Clean and build
step3_build() {
    print_status "Step 3: Building project..."

    # Always perform full clean
    print_status "Performing full clean..."
    if idf.py fullclean; then
        print_status "✅ Full clean completed"
    else
        print_warning "Full clean failed, but continuing..."
    fi

    # Build the project
    print_status "Building project..."
    if idf.py build; then
        print_status "✅ Build completed successfully!"
    else
        print_error "Build failed!"
        exit 1
    fi
}

# Step 4: Flash to device
step4_flash() {
    print_status "Step 4: Flashing to device..."

    # Check serial port before flashing
    check_serial_port

    if idf.py flash --port "$SERIAL_PORT"; then
        print_status "✅ Flash completed successfully!"
    else
        print_error "Flash failed!"
        exit 1
    fi
}

# Step 5: Monitor (always enabled)
step5_monitor() {
    print_status "Step 5: Starting serial monitor..."
    print_status "Monitor will show device logs..."
    echo -e "${YELLOW}Press Ctrl+C to exit monitor${NC}"
    idf.py monitor --port "$SERIAL_PORT"
}

# Main execution
main() {
    echo ""
    check_esp_idf

    step1_navigate
    step2_setup_environment
    step3_build
    step4_flash
    step5_monitor

    # Note: step5_monitor will run indefinitely until user presses Ctrl+C
    # The lines below will only execute if monitor is exited
    echo ""
    print_status "🎉 Build and flash completed successfully!"
    print_status "Serial monitor was running - ESP32 firmware updated!"
}

# Handle script interruption
trap 'echo -e "\n${YELLOW}Script interrupted by user${NC}"; exit 1' INT

# Run main function
main "$@"
