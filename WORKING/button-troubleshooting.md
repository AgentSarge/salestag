# SalesTag Button Troubleshooting & Resolution

## 🚨 **Problem Summary**

**Issue**: The SalesTag Smart Badge button (SW2 on GPIO 4) appeared to be non-functional, showing stuck at GPIO level 0 even when physically unpressed.

**Initial Symptoms**:
- Button appeared "stuck" at GPIO 4 = 0
- No button press/release events detected
- System appeared unresponsive to button input

## 🔍 **Investigation Process**

### **Phase 1: Hardware Investigation**
- **Initial Diagnosis**: Suspected missing external pull-up resistor or hardware short circuit
- **Hardware Review**: 
  - Checked schematics, BOM, and PCB images
  - Confirmed GPIO 4 → SW2 → GND wiring
  - Verified 10kΩ resistors (R1-R4) in BOM
  - **Finding**: Hardware design was correct with internal pull-ups enabled

### **Phase 2: Software Investigation**
- **Historical Analysis**: Reviewed previous working firmware logs
  - **Key Discovery**: Button WAS working previously
  - **Evidence**: Logs showed `GPIO[4]| Pullup: 1` and successful button events
- **Revised Diagnosis**: Software timing issue, not hardware problem

### **Phase 3: Root Cause Identification**
- **Problem**: FreeRTOS task creation crash in simplified firmware
- **Error**: `Guru Meditation Error: Core 1 panic'ed (LoadProhibited)`
- **Location**: Task watchdog timer initialization
- **Timing**: Crash occurred immediately after `app_main()` started

## ✅ **Root Cause**

**The button issue was NOT hardware-related.** The problem was:

1. **Software Complexity**: The original firmware had complex task dependencies
2. **Task Creation Crash**: FreeRTOS task creation was failing during initialization
3. **Timing Issues**: Button polling was starting before GPIO stabilization
4. **Dependency Chain**: WiFi, web server, and other components were interfering

## 🛠️ **Solution Implemented**

### **Working Button Logic (from diagnostic build)**

```c
// Simple button polling with debouncing
static void button_poll_task(void *arg) {
    (void)arg;
    
    ESP_LOGI(TAG, "Button polling task started");
    ESP_LOGI(TAG, "GPIO[%d] initial level: %d", BTN_GPIO, gpio_get_level(BTN_GPIO));
    
    // Simple button polling with debouncing
    bool last_button_state = false;
    TickType_t last_change = xTaskGetTickCount();
    
    while (true) {
        // Read button state
        bool current_button_state = (gpio_get_level(BTN_GPIO) == 0);  // pressed = LOW
        
        // Check for state change
        if (current_button_state != last_button_state) {
            TickType_t now = xTaskGetTickCount();
            
            // Debounce
            if ((now - last_change) >= pdMS_TO_TICKS(DEBOUNCE_MS)) {
                last_button_state = current_button_state;
                last_change = now;
                
                if (current_button_state) {
                    handle_button_press();
                } else {
                    handle_button_release();
                }
            }
        }
        
        // Small delay to avoid busy waiting
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

### **Key Success Factors**

1. **Simple Task Structure**: Single button polling task with minimal dependencies
2. **Proper Debouncing**: 50ms debounce with timestamp-based logic
3. **GPIO Configuration**: Internal pull-up enabled (`GPIO_PULLUP_ENABLE`)
4. **Stable Timing**: 10ms polling interval with `vTaskDelay`
5. **No Complex Dependencies**: Eliminated WiFi, web server, and other components

## 🔧 **Hardware Configuration**

### **GPIO Setup**
```c
// Button GPIO configuration
gpio_config_t btn_config = {
    .pin_bit_mask = 1ULL << BTN_GPIO,        // GPIO 4
    .mode = GPIO_MODE_INPUT,                  // Input mode
    .pull_up_en = GPIO_PULLUP_ENABLE,        // Internal pull-up enabled
    .pull_down_en = GPIO_PULLDOWN_DISABLE,   // No external pull-down
    .intr_type = GPIO_INTR_DISABLE           // No interrupts
};
gpio_config(&btn_config);

