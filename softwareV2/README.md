# SalesTag Diagnostic Build - Simplified (No WiFi)

This is a simplified diagnostic build that focuses on core button functionality without WiFi or web server components.

## What's Included

- **Button Input**: GPIO 4 with debouncing (50ms)
- **LED Output**: GPIO 40 for status indication
- **SD Card Storage**: Basic recording functionality
- **Audio Recording**: 16kHz, 16-bit, stereo WAV files
- **Simple State Machine**: Record/Stop with LED feedback

## What's Removed

- WiFi manager
- Web server
- HTTP dependencies
- Network stack components
- Complex provisioning logic

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
=== SalesTag Diagnostic Build - No WiFi ===
BOOT: Initializing system...
NVS initialized
Initializing UI (button + LED)...
UI initialized - button callback registered
Initializing SD card storage...
=== System Initialization Complete ===
System ready: YES
Recorder ready: YES
SD card ready: YES
=== Button Diagnostic Test ===
Testing LED...
LED test complete
Button GPIO[4] level: 1 (1=unpressed, 0=pressed)
Button reading test (10 samples):
  Sample  1: GPIO[4] = 1
  Sample  2: GPIO[4] = 1
  ...
=== Button Diagnostic Complete ===
=== Ready for button input ===
Press button to start/stop recording or toggle LED
```

## Troubleshooting

- **Button not responding**: Check GPIO 4 connection and pull-up resistor
- **LED not working**: Verify GPIO 40 connection and power
- **SD card issues**: Check SPI connections (CS:39, MOSI:35, MISO:37, SCLK:36)
- **Build errors**: Ensure ESP-IDF v5.x is properly configured
