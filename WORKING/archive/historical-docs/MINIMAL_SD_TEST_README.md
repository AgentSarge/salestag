# Minimal SD Card Test

This test isolates SD card mount and write issues by removing all BLE and audio capture complexity.

## Problem Context

Your SD card was reformatted and confirmed working on a PC. Any `errno 22` (read-only) errors are now confirmed to be coming from the ESP32 side - either electrical (wiring, pullups, SPI timing) or software (mount sequence, write operations).

## Quick Start

1. **Switch to minimal test:**
   ```bash
   cd softwareV2
   ./test_minimal_sd.sh minimal
   ```

2. **Build and flash:**
   ```bash
   idf.py build flash monitor
   ```

3. **Restore full system when done:**
   ```bash
   ./test_minimal_sd.sh restore
   ```

## What This Test Does

### 🔍 Electrical Issue Detection
- Tests multiple SPI speeds (400kHz to 20MHz)
- If **NO speeds work** → Electrical issue (wiring, pullups, power)
- If **some speeds work** → Software/timing issue

### ⚡ Power Cycle Testing
- Tests mounting after power cycles
- Validates timing between operations
- Checks for state corruption issues

### 📁 Filesystem Validation
- Mount point permission checks
- Multiple file size write tests
- Directory creation tests
- Sync operation validation

### 🔄 Continuous Monitoring
- Periodic write access tests
- Automatic recovery attempts
- Long-term stability validation

## Expected Output

### ✅ Healthy SD Card
```
=== Testing SPI Speed: 1MHz (Slow) ===
  ✅ SPI bus initialized
  ✅ SD card mounted successfully
  📊 Card size: 32212254720 bytes (30.0 GB)
  ✅ Write test passed (45 bytes written)
  ✅ Multi-write test passed (10 lines written)
  ✅ Directory creation passed
```

### ❌ Electrical Issues
```
=== Testing SPI Speed: 400kHz (Very Slow) ===
  ✅ SPI bus initialized
  ❌ Mount failed: ESP_ERR_NOT_FOUND
❌ NO SPI SPEEDS WORKED - LIKELY ELECTRICAL ISSUE
Check:
  - Wiring connections
  - Pull-up resistors on CS/MISO
  - Power supply stability
```

### ⚠️ Software/Timing Issues
```
=== Testing SPI Speed: 1MHz (Slow) ===
  ✅ SD card mounted successfully
  ❌ Write test failed (errno: 22 - Read-only file system)
✅ AT LEAST ONE SPI SPEED WORKS
Issue is likely software/timing related, not electrical
```

## Hardware Configuration

The test uses your current pin configuration:
- **CS:** GPIO 39
- **MOSI:** GPIO 35
- **MISO:** GPIO 37  
- **SCLK:** GPIO 36
- **SPI Host:** SPI2_HOST

## Troubleshooting Guide

### If NO SPI speeds work:
1. Check physical SD card insertion
2. Verify all 4 SPI wires are connected
3. Add 10kΩ pull-up resistors on CS and MISO if missing
4. Check 3.3V power supply stability
5. Try a different SD card

### If SOME speeds work but writes fail:
1. Use slower SPI speeds (400kHz-1MHz)
2. Add 500ms delay after mount before first write
3. Implement power cycling between operations
4. Use smaller allocation unit sizes (512 bytes)
5. Check for filesystem corruption

### If writes work initially but fail later:
1. Power supply voltage drops under load
2. Temperature-related issues
3. SD card wearing out
4. Timing sensitivity to other system tasks

## Key Differences from Full System

| Full System | Minimal Test |
|-------------|--------------|
| Complex initialization | Simple, focused init |
| Audio capture interfering | No audio tasks |
| BLE communication overhead | No BLE |
| Multiple concurrent tasks | Single-threaded |
| Complex error handling | Clear, isolated testing |

## Recovery Commands

```bash
# Check current mode
./test_minimal_sd.sh status

# Switch to minimal test
./test_minimal_sd.sh minimal
idf.py build flash monitor

# Restore full system
./test_minimal_sd.sh restore
idf.py build flash monitor
```

## Files Created

The test creates several files on the SD card:
- `speed_test_*.txt` - SPI speed validation
- `power_cycle_test_*.txt` - Power cycle validation  
- `size_test_*.txt` - Different file size tests
- `status_check.txt` - Periodic health checks

These files help verify that write operations are working correctly and can be safely deleted after testing.
