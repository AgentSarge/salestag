# SalesTag WORKING Build - Complete Documentation

**Date**: August 25, 2025  
**Status**: ✅ **STABLE** - USB Communication Fixed, Core Systems Working  
**Source of Truth**: `/Users/self/Desktop/salestag/WORKING/softwareV2/`

## 🎯 **Current Status**

### ✅ **Working Systems:**
- **USB Serial/JTAG**: Stable communication, no bricking issues
- **Button Detection**: Perfect debouncing (GPIO 4) with 3-consecutive reading validation
- **LED Control**: Hardware confirmed working (GPIO 40) with immediate ON/OFF toggling
- **SD Card Storage**: Fully functional - 31GB accessible, reliable writes
- **Audio Recording**: Working - 31K+ samples captured, 374KB+ files written
- **File Operations**: All working - test files, audio files, heartbeat tests
- **System Stability**: No crashes, responsive, flashable
- **Audio System**: ADC oneshot mode working (GPIO 9, 12) at 1kHz


## 🚀 **Quick Start**

### **Flash & Test:**
```bash
cd /Users/self/Desktop/salestag/WORKING/softwareV2
source ~/esp/esp-idf/export.sh
idf.py -p /dev/cu.usbmodem101 flash monitor
```

### **Expected Behavior:**
- **Button Press**: LED toggles ON/OFF (this works perfectly)
- **System Logs**: Shows stable initialization with SD card success
- **USB**: Remains flashable after operation
- **SD Card**: Will show successful mounting and file operations
- **Audio**: Will show samples captured and files written successfully

## 🔧 **Critical Fixes Applied**

### **1. USB Communication Crisis - SOLVED**
**Problem**: Audio task priority 24 monopolized CPU, starving USB Serial/JTAG  
**Solution**: Lowered audio task priority from 24 → 5

```c
// FIXED in audio_capture.c:
xTaskCreate(audio_capture_task, "audio_capture", 4096, NULL, 5, &task);
```

### **2. ChatGPT 5 Fixes - Applied**
- **Audio Timing**: Fixed `xTaskDelayUntil` assertion with `vTaskDelay`
- **Button GPIO**: Reassert GPIO config after audio init
- **Both fixes prevent crashes and GPIO stuck LOW issues**

### **3. SD Card Status - FULLY WORKING** 
**Current State**: SD card mounting successful with reliable file operations
**Solution**: Physical SD card reset resolved the issue
**Impact**: All file operations working, audio recording functional

```c
// Current success pattern:
I (647) sd_storage: SD card mounted successfully
I (727) sd_storage: ✅ Write access confirmed - removing test file
I (777) sd_storage: SD card mounted: 31164727296 bytes total
```

## 📋 **System Architecture**

### **Hardware Pins:**
- **Button**: GPIO 4 (pullup enabled, 50ms debounce)
- **LED**: GPIO 40 (output mode)
- **Microphones**: GPIO 9 (MIC1), GPIO 12 (MIC2) - ADC oneshot
- **SD Card SPI**: CS=39, MOSI=35, MISO=37, CLK=36

### **Software Stack:**
- **UI Layer**: Button polling with 3-consecutive debouncing, LED control
- **Audio Layer**: ADC oneshot sampling at 1kHz, dual microphone support
- **Storage Layer**: Raw audio storage system, SD card with FatFS
- **Integration Layer**: Button → LED → Audio → SD storage chain

### **Task Priorities:**
- **Audio Capture**: Priority 5 (safe, won't interfere with system tasks)
- **Button Polling**: Default priority
- **System Tasks**: Priority 25+ (USB, WiFi, etc.)

## 🧪 **Development Workflow**

### **Build & Flash:**
```bash
cd /Users/self/Desktop/salestag/WORKING/softwareV2
source ~/esp/esp-idf/export.sh
idf.py build
idf.py -p /dev/cu.usbmodem101 flash monitor
```

### **USB Recovery (if needed):**
1. Disconnect USB cable for 10 seconds
2. Disconnect battery ground wire
3. Reconnect battery, then USB
4. Hold BOOT + press RESET for manual download mode

## 📚 **Reference Documentation**

### **Troubleshooting Guides:**
- `button-troubleshooting.md` - Button debugging history
- `SD-CARD-FIXES-SUMMARY.md` - errno 22 fixes (file operations)

### **Quick References:**
- `QUICK-START-GUIDE.md` - Testing instructions
- **This README** - Complete current status

## 🔄 **Next Steps**

### **Current Status:**
1. **✅ SD Card Working** - Physical reset resolved the issue
2. **✅ Audio Recording Working** - 31K+ samples captured successfully
3. **✅ File System Working** - All file operations functional

### **Future Enhancements:**
- Power consumption optimization
- Audio format improvements
- User interface refinements

## 🏆 **Key Achievements**

### **Major Breakthroughs:**
1. **USB Communication Crisis Resolved** - Root cause found and fixed
2. **System Stability Achieved** - No crashes, reliable operation
3. **Expert Fixes Integrated** - ChatGPT 5's solutions successfully applied
4. **Foundation Complete** - Ready for final SD card resolution

### **Development Methodology:**
- **Systematic debugging** with isolated testing
- **Expert consultation** for complex issues
- **Incremental validation** of each fix
- **Single source of truth** for clarity

---

## 📍 **Current State: FULLY FUNCTIONAL SMARTBADGE**

**This build represents a complete working smartbadge system:**
- ✅ **Button/LED interface working perfectly**
- ✅ **SD card storage fully functional**
- ✅ **Audio recording working (31K+ samples captured)**
- ✅ **File operations working reliably**
- ✅ **System stability achieved**
- ✅ **USB communication reliable**

**Status**: All core smartbadge functionality is working!
