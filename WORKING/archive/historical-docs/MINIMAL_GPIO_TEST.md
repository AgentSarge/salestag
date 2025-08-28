# SalesTag Minimal GPIO Test

## 🔧 **Current Build: BASIC GPIO ONLY**

**Status**: ✅ Build successful - 236KB firmware (77% flash free)  
**Components**: Only basic GPIO driver + FreeRTOS  
**Purpose**: Validate basic button → LED functionality before adding complexity

## 🧪 **Test Plan: Component by Component**

### **Step 1: Basic GPIO Test (Current)**
**What to Test**: 
- Flash minimal firmware
- Verify button press → LED ON
- Verify button release → LED OFF
- Check serial output

**Expected Serial Output**:
```
=== SalesTag Simple GPIO Test ===
BOOT: Testing basic button + LED functionality...
GPIO configured successfully:
  Button: GPIO[4] (pullup enabled)
  LED: GPIO[40] (output mode)
Button initial level: 1 (1=unpressed, 0=pressed)
=== System Ready ===
Press button to turn LED ON, release to turn LED OFF
Monitoring button state every 10ms...

[When button pressed]
Button state changed: PRESSED
LED turned ON

[When button released]
Button state changed: RELEASED
LED turned OFF
```

**Hardware Requirements**:
- ESP32-S3 development board
- Button between GPIO 4 and GND (ESP32-S3 internal pullup handles this)
- LED + 220Ω resistor between GPIO 40 and GND

**Success Criteria**:
- [ ] System boots without errors
- [ ] Button press immediately turns LED ON  
- [ ] Button release immediately turns LED OFF
- [ ] Serial output shows button state changes
- [ ] No crashes or resets

## 🔄 **Component Addition Plan (After GPIO Works)**

### **Step 2: Add UI Module**
- Integrate `ui.c` with proper debouncing
- Test callback-based button handling
- Verify LED control through ui module

### **Step 3: Add SD Card Storage**
- Add `sd_storage.c` and dependencies
- Test SD card mounting and file creation
- No audio yet - just file system validation

### **Step 4: Add Audio Foundation**
- Add `wav_writer.c` for file format
- Add `audio_capture.c` (still stubbed)
- Test WAV file creation with dummy data

### **Step 5: Add Real Audio**
- Add `recorder.c` with ADC microphone capture
- Test real audio recording to WAV files
- Validate 10-second recording cycles

### **Step 6: Integration Testing**
- Complete button → record → save → LED workflow
- Multiple recording sessions
- Full system validation

## 🚀 **Current Flash Commands**

```bash
# Navigate to project
cd /Users/self/Desktop/salestag/softwareV2

# Source ESP-IDF environment
. ~/esp/esp-idf/export.sh

# Flash minimal firmware
idf.py flash monitor
```

## ⚠️ **Important Notes**

### **ESP32-S3 Pullup Handling**
The current implementation uses:
```c
.pull_up_en = GPIO_PULLUP_ENABLE
```
This should properly handle the button input without external pullup resistors.

### **Button Logic**
- **GPIO 4 = 1**: Button not pressed (pullup pulls high)
- **GPIO 4 = 0**: Button pressed (pulls to ground)

### **LED Logic**
- **GPIO 40 = 1**: LED ON
- **GPIO 40 = 0**: LED OFF

## 🎯 **If GPIO Test Fails**

### **Troubleshooting Steps**:
1. **Check Serial Output**: Look for GPIO configuration errors
2. **Verify Wiring**: Button between GPIO 4 and GND, LED+resistor between GPIO 40 and GND
3. **Test LED Manually**: Modify code to force LED ON at startup
4. **Test Button Manually**: Check button GPIO level in serial monitor
5. **Hardware Issues**: Try different GPIO pins if needed

### **Debugging Modifications**:
```c
// Add to app_main() for testing:
ESP_LOGI(TAG, "Forcing LED ON for 2 seconds...");
gpio_set_level(LED_GPIO, 1);
vTaskDelay(pdMS_TO_TICKS(2000));
gpio_set_level(LED_GPIO, 0);
ESP_LOGI(TAG, "LED test complete");
```

## 📋 **Current Status**

- ✅ **Build Environment**: ESP-IDF v5.2.2 working
- ✅ **Firmware Size**: 236KB (reasonable for minimal test)
- ✅ **GPIO Configuration**: Button + LED setup complete
- ⏳ **Hardware Test**: Ready for physical validation

**Next**: Flash the minimal firmware and test basic GPIO functionality before adding any other components.

---
**Goal**: Prove GPIO basics work, then systematically add one component at a time until full SalesTag functionality is achieved.