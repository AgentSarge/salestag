# SalesTag Quick Start Testing Guide

## Step-by-Step Process After Plugging In Device

### Step 1: Setup ESP-IDF Environment (5 minutes)

```bash
# 1. Open Terminal and install ESP-IDF
mkdir -p ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
git checkout v5.2.2
./install.sh esp32s3

# 2. Set up environment
. ./export.sh

# 3. Verify installation
idf.py --version
# Should show: ESP-IDF v5.2.2
```

### Step 2: Prepare Hardware (2 minutes)

```bash
# 1. Check device connection
ls /dev/tty*USB* /dev/cu.usb*  # macOS/Linux
# or check Device Manager on Windows

# 2. Insert SD card into computer, format as FAT32
# 3. Insert formatted SD card into ESP32-S3 device
# 4. Connect any external components (button, LED, microphones)
```

### Step 3: Build and Flash (3 minutes)

```bash
# 1. Navigate to project
cd /path/to/salestag/softwareV2

# 2. Source ESP-IDF (in every new terminal)
. ~/esp/esp-idf/export.sh

# 3. Set target
idf.py set-target esp32s3

# 4. Build and flash
idf.py build flash monitor
```

### Step 4: What You Should See Immediately

**Expected Serial Output:**
```
=== SalesTag Audio Recording System v1.0 ===
BOOT: Initializing components...
NVS initialized
UI initialized - button callback registered
Initializing SD card storage...
SD card storage initialized successfully
Recording directory: /sdcard/rec
Initializing recorder system...
Initializing dual microphone ADC setup with modern API
  MIC1: GPIO 9 (ADC1_CH3) - MIC_DATA1
  MIC2: GPIO 12 (ADC1_CH6) - MIC_DATA2
Recorder initialized - 16kHz, 16-bit, 2 channels
=== System Initialization Complete ===
System ready: YES
Recorder ready: YES
SD card ready: YES
=== Ready for Recording ===
Press button to start 10-second recording session
```

### Step 5: Quick Function Test (2 minutes)

**Test 1: Button Response**
1. Press button connected to GPIO 4
2. **Expected**: LED turns ON immediately
3. **Expected**: Serial shows: `Button pressed - current state: 0`
4. **Expected**: Serial shows: `Starting 10-second recording session #1`

**Test 2: Recording Cycle**
1. Wait 10 seconds
2. **Expected**: LED turns OFF automatically
3. **Expected**: Serial shows: `Recording stopped automatically after 10 seconds`
4. **Expected**: Serial shows: `Recording session #1 complete: XXXX bytes, 10000 ms`

**Test 3: File Creation**
1. Remove SD card and check on computer
2. **Expected**: File `/rec/recording_001.wav` exists
3. **Expected**: File size ~320KB (for 10-second stereo)

## Quick Results Checklist

Fill this out in 5 minutes:

| Test | Expected Result | Your Result | Status |
|------|----------------|-------------|---------|
| Build completes | No errors | ____________ | ⬜ Pass ⬜ Fail |
| Device connects | Serial output visible | ____________ | ⬜ Pass ⬜ Fail |
| System boots | "Ready for Recording" message | ____________ | ⬜ Pass ⬜ Fail |
| Button works | LED turns ON when pressed | ____________ | ⬜ Pass ⬜ Fail |
| Recording works | LED turns OFF after 10 seconds | ____________ | ⬜ Pass ⬜ Fail |
| File created | WAV file on SD card | ____________ | ⬜ Pass ⬜ Fail |

## What Each Result Means

### ✅ **All Pass**: 
- **Congratulations!** Phase 1 is working
- Move to detailed testing checklist: `docs/phase1-testing-checklist.md`
- Then proceed to Phase 2 development

### ⚠️ **Build Fails**:
- Check ESP-IDF installation: `idf.py --version`
- Verify target: `idf.py set-target esp32s3`
- Check error messages in build output

### ⚠️ **Device Not Found**:
- Try different USB port
- Check USB cable (needs data pins, not just power)
- Install USB drivers if needed

### ⚠️ **Boot Fails**:
- Check serial monitor settings (115200 baud)
- Try resetting device (press reset button)
- Check power supply (USB should provide enough)

### ⚠️ **Button Not Working**:
- Check GPIO 4 wiring (button between GPIO 4 and GND)
- Try pressing/holding reset if system seems stuck
- Verify button connection with multimeter

### ⚠️ **No LED Response**:
- Check GPIO 40 wiring (LED + resistor to GND)
- Verify LED polarity (long leg to GPIO, short to resistor)
- Test LED separately with 3.3V

### ⚠️ **No File Created**:
- Check SD card format (must be FAT32)
- Verify SD card is properly inserted
- Check SD card wiring per pin assignments

## Next Steps Based on Results

### If Quick Test PASSES:
```bash
# Run the comprehensive testing
# Use: docs/phase1-testing-checklist.md
# This includes 15 detailed tests for full validation
```

### If Issues Found:
```bash
# Use troubleshooting guides:
# Hardware issues: docs/hardware-validation-guide.md
# Build issues: docs/build-environment-setup.md
# Component issues: docs/phase1-testing-checklist.md (relevant sections)
```

## Emergency Troubleshooting

**Device Won't Flash:**
```bash
# Hold BOOT button on ESP32-S3 while running:
idf.py flash
# Release BOOT button when flashing starts
```

**No Serial Output:**
```bash
# Check baud rate and port:
idf.py -p /dev/ttyUSB0 -b 115200 monitor
# Try different port if needed
```

**System Keeps Restarting:**
```bash
# Check for infinite reset loops in monitor
# Look for "Guru Meditation Error" or stack traces
# Usually indicates hardware wiring issues
```

## Time Estimate

- **Total time for quick validation**: 15-20 minutes
- **Basic functionality confirmed**: 5 minutes  
- **Ready for detailed testing**: +10 minutes for comprehensive checklist

**Ready to start? Plug in your device and run the commands above!** 🚀