# SalesTag Smart Badge Project

**Audio Recording Smart Badge with ESP32-S3**

## 🎯 **Project Status**
- **Hardware**: ESP32-S3 with dual microphones, SD card, button, LED
- **Current Status**: Core systems working, SD card troubleshooting in progress  
- **Latest Build**: See `/WORKING/README.md` for current status

## 📁 **Repository Structure**

### **Active Development**
```
softwareV2/                 ← 🎯 Main firmware source code
WORKING/                    ← 📍 Current build documentation & stable code
├── softwareV2/            ← Stable backup of working code
├── README.md              ← Current build status & instructions
├── QUICK-START-GUIDE.md   ← Quick testing guide
└── archive/               ← Historical documents (preserved for reference)
```

### **Hardware & Architecture**
```
device_hardware_info/       ← Hardware specs, schematics, BOM
docs/                       ← Architecture documentation
```

### **Tools & Utilities**
```
ble_audio_receiver.py      ← Python script for receiving audio via BLE
requirements.txt           ← Python dependencies
txt.md                     ← Useful command references
validate-project.sh        ← Project validation script
```

## 🚀 **Quick Start**

1. **Get Latest Status:**
   ```bash
   cat WORKING/README.md
   ```

2. **Flash Current Build:**
   ```bash
   cd softwareV2
   source ~/esp/esp-idf/export.sh
   idf.py -p /dev/cu.usbmodem101 flash monitor
   ```

3. **Test System:**
   - Press button → LED should toggle
   - Monitor serial output for system status

## 🏗️ **Development Workflow**

### **Working on Issues:**
1. Use `/softwareV2/` for active development
2. Document changes in `/WORKING/README.md`
3. Create stable backups in `/WORKING/softwareV2/` when major milestones reached

### **Before Making Changes:**
- Always check current status: `cat WORKING/README.md`
- Read relevant troubleshooting docs in `/WORKING/`
- Review historical context in `/WORKING/archive/` if needed

## 📋 **Key Documentation**

- **Current Build Status**: `/WORKING/README.md`
- **Quick Testing**: `/WORKING/QUICK-START-GUIDE.md`  
- **Button Issues**: `/WORKING/button-troubleshooting.md`
- **SD Card Fixes**: `/WORKING/SD-CARD-FIXES-SUMMARY.md`
- **Historical Reference**: `/WORKING/archive/`

## 🔧 **Hardware Configuration**

- **MCU**: ESP32-S3-mini
- **Audio**: Dual MAX9814 microphones (GPIO 9, 12)
- **Storage**: MicroSD card via SPI (GPIO 35-39)
- **UI**: Button (GPIO 4), LED (GPIO 40)
- **Communication**: USB Serial/JTAG

## ⚠️ **Important Notes**

- **Single Source of Truth**: `/WORKING/softwareV2/` is the stable codebase
- **Clean Repository**: Temp files automatically ignored via `.gitignore`
- **No Duplicates**: Historical docs archived, not in root directory
- **Always Document**: Update `/WORKING/README.md` with any changes

---
**Last Updated**: August 25, 2025  
**Maintainer**: Development Team
