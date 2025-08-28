# SalesTag Audio Recording System v1.0

This is the complete audio recording system implementing PRD Phase 1 functionality with real microphone capture, 10-second recording sessions, and SD card storage.

## What's Included

- **Real Audio Capture**: Dual MAX9814 microphones via ESP32-S3 ADC (GPIO 9, GPIO 12)
- **Button Interface**: GPIO 4 with debouncing - one-touch recording per PRD FR1
- **LED Status**: GPIO 40 for recording state indication per PRD FR5
- **SD Card Storage**: WAV files with metadata per PRD FR2
- **10-Second Auto-Stop**: Automatic recording termination per PRD FR1
- **Complete State Machine**: Idle → Recording → Stopping → Idle cycle
- **Audio Format**: 16kHz, 16-bit, stereo PCM per PRD NFR2

## What's Removed (vs Full PRD Vision)

- BLE communication stack (Phase 3)
- Mobile companion app (Phase 4)
- WiFi connectivity (intentionally excluded)
- Power management optimization (Phase 1.3)
- File encryption (Phase 2.2)

## Build Instructions

```bash
cd diagnostic_build
idf.py build
idf.py flash monitor
```

## Button Behavior

1. **Single Press**: Start recording (LED ON)
2. **Second Press**: Stop recording (LED OFF)
3. **Without SD Card**: LED toggle only
4. **System Not Ready**: Button ignored with warning logs

## Diagnostic Features

- Button state monitoring
- LED functionality test
- GPIO level reporting
- System health heartbeat
- Comprehensive logging

## Expected Output

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
Audio format: 16kHz, 16-bit, 2 channels (stereo)

[Button Press]
Button pressed - current state: 0
Starting 10-second recording session #1
Recording started - auto-stop in 10 seconds
[10 seconds later]
10-second recording timeout reached - stopping automatically
Recording stopped automatically after 10 seconds
Recording session #1 complete: 320000 bytes, 10000 ms
```

## Troubleshooting

- **Button not responding**: Check GPIO 4 connection and pull-up resistor
- **LED not working**: Verify GPIO 40 connection and power
- **SD card issues**: Check SPI connections (CS:39, MOSI:35, MISO:37, SCLK:36)
- **Build errors**: Ensure ESP-IDF v5.x is properly configured
