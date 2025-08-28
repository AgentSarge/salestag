#!/bin/bash

# Script to switch between full main.c and minimal SD test
# This isolates SD card issues without BLE or audio complications

echo "╔════════════════════════════════════════╗"
echo "║      MINIMAL SD CARD TEST SCRIPT       ║"
echo "╚════════════════════════════════════════╝"
echo ""

MAIN_DIR="main"
ORIGINAL_MAIN="$MAIN_DIR/main.c"
MINIMAL_TEST="$MAIN_DIR/minimal_sd_test.c"
BACKUP_MAIN="$MAIN_DIR/main.c.backup"
MINIMAL_CMAKE="$MAIN_DIR/CMakeLists.minimal.txt"

# Create minimal CMakeLists.txt for testing
echo "Creating minimal CMakeLists.txt..."
cat > "$MINIMAL_CMAKE" << 'EOF'
idf_component_register(SRCS "minimal_sd_test.c"
                            INCLUDE_DIRS "."
                            REQUIRES driver fatfs sdmmc esp_timer)
EOF

# Function to switch to minimal test
switch_to_minimal() {
    echo "🔄 Switching to minimal SD test..."
    
    # Backup original files
    if [ -f "$ORIGINAL_MAIN" ] && [ ! -f "$BACKUP_MAIN" ]; then
        cp "$ORIGINAL_MAIN" "$BACKUP_MAIN"
        echo "✅ Backed up original main.c"
    fi
    
    if [ -f "$MAIN_DIR/CMakeLists.txt" ] && [ ! -f "$MAIN_DIR/CMakeLists.txt.backup" ]; then
        cp "$MAIN_DIR/CMakeLists.txt" "$MAIN_DIR/CMakeLists.txt.backup"
        echo "✅ Backed up original CMakeLists.txt"
    fi
    
    # Switch to minimal test
    cp "$MINIMAL_TEST" "$ORIGINAL_MAIN"
    cp "$MINIMAL_CMAKE" "$MAIN_DIR/CMakeLists.txt"
    
    echo "✅ Switched to minimal SD test"
    echo ""
    echo "📋 MINIMAL TEST FEATURES:"
    echo "   • No BLE communication"
    echo "   • No audio capture"
    echo "   • No UI button handling"
    echo "   • Pure SD card mount and write testing"
    echo "   • Tests multiple SPI speeds"
    echo "   • Power cycle testing"
    echo "   • Detailed error reporting"
    echo ""
    echo "🔨 BUILD AND FLASH:"
    echo "   idf.py build flash monitor"
    echo ""
}

# Function to restore original
switch_to_original() {
    echo "🔄 Restoring original main.c..."
    
    if [ -f "$BACKUP_MAIN" ]; then
        cp "$BACKUP_MAIN" "$ORIGINAL_MAIN"
        rm "$BACKUP_MAIN"
        echo "✅ Restored original main.c"
    else
        echo "❌ No backup found!"
        exit 1
    fi
    
    if [ -f "$MAIN_DIR/CMakeLists.txt.backup" ]; then
        cp "$MAIN_DIR/CMakeLists.txt.backup" "$MAIN_DIR/CMakeLists.txt"
        rm "$MAIN_DIR/CMakeLists.txt.backup"
        echo "✅ Restored original CMakeLists.txt"
    else
        echo "❌ No CMakeLists.txt backup found!"
        exit 1
    fi
    
    echo "✅ Restored to full system"
}

# Function to show current status
show_status() {
    echo "📊 CURRENT STATUS:"
    
    if [ -f "$BACKUP_MAIN" ]; then
        echo "   Mode: MINIMAL SD TEST"
        echo "   Original main.c: backed up"
        echo ""
        echo "📁 Test program includes:"
        echo "   • SPI speed testing (400kHz to 20MHz)"
        echo "   • Power cycle reliability tests"
        echo "   • Write operation validation"
        echo "   • Filesystem permission checks"
        echo "   • Continuous monitoring mode"
    else
        echo "   Mode: FULL SYSTEM"
        echo "   Using: complete main.c with audio/UI"
    fi
}

# Parse command line arguments
case "$1" in
    "minimal")
        switch_to_minimal
        ;;
    "restore")
        switch_to_original
        ;;
    "status")
        show_status
        ;;
    *)
        echo "🔧 USAGE:"
        echo "   $0 minimal    - Switch to minimal SD test"
        echo "   $0 restore    - Restore original main.c"  
        echo "   $0 status     - Show current mode"
        echo ""
        show_status
        ;;
esac
