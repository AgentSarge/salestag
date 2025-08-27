#!/bin/bash

echo "=== SalesTag BLE Testing Script ==="
echo "This script helps test and debug BLE advertising issues"
echo ""

# Check if we're in the right directory
if [ ! -f "main/CMakeLists.txt" ]; then
    echo "Error: Please run this script from the softwareV3 directory"
    exit 1
fi

# Source ESP-IDF environment
echo "Setting up ESP-IDF environment..."
source ~/esp/esp-idf/export.sh

echo "1. Building the project..."
idf.py build

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo ""
echo "2. Flashing the device..."
idf.py flash

if [ $? -ne 0 ]; then
    echo "Flash failed!"
    exit 1
fi

echo ""
echo "3. Starting monitor to check BLE initialization..."
echo "Look for these key messages in the output:"
echo "  - 'NimBLE Host Stack is synchronized'"
echo "  - 'Advertising started successfully'"
echo "  - 'BLE Host Stack Synced: YES'"
echo "  - 'Advertising Status: ACTIVE'"
echo ""
echo "Press Ctrl+C to stop monitoring when done"
echo ""

idf.py monitor
