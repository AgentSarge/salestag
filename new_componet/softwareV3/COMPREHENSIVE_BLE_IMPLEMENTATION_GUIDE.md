# Comprehensive BLE Implementation Guide for SalesTag ESP32-S3

## Table of Contents
1. [Overview](#overview)
2. [Device Specifications](#device-specifications)
3. [BLE Services and Characteristics](#ble-services-and-characteristics)
4. [Dynamic BLE State Management](#dynamic-ble-state-management)
5. [File Transfer Protocol](#file-transfer-protocol)
6. [Mobile App Developer Guide](#mobile-app-developer-guide)
7. [Troubleshooting Guide](#troubleshooting-guide)
8. [Testing and Validation](#testing-and-validation)
9. [Future Enhancements](#future-enhancements)

## Overview

The SalesTag ESP32-S3 device implements a comprehensive BLE (Bluetooth Low Energy) system that provides:
- **Dynamic BLE State Management**: Automatic BLE advertising control based on recording status
- **File Transfer Protocol**: Bidirectional file transfer capabilities via BLE
- **Audio Recording Control**: Remote control of audio recording via BLE
- **Status Monitoring**: Real-time device status and file information

This document serves as the complete reference for both firmware developers and mobile app developers working with the SalesTag device.

## Device Specifications

### Hardware
- **Chip**: ESP32-S3 (QFN56, revision v0.2)
- **Features**: WiFi, BLE, Embedded Flash 8MB (GD)
- **Crystal**: 40MHz
- **USB Mode**: USB-Serial/JTAG
- **MAC Address**: b4:3a:45:35:aa:ec
- **Button**: GPIO 4 (with pullup, 50ms debounce)
- **LED**: GPIO 40 (output mode)
- **Microphones**: GPIO 9 (MIC1, ADC1_CH3), GPIO 12 (MIC2, ADC1_CH6)

### BLE Stack
- **Framework**: NimBLE (Apache Mynewt)
- **Version**: ESP-IDF v5.2.2
- **Connection Type**: Peripheral device
- **Advertising Mode**: General discoverable, undirected connectable
- **Max Connections**: 3
- **Max Bonds**: 3
- **Max CCCDs**: 8
- **Host Task Stack Size**: 4096 bytes
- **Pinned to Core**: 0
- **Log Level**: INFO

### Device Identification
- **Device Name**: `ESP32-S3-Mini-BLE`
- **Advertising Name**: `ESP32-S3-Mini-BLE`
- **Name Length**: 18 characters
- **Advertising Flags**: General discoverable mode only

## BLE Services and Characteristics

### 1. Audio Recording Service (UUID: 0x1234)

#### Characteristics:

##### Record Control (UUID: 0x1235)
- **Properties**: Read, Write
- **Purpose**: Control audio recording operations
- **Commands**:
  - `0x01` - Start recording
  - `0x00` - Stop recording

##### Status (UUID: 0x1236)
- **Properties**: Read, Notify
- **Purpose**: Report device status
- **Data Structure**:
  ```c
  struct {
      uint8_t audio_enabled;    // 1 = enabled, 0 = disabled
      uint8_t sd_available;     // 1 = available, 0 = unavailable
      uint8_t recording;        // 1 = recording, 0 = not recording
      uint32_t total_files;     // Total number of recorded files
  } status;
  ```

##### File Count (UUID: 0x1237)
- **Properties**: Read
- **Purpose**: Get total number of recorded files
- **Data**: 32-bit integer (uint32_t)

### 2. File Transfer Service (UUID: 0x1240)

#### Characteristics:

##### File Control (UUID: 0x1241)
- **Properties**: Read, Write
- **Purpose**: Control file transfer operations
- **Read**: Returns current transfer status (0 = IDLE, 1 = ACTIVE)
- **Commands**:
  - `0x01` - FILE_TRANSFER_CMD_START: Start new file transfer (uses default filename "ble_transfer.dat" and size 1024 bytes)
  - `0x02` - FILE_TRANSFER_CMD_STOP: Complete current file transfer
  - `0x03` - FILE_TRANSFER_CMD_ABORT: Abort current file transfer

##### File Data (UUID: 0x1242)
- **Properties**: Read, Write
- **Purpose**: Transfer file data in chunks
- **Max Chunk Size**: 512 bytes
- **Data**: Raw file data bytes
- **Note**: Only accepts writes when transfer is active

##### File Status (UUID: 0x1243)
- **Properties**: Read, Notify
- **Purpose**: Report transfer status and progress
- **Status Values**:
  - `0x00` - FILE_TRANSFER_STATUS_IDLE: No transfer active
  - `0x01` - FILE_TRANSFER_STATUS_ACTIVE: Transfer in progress
  - `0x02` - FILE_TRANSFER_STATUS_COMPLETE: Transfer completed successfully
  - `0x03` - FILE_TRANSFER_STATUS_ERROR: Transfer failed or aborted
- **Data Structure**:
  ```c
  struct {
      uint8_t status;    // Transfer status
      uint32_t offset;   // Current transfer offset in bytes
      uint32_t size;     // Total file size in bytes
  } status_notification;
  ```

## Dynamic BLE State Management

### Implementation Status
**Current Implementation**: Dynamic BLE state management is fully implemented and functional
- ✅ Automatic BLE advertising stop during recording
- ✅ Automatic BLE advertising restart after recording
- ✅ Error recovery and state management
- ✅ Integration with both button and BLE commands
- ✅ Comprehensive logging and debugging

### Overview
The device implements intelligent BLE state management that automatically stops BLE advertising during audio recording to prevent RF interference and ensures clean audio capture.

### Implementation Details

#### Global State Variable
```c
static volatile bool s_is_recording = false;  // Made volatile for multi-task access
```

The `s_is_recording` variable is marked as `volatile` because it's accessed by multiple tasks (button handling, BLE callbacks, and main application logic).

#### Control Functions

##### `ble_stop_advertising()`
- Stops BLE advertising to prevent audio interference
- Called when recording starts
- Logs: `"Stopping BLE advertising to prevent audio interference"`
- Logs: `"BLE advertising stopped successfully"` on success or error code on failure

##### `ble_start_advertising_if_not_recording()`
- Conditionally starts BLE advertising only if not currently recording
- Prevents advertising restart during active recording
- Logs: `"Starting BLE advertising (not currently recording)"` or `"Skipping BLE advertising start (currently recording)"`

### Behavior Flow

#### Recording Start
1. **Stop BLE advertising** to prevent interference
2. Start audio capture and storage
3. Set `s_is_recording = true`
4. If recording fails, restart BLE advertising

#### Recording Stop
1. Stop audio capture and storage
2. Set `s_is_recording = false`
3. **Restart BLE advertising** now that recording is finished

#### BLE Callback Behavior
- **`ble_app_on_sync()`**: Only starts advertising if not recording
- **`ble_app_on_disconnect()`**: Only restarts advertising if not recording
- **`ble_app_on_adv_complete()`**: Only restarts advertising if not recording

### Benefits
1. **Clean Audio Signal**: BLE advertising is disabled during recording to prevent RF interference
2. **Predictable Behavior**: Device is only discoverable when not recording
3. **Automatic Management**: No manual intervention required - system handles BLE state automatically
4. **Error Recovery**: BLE advertising is restored if recording operations fail

### Log Messages

The implementation includes comprehensive logging:

- `"Stopping BLE advertising to prevent audio interference"`
- `"Starting BLE advertising (not currently recording)"`
- `"Skipping BLE advertising start (currently recording)"`
- `"BLE advertising stopped successfully"`
- `"Restart BLE advertising since recording failed"`
- `"BLE: Recording started via BLE command"`
- `"BLE: Recording stopped via BLE command"`
- `"BLE: Failed to start audio capture"`
- `"BLE: Failed to start recording storage"`
- `"BLE: Failed to stop recording"`

## File Transfer Protocol

### Implementation Status
**Current Implementation**: Basic file transfer protocol is fully implemented and functional
- ✅ File transfer start/stop/abort commands
- ✅ Data transfer in chunks (max 512 bytes)
- ✅ Progress notifications every 1KB
- ✅ Error handling and state management
- ⚠️ **Limitation**: Uses fixed filename "ble_transfer.dat" and size 1024 bytes
- 🔄 **Future**: Dynamic filename and size support planned

### Protocol Flow

#### 1. Starting a File Transfer
1. Client writes `0x01` to File Control characteristic
2. Server creates new file on SD card (default: `ble_transfer.dat` with size 1024 bytes)
3. Server sends status notification: `ACTIVE` with offset=0, size=1024
4. Server ready to receive data
5. **Note**: Current implementation uses fixed filename and size - dynamic filename/size not yet implemented

#### 2. Transferring File Data
1. Client writes file data to File Data characteristic in chunks (max 512 bytes)
2. Server writes data to SD card file
3. Server tracks transfer progress (offset)
4. Server sends progress notifications every 1KB transferred

#### 3. Completing a File Transfer
1. Client writes `0x02` to File Control characteristic
2. Server closes the file
3. Server sends status notification: `COMPLETE` with final offset and size

#### 4. Aborting a File Transfer
1. Client writes `0x03` to File Control characteristic
2. Server closes the file and deletes it
3. Server sends status notification: `ERROR` with current offset and size

### State Management
```c
static uint16_t s_file_transfer_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static uint16_t s_file_transfer_status_handle = 0;
static bool s_file_transfer_active = false;
static uint32_t s_file_transfer_size = 0;
static uint32_t s_file_transfer_offset = 0;
static char s_file_transfer_filename[128] = {0};
static FILE* s_file_transfer_fp = NULL;
```

### Key Functions

#### `file_transfer_start(const char* filename, uint32_t file_size)`
- Validates SD card availability
- Creates file on SD card with specified filename and size
- Initializes transfer state
- Sends initial status notification

#### `file_transfer_write_data(const uint8_t* data, uint16_t len)`
- Writes data chunks to SD card file
- Tracks transfer progress
- Sends progress notifications every 1KB transferred

#### `file_transfer_stop()`
- Closes the file
- Sends completion notification
- Resets transfer state

#### `file_transfer_abort()`
- Closes and deletes the file
- Sends error notification
- Resets transfer state

#### `file_transfer_notify_status(uint8_t status, uint32_t offset, uint32_t size)`
- Sends status notifications to connected client
- Includes current offset and total size

### Error Handling
- **SD Card Unavailable**: Returns error if SD card not mounted
- **File Creation Failure**: Returns error if file cannot be created
- **Write Failures**: Returns error if data cannot be written
- **Connection Loss**: Automatically aborts transfer if BLE connection lost
- **Transfer Already Active**: Returns error if trying to start transfer when one is already active
- **Transfer Not Active**: Returns error if trying to write data or stop transfer when none is active

## Mobile App Developer Guide

### Device Discovery

#### Scan Parameters
- **Device Name**: `ESP32-S3-Mini-BLE`
- **Service UUIDs**: 
  - Audio Service: `0x1234`
  - File Transfer Service: `0x1240`
- **Scan Duration**: 10-30 seconds recommended
- **Scan Mode**: Low latency mode for faster discovery

#### Expected RSSI Values
- **Good Signal**: -30 to -60 dBm
- **Acceptable Signal**: -60 to -70 dBm
- **Poor Signal**: Below -70 dBm (may have connection issues)

### Connection Management

#### Connection Parameters
- **Connection Interval**: 7.5ms to 4s (negotiated)
- **Slave Latency**: 0-499 (negotiated)
- **Supervision Timeout**: 100ms to 32s (negotiated)

#### Connection Lifecycle
1. **Scan** for device with name `ESP32-S3-Mini-BLE`
2. **Connect** to discovered device
3. **Discover Services** and characteristics
4. **Subscribe** to notification characteristics
5. **Interact** with device via read/write operations
6. **Disconnect** when finished

### Service Discovery

#### Required Services
```javascript
// Example service discovery (JavaScript/React Native)
const audioService = await device.services.find(s => s.uuid === '1234');
const fileTransferService = await device.services.find(s => s.uuid === '1240');

// Audio Service Characteristics
const recordControl = await audioService.characteristics.find(c => c.uuid === '1235');
const status = await audioService.characteristics.find(c => c.uuid === '1236');
const fileCount = await audioService.characteristics.find(c => c.uuid === '1237');

// File Transfer Service Characteristics
const fileControl = await fileTransferService.characteristics.find(c => c.uuid === '1241');
const fileData = await fileTransferService.characteristics.find(c => c.uuid === '1242');
const fileStatus = await fileTransferService.characteristics.find(c => c.uuid === '1243');
```

### Audio Recording Control

#### Start Recording
```javascript
// Write 0x01 to Record Control characteristic
await recordControl.writeValue(new Uint8Array([0x01]));
```

#### Stop Recording
```javascript
// Write 0x00 to Record Control characteristic
await recordControl.writeValue(new Uint8Array([0x00]));
```

#### Audio Configuration
- **Sample Rate**: 1kHz (FreeRTOS tick-limited)
- **Channels**: 2 (stereo)
- **ADC Resolution**: 12-bit
- **ADC Attenuation**: 12dB
- **File Format**: Raw ADC samples (.raw files)
- **Storage**: Direct to SD card via raw_audio_storage module

#### Read Status
```javascript
// Read device status
const statusData = await status.readValue();
const statusView = new DataView(statusData.buffer);
const deviceStatus = {
    audioEnabled: statusView.getUint8(0),
    sdAvailable: statusView.getUint8(1),
    recording: statusView.getUint8(2),
    totalFiles: statusView.getUint32(3, true) // little-endian
};
```

#### Subscribe to Status Notifications
```javascript
// Subscribe to status notifications
await status.startNotifications();
status.addEventListener('characteristicvaluechanged', (event) => {
    const value = event.target.value;
    const view = new DataView(value.buffer);
    const status = {
        audioEnabled: view.getUint8(0),
        sdAvailable: view.getUint8(1),
        recording: view.getUint8(2),
        totalFiles: view.getUint32(3, true)
    };
    console.log('Device status updated:', status);
});
```

### File Transfer Operations

#### Start File Transfer
```javascript
// Start a new file transfer (uses default filename "ble_transfer.dat" and size 1024 bytes)
await fileControl.writeValue(new Uint8Array([0x01])); // START command

// Note: Current implementation uses fixed filename and size
// Future versions will support dynamic filename and size specification
```

#### Send File Data
```javascript
// Send file data in chunks (max 512 bytes)
const chunkSize = 512;
for (let offset = 0; offset < fileData.length; offset += chunkSize) {
    const chunk = fileData.slice(offset, offset + chunkSize);
    await fileData.writeValue(chunk);
    // Wait for acknowledgment or progress notification
}
```

#### Complete File Transfer
```javascript
// Complete the file transfer
await fileControl.writeValue(new Uint8Array([0x02])); // STOP command
```

#### Abort File Transfer
```javascript
// Abort the file transfer
await fileControl.writeValue(new Uint8Array([0x03])); // ABORT command
```

#### Monitor Transfer Progress
```javascript
// Subscribe to file status notifications
await fileStatus.startNotifications();
fileStatus.addEventListener('characteristicvaluechanged', (event) => {
    const value = event.target.value;
    const view = new DataView(value.buffer);
    const transferStatus = {
        status: view.getUint8(0),
        offset: view.getUint32(1, true),
        size: view.getUint32(5, true)
    };
    
    switch (transferStatus.status) {
        case 0x00: // IDLE
            console.log('Transfer idle');
            break;
        case 0x01: // ACTIVE
            console.log(`Transfer active: ${transferStatus.offset}/${transferStatus.size} bytes`);
            break;
        case 0x02: // COMPLETE
            console.log('Transfer completed successfully');
            break;
        case 0x03: // ERROR
            console.log('Transfer failed or aborted');
            break;
    }
});
```

### Error Handling

#### Connection Errors
```javascript
device.addEventListener('gattserverdisconnected', (event) => {
    console.log('Device disconnected:', event.reason);
    // Implement reconnection logic
});

device.addEventListener('gattserverdisconnected', (event) => {
    console.log('Device disconnected:', event.reason);
    // Implement reconnection logic
});
```

#### Characteristic Access Errors
```javascript
try {
    await characteristic.writeValue(data);
} catch (error) {
    console.error('Write failed:', error);
    // Handle specific error codes
    if (error.code === 0x80) {
        console.log('Characteristic not found');
    } else if (error.code === 0x81) {
        console.log('Characteristic not writable');
    }
}
```

### Platform-Specific Considerations

#### Android
- **Permissions**: `BLUETOOTH_SCAN`, `BLUETOOTH_CONNECT`, `ACCESS_FINE_LOCATION`
- **API Level**: Minimum API 21 (Android 5.0)
- **Scanning**: Use `BluetoothLeScanner` for device discovery

#### iOS
- **Permissions**: `NSBluetoothAlwaysUsageDescription` in Info.plist
- **API**: Core Bluetooth framework
- **Background Mode**: Enable "Uses Bluetooth LE accessories" for background operation

#### React Native
- **Library**: `react-native-ble-plx` or `@config-plugins/react-native-ble-plx`
- **Permissions**: Configure in `android/app/src/main/AndroidManifest.xml` and `ios/Info.plist`

## Troubleshooting Guide

### Historical Issues (Resolved)
**Previous Problem**: The ESP32-S3 device was not appearing in BLE scanner apps despite reaching the advertising stage.

**Root Causes Identified and Fixed**:
1. **Conditional Logic**: "BLE functionality removed - device operates in standalone mode" - This has been resolved
2. **Advertising Flags**: Conflicting `BLE_HS_ADV_F_DISC_LTD` flag removed, now using only `BLE_HS_ADV_F_DISC_GEN`
3. **Advertising Parameters**: All parameters now correctly configured for general discoverable mode

**Current Status**: All known BLE advertising issues have been resolved and the device should be discoverable when not recording.

### Common Issues

#### Issue 1: Device Not Appearing in Scanner
**Symptoms**: No device visible in any scanner app
**Causes**:
- Advertising not started
- Wrong advertising parameters
- Hardware issues
- Device is currently recording (BLE advertising stopped)
- Conditional logic disabling BLE (historical issue - now resolved)

**Solutions**:
1. Check device logs for "Advertising started successfully"
2. Verify device is not currently recording
3. Check advertising parameters
4. Test with different scanner apps
5. Use `test_ble.sh` script for automated testing

#### Issue 2: Device Appears but Can't Connect
**Symptoms**: Device visible but connection fails
**Causes**:
- GATT service registration issues
- Connection parameters mismatch
- Device in recording mode

**Solutions**:
1. Check GATT service registration logs
2. Verify connection parameters
3. Ensure device is not recording
4. Test with nRF Connect app

#### Issue 3: Intermittent Visibility
**Symptoms**: Device appears/disappears randomly
**Causes**:
- Dynamic BLE state management (recording state changes)
- Advertising restart issues
- Power management
- Interference
- Advertising completion events

**Solutions**:
1. Check recording state - device won't advertise while recording
2. Monitor advertising restart logic
3. Check power consumption
4. Test in different environments
5. Verify advertising parameters are correct

#### Issue 4: File Transfer Fails
**Symptoms**: File transfer starts but fails or hangs
**Causes**:
- SD card issues
- Connection instability
- Chunk size too large (max 512 bytes)
- Memory constraints
- Transfer already active
- File creation failure
- SD card power cycling issues (known limitation)

**Solutions**:
1. Verify SD card is mounted and has space
2. Use smaller chunk sizes (256 bytes recommended)
3. Check connection stability
4. Monitor device memory usage
5. Ensure no other transfer is active
6. Check file permissions on SD card
7. Avoid SD card power cycling during operations (causes crashes)

### Debug Commands

#### Check BLE Status
```bash
# Monitor device logs
idf.py monitor

# Look for key messages:
# "BLE Host Stack Synced: YES"
# "Advertising started successfully"
# "Stopping BLE advertising to prevent audio interference"
```

#### Automated Testing
```bash
# Use the provided test script
cd new_componet/softwareV3
./test_ble.sh

# This script will:
# 1. Build the project
# 2. Flash the device
# 3. Start monitoring for BLE initialization
```

#### Test with Scanner Apps
- **nRF Connect** (Android/iOS) - Most comprehensive
- **BLE Scanner** (Android) - Simple and reliable
- **LightBlue** (iOS) - Good for debugging
- **Bluetooth Scanner** (Windows) - Desktop testing

#### Check Device Visibility
- Ensure device is within 10 meters
- Check for interference from other BLE devices
- Verify scanner app has location permissions (Android)
- Check RSSI values (should be > -70 dBm)

#### Verify Hardware
```bash
# Check GPIO configuration
# Verify antenna connection
# Test with known working BLE device
# Check power supply stability
```

### Expected Log Output

#### Successful BLE Operation
```
I (xxx) salestag-sd: Initializing NimBLE host stack...
I (xxx) salestag-sd: NimBLE device name set to: ESP32-S3-Mini-BLE
I (xxx) salestag-sd: NimBLE host stack started successfully
I (xxx) salestag-sd: BLE Host Stack is synchronized.
I (xxx) salestag-sd: Setting advertising data - name: 'ESP32-S3-Mini-BLE' (len: 18)
I (xxx) salestag-sd: Scan response data set successfully
I (xxx) salestag-sd: Starting advertising with parameters:
I (xxx) salestag-sd: Advertising started successfully!
I (xxx) salestag-sd: BLE Host Stack Synced: YES
I (xxx) salestag-sd: Advertising Status: SHOULD BE ACTIVE
```

#### Recording State Changes
```
I (xxx) salestag-sd: 🎤 Starting audio recording: /sdcard/r001.raw
I (xxx) salestag-sd: Stopping BLE advertising to prevent audio interference
I (xxx) salestag-sd: BLE advertising stopped successfully
I (xxx) salestag-sd: ✅ Recording started successfully

I (xxx) salestag-sd: ⏹️ Stopping audio recording...
I (xxx) salestag-sd: ✅ Recording stopped: /sdcard/r001.raw
I (xxx) salestag-sd: Starting BLE advertising (not currently recording)
I (xxx) salestag-sd: Advertising started successfully!
```

## Testing and Validation

### Automated Testing

#### Build and Flash
```bash
cd new_componet/softwareV3
source ~/esp/esp-idf/export.sh
idf.py build
idf.py flash
```

#### Use Test Script
```bash
# Automated testing script
cd new_componet/softwareV3
./test_ble.sh

# This script provides:
# - Automated build and flash
# - Monitoring with key message highlighting
# - Step-by-step testing guidance
```

#### Build Dependencies
The project requires the following ESP-IDF components:
- `driver` - GPIO and ADC drivers
- `fatfs` - File system for SD card
- `sdmmc` - SD card interface
- `esp_adc` - ADC functionality
- `esp_timer` - Timer functionality
- `nvs_flash` - Non-volatile storage
- `bt` - Bluetooth stack (NimBLE)

#### Monitor Device
```bash
idf.py monitor
```

### Manual Testing Checklist

#### BLE Discovery
- [ ] Device appears in scanner apps
- [ ] Device name shows as "ESP32-S3-Mini-BLE"
- [ ] RSSI values are reasonable (-30 to -70 dBm)
- [ ] Device can be connected to
- [ ] Device is within 10 meters range
- [ ] No interference from other BLE devices

#### Audio Recording Control
- [ ] Recording starts via button press
- [ ] Recording starts via BLE command
- [ ] Recording stops via button press
- [ ] Recording stops via BLE command
- [ ] BLE advertising stops during recording
- [ ] BLE advertising restarts after recording

#### File Transfer
- [ ] File transfer starts successfully (creates "ble_transfer.dat")
- [ ] Data can be sent in chunks (max 512 bytes)
- [ ] Progress notifications are received every 1KB
- [ ] File transfer completes successfully
- [ ] File transfer can be aborted (deletes partial file)
- [ ] Files are saved to SD card correctly
- [ ] Transfer state is properly managed (no duplicate transfers)

#### Error Scenarios
- [ ] SD card removal during recording
- [ ] SD card removal during file transfer
- [ ] BLE disconnection during operations
- [ ] Power cycling during operations
- [ ] Memory exhaustion scenarios
- [ ] Long button press (3s) - SD card power cycle (DISABLED - causes crashes)
- [ ] GPIO button stuck LOW (hardware issue detection)

### Performance Testing

#### Connection Stability
- **Test Duration**: 1 hour continuous connection
- **Expected Result**: No disconnections or data loss
- **Metrics**: Connection uptime, data transfer reliability

#### File Transfer Performance
- **Test File Sizes**: 1KB, 10KB, 100KB, 1MB
- **Expected Throughput**: 1-5 KB/s (BLE limitations)
- **Metrics**: Transfer time, success rate, error rate

#### Audio Recording Quality
- **Test Duration**: 5 minutes continuous recording
- **Expected Result**: Clean audio without BLE interference
- **Metrics**: Audio quality, file size, recording stability

## Future Enhancements

### Planned Features

#### 1. Enhanced Security
- **BLE Pairing and Bonding**: Implement secure pairing
- **File Transfer Encryption**: Add optional encryption
- **Access Control**: Implement user authentication
- **Secure Boot**: Verify firmware integrity

#### 2. Advanced File Management
- **Dynamic Filename Support**: Allow custom filenames (currently uses fixed "ble_transfer.dat")
- **File Size Validation**: Validate before transfer
- **Resume Capability**: Resume interrupted transfers
- **File Listing Service**: List available files
- **File Deletion**: Delete files via BLE
- **Multiple File Support**: Support concurrent or queued file transfers

#### 3. Power Optimization
- **BLE Power Management**: Optimize power during recording
- **Sleep Mode**: Implement low-power sleep states
- **Battery Monitoring**: Add battery level service
- **Power Profiling**: Monitor power consumption

#### 4. User Experience
- **BLE Status Indicators**: Visual BLE status
- **Configuration Service**: BLE-configurable settings
- **Firmware Update**: OTA updates via BLE
- **Diagnostic Service**: System diagnostics via BLE

#### 5. Advanced Audio Features
- **Audio Streaming**: Real-time audio streaming via BLE
- **Audio Quality Control**: Configurable sample rates
- **Multi-channel Support**: Support for more microphones
- **Audio Processing**: On-device audio processing

### Development Roadmap

#### Phase 1 (Current)
- ✅ Basic BLE functionality
- ✅ Dynamic state management
- ✅ File transfer protocol
- ✅ Audio recording control

#### Phase 2 (Next)
- 🔄 Enhanced security features
- 🔄 Advanced file management
- 🔄 Power optimization
- 🔄 User experience improvements

#### Phase 3 (Future)
- 📋 Audio streaming capabilities
- 📋 Advanced diagnostics
- 📋 Cloud integration
- 📋 Multi-device support

## Conclusion

This comprehensive BLE implementation provides a solid foundation for mobile app development and device management. The dynamic state management ensures optimal audio quality, while the file transfer protocol enables flexible data exchange. The modular design allows for easy extension and enhancement as requirements evolve.

For mobile app developers, this document provides all necessary information to integrate with the SalesTag device, including service definitions, protocol specifications, and troubleshooting guidance. The implementation is production-ready and includes comprehensive error handling and logging for reliable operation.

---

**Document Version**: 1.0  
**Last Updated**: December 2024  
**ESP-IDF Version**: 5.2.2  
**NimBLE Version**: Latest  
**Device**: ESP32-S3-Mini-BLE  
**Project Name**: salestag-diagnostic
