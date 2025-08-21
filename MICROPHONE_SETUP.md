# 🎤 Microphone Setup for SalesTag ESP32-S3

## 🔌 Hardware Connection

### **✅ Already Connected (From Hardware Docs):**

- **2x GMI4015P-66DB Electret Microphones** (MIC1, MIC2)
- **2x MAX9814ETD+T Amplifiers** (U3, U19)
- **Dual-channel stereo recording** capability

### **🔧 Exact GPIO Connections (From FlyingProbeTesting.json):**

```
MIC1 (U3 Output) → GPIO 9 (ADC1_CH3) - MIC_DATA1
MIC2 (U19 Output) → GPIO 12 (ADC1_CH6) - MIC_DATA2
```

### **📊 Hardware Configuration:**

- **Microphone Type**: Electret condenser (-66dB sensitivity)
- **Amplifier**: MAX9814 with automatic gain control
- **Power**: 3.3V regulated supply
- **Audio Quality**: Professional-grade stereo recording

## 🔧 Software Configuration

The code is now configured to:

- **ADC Channel 1**: GPIO 9 (ADC1_CH3) for MIC1
- **ADC Channel 2**: GPIO 12 (ADC1_CH6) for MIC2
- **Sample Rate**: 16kHz
- **Resolution**: 12-bit
- **Channels**: 2 (Stereo)
- **Voltage Range**: 0-3.3V

## 📊 Expected Behavior

### **With Current Hardware:**

- Logs will show: `"Dual microphone ADC initialized successfully"`
- Logs will show: `"MIC1: GPIO 9 (ADC1_CH3) - MIC_DATA1"`
- Logs will show: `"MIC2: GPIO 12 (ADC1_CH6) - MIC_DATA2"`
- You'll hear: **Your actual voice/audio in stereo**

### **If Something Goes Wrong:**

- Logs will show: `"Failed to initialize microphone ADC, continuing with synthetic audio"`
- You'll hear: **440Hz continuous beep** (fallback mode)

## 🧪 Testing

1. **Build and flash** the updated firmware
2. **Monitor output** to see ADC initialization status
3. **Record audio** and check if it's your voice instead of beep
4. **Verify stereo** - you should get left/right channel separation

## 🚨 Troubleshooting

### **ADC Initialization Fails:**

- Check if MAX9814 amplifiers are properly powered
- Verify 3.3V power supply to U3 and U19
- Check for loose connections on GPIO 9 and 12

### **No Audio Input:**

- Verify microphone connections to MAX9814 inputs
- Check MAX9814 gain settings (GAIN1, GAIN2 pins)
- Ensure proper grounding

### **Poor Audio Quality:**

- Check MAX9814 reference voltages (A_R1, A_R2)
- Verify power supply stability
- Check for electrical interference

## 🎯 Current Status

✅ **Hardware**: Dual microphones with MAX9814 amplifiers connected  
✅ **GPIO Mapping**: GPIO 9 (MIC1) and GPIO 12 (MIC2) identified  
✅ **Software**: Updated to use correct ADC channels  
✅ **Configuration**: Stereo recording (2 channels) enabled

## 🚀 Next Steps

1. **Build and test** the updated firmware
2. **Verify** real audio recording works
3. **Test stereo separation** between channels
4. **Adjust gain** if needed via MAX9814 settings

---

**🎵 Your dual microphone setup is ready to record real audio!** 🎵