// LED GPIO configuration  
gpio_config_t led_config = {
    .pin_bit_mask = 1ULL << LED_GPIO,        // GPIO 40
    .mode = GPIO_MODE_OUTPUT,                // Output mode
    .pull_up_en = GPIO_PULLUP_DISABLE,      // No pull-up for output
    .pull_down_en = GPIO_PULLDOWN_DISABLE,   // No pull-down
    .intr_type = GPIO_INTR_DISABLE           // No interrupts
};
gpio_config(&led_config);
```

### **Hardware Details**
- **Button**: SW2 (tactile switch) connected to GPIO 4
- **LED**: Status LED connected to GPIO 40
- **Pull-up**: Internal ESP32-S3 pull-up resistor (no external needed)
- **Wiring**: GPIO 4 → SW2 → GND (button press = LOW)

## 📊 **Testing Results**

### **Diagnostic Build Success**
```
I (306) salestag-simple: === SalesTag Simple Test - Button + LED Only ===
I (316) salestag-simple: BOOT: Starting simple button test...
I (316) gpio: GPIO[4]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0 
I (326) gpio: GPIO[40]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
I (336) salestag-simple: GPIO configured - Button: GPIO[4], LED: GPIO[40]
I (346) salestag-simple: Button initial level: 1
I (346) salestag-simple: === System Ready ===
I (356) salestag-simple: Press button to turn LED ON, release to turn OFF
I (8936) salestag-simple: BTN_DOWN - Button pressed
I (9156) salestag-simple: BTN_UP - Button released
I (10046) salestag-simple: BTN_DOWN - Button pressed
I (10246) salestag-simple: BTN_UP - Button released
```

### **Button Behavior**
- ✅ **Initial State**: GPIO 4 = 1 (unpressed, pull-up active)
- ✅ **Press Detection**: GPIO 4 = 0 (pressed, connected to GND)
- ✅ **Release Detection**: GPIO 4 = 1 (unpressed, pull-up active)
- ✅ **Debouncing**: Clean press/release events, no phantom triggers
- ✅ **LED Control**: ON when pressed, OFF when released

## 🚀 **Implementation in Main Firmware**

### **Changes Made**
1. **Replaced Complex Button Logic**: Used working button polling from diagnostic build
2. **Simplified Task Creation**: Single button polling task with minimal dependencies
3. **Maintained Functionality**: Kept recorder, SD card, and LED control features
4. **Improved Reliability**: Eliminated task creation crashes

### **File Updates**
- **`software_v1/main/main.c`**: Updated with working button logic
- **`diagnostic_build/main/main.c`**: Ultra-simple working version
- **`docs/button-troubleshooting.md`**: This documentation

## 📋 **Lessons Learned**

### **1. Hardware vs Software Diagnosis**
- **Initial Assumption**: Hardware problem (missing pull-up, short circuit)
- **Reality**: Software complexity and task creation issues
- **Lesson**: Always verify software before assuming hardware problems

### **2. Simplification Strategy**
- **Approach**: Start with simplest possible implementation
- **Result**: Identified root cause and created working solution
- **Lesson**: Complexity reduction often reveals underlying issues

### **3. Historical Analysis Value**
- **Action**: Reviewed previous working firmware logs
- **Discovery**: Button was working before, indicating software regression
- **Lesson**: Historical data is invaluable for troubleshooting

### **4. FreeRTOS Task Management**
- **Problem**: Task creation crashes during initialization
- **Solution**: Simple, single-purpose tasks with minimal dependencies
- **Lesson**: Complex task dependencies can cause system instability

## 🛡️ **Prevention Measures**

### **1. Code Review Guidelines**
- Keep button logic simple and independent
- Avoid complex task dependencies during initialization
- Test button functionality in isolation first

### **2. Testing Strategy**
- Always test button input before adding complex features
- Use diagnostic builds for component isolation
- Maintain working button implementation as reference

### **3. Documentation Requirements**
- Document button behavior and GPIO configuration
- Record any changes to button logic
- Maintain troubleshooting guides for future issues

## 🔮 **Future Improvements**

### **1. Button Enhancement**
- Add button press duration detection (short vs long press)
- Implement button press patterns (double-click, hold)
- Add haptic feedback if hardware supports it

### **2. System Robustness**
- Implement button watchdog for stuck button detection
- Add button state validation and error recovery
- Consider interrupt-based button handling for power efficiency

### **3. Testing Automation**
- Automated button functionality tests
- Button stress testing (rapid presses, long holds)
- Integration testing with all system components

## 📞 **Support Information**

### **Files Modified**
- `software_v1/main/main.c` - Main firmware with working button
- `diagnostic_build/main/main.c` - Simple working reference
- `docs/button-troubleshooting.md` - This documentation

### **Key Contributors**
- **Issue Identification**: User reported non-functional button
- **Investigation**: Systematic hardware and software analysis
- **Solution**: Ultra-simple diagnostic build approach
- **Implementation**: Integration of working logic into main firmware

### **Related Documentation**
- `docs/architecture.md` - System architecture overview
- `docs/hardware/README.md` - Hardware specifications
- `device_hardware_info/` - Schematics and BOM

---

**Status**: ✅ **RESOLVED** - Button functionality fully restored  
**Date**: August 20, 2025  
**Version**: 1.0  
**Next Review**: After next firmware update
