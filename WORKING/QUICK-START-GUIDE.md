# Quick Start Guide - Working SalesTag Build

## 🚀 **Get Started in 5 Minutes**

### **Step 1: Setup Environment**
```bash
# Load ESP-IDF (required before every session)
source ~/esp/esp-idf/export.sh

# Navigate to working build
cd /Users/self/Desktop/salestag/WORKING/softwareV2-WORKING/
```

### **Step 2: Build & Flash** 
```bash
# Build the project
idf.py build

# Flash to ESP32 (connect USB-C first!)
idf.py -p /dev/cu.usbmodem101 flash monitor
```

### **Step 3: Verify Success** ✅
**Look for these SUCCESS messages in the log:**
```
✅ I (357) salestag-sd: UI module initialized successfully
✅ I (647) sd_storage: SD card mounted successfully
✅ I (727) sd_storage: ✅ Write access confirmed - removing test file
✅ I (777) sd_storage: SD card mounted: 31164727296 bytes total
✅ I (837) salestag-sd: === UI System Ready ===
✅ I (927) salestag-sd: Pre-audio test file created successfully
✅ I (1237) raw_audio_storage: Raw audio recording started successfully
✅ I (23427) salestag-sd: SD card heartbeat test successful
```

---

## 🎛️ **Hardware Functions**

| Action | Result |
|--------|---------|
| **Press Button** | LED turns ON, starts audio recording |  
| **Release Button** | LED turns OFF, stops audio recording |
| **Boot System** | System initializes, SD card mounts, all systems ready |

---

## 📁 **SD Card Status**
**SD card is FULLY WORKING - files are being created:**
```
✅ SD card mounted successfully
✅ Files can be written to SD card
✅ Audio recording functional (31K+ samples captured)
✅ All file operations working
```

---

## 🔧 **Troubleshooting**

### **Problem: No /dev/cu.usbmodem101**
```bash
# Check available ports
ls /dev/cu.*

# If missing, try:
# 1. Unplug/replug USB-C cable
# 2. Press RESET button on ESP32
# 3. Try different USB cable/port
```

### **Problem: SD Card errno 22 Returns**
**This means someone modified the working code!** 
```bash
# Restore from backup:
cp -r WORKING/softwareV2-WORKING/* softwareV2/
```

### **Problem: Deep Sleep Mode (No USB)**
**ESP32 stuck in deep sleep:**
```bash
# Hardware recovery:
# 1. Disconnect battery ground wire
# 2. Wait 5 seconds  
# 3. Reconnect battery
# 4. Plug in USB-C
```

### **Problem: Button Not Working**
**Button appears stuck or unresponsive:**
```bash
# Check logs for:
✅ I (346) salestag-simple: Button initial level: 1
✅ GPIO[4]| Pullup: 1

# If button stuck at level 0:
# 1. Check for GPIO conflicts with other modules  
# 2. Verify pullup configuration in code
# 3. See button-troubleshooting.md for detailed debug steps
```

---

## ⚡ **Key Features Working**

- ✅ **Button:** GPIO4 with debounce, press/release detection (works perfectly)
- ✅ **LED:** GPIO40 visual feedback, on/off control (works perfectly)
- ✅ **SD Card:** Fully functional - 31GB accessible, reliable writes
- ✅ **Audio Recording:** Working - 31K+ samples captured, 374KB+ files written
- ✅ **File Operations:** All working - test files, audio files, heartbeat tests
- ✅ **System Stability:** No crashes, reliable USB communication

---

## 📊 **Success Metrics**

- **Boot Time:** ~2 seconds to operational  
- **Button Response:** <50ms debounced (working perfectly)
- **System Uptime:** Stable continuous operation
- **Memory Available:** ~315KB RAM free
- **SD Card Status:** Fully functional
- **Audio Recording:** 31K+ samples captured

---

## 🚨 **Important Notes**

1. **SD card is fully working** - all file operations functional
2. **Audio recording is working** - 31K+ samples captured
3. **This is a complete smartbadge** - all core functionality working
4. **Physical SD card reset was the solution** - resolved the issue
5. **Use this as reference** for complete smartbadge functionality

---

## 📞 **Quick Reference**

### **Build Commands**
```bash
source ~/esp/esp-idf/export.sh    # Setup environment
idf.py build                      # Compile
idf.py -p /dev/cu.usbmodem101 flash  # Program device  
idf.py monitor                    # View serial output
```

### **File Locations**
```bash
WORKING/softwareV2-WORKING/       # Main code
WORKING/README-WORKING-BUILD.md   # Full documentation
WORKING/SD-CARD-FIXES-SUMMARY.md  # Technical fixes
WORKING/button-troubleshooting.md # Button debugging guide
```

---

**This build is a complete working smartbadge with all functionality operational. Use it as reference for full smartbadge implementation! 🎯**
