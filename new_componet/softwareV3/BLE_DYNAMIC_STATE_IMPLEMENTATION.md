# Dynamic BLE State Implementation

## Overview

This document describes the implementation of dynamic BLE state management based on recording status in the SalesTag ESP32-S3 firmware. The system ensures that BLE advertising is automatically stopped during audio recording to prevent interference and restarted when recording stops.

## Implementation Details

### 1. Global State Variable

```c
// Global state for recording
static volatile bool s_is_recording = false;  // Made volatile for multi-task access
```

The `s_is_recording` variable is marked as `volatile` because it's accessed by multiple tasks (button handling, BLE callbacks, and main application logic).

### 2. BLE Advertising Control Functions

#### `ble_stop_advertising()`
- Stops BLE advertising to prevent audio interference
- Called when recording starts
- Logs the action for debugging

#### `ble_start_advertising_if_not_recording()`
- Conditionally starts BLE advertising only if not currently recording
- Prevents advertising restart during active recording
- Logs whether advertising was started or skipped

### 3. Recording Start Logic

When recording starts (via button press or BLE command):

1. **Stop BLE advertising** to prevent interference
2. Start audio capture and storage
3. Set `s_is_recording = true`
4. If recording fails, restart BLE advertising

```c
// Stop BLE advertising to prevent interference
ble_stop_advertising();

esp_err_t ret = raw_audio_storage_start_recording(s_current_raw_file);
if (ret == ESP_OK) {
    ret = audio_capture_start();
    if (ret == ESP_OK) {
        s_is_recording = true;
        // Recording successful
    } else {
        // Restart BLE advertising since recording failed
        ble_start_advertising_if_not_recording();
    }
} else {
    // Restart BLE advertising since recording failed
    ble_start_advertising_if_not_recording();
}
```

### 4. Recording Stop Logic

When recording stops:

1. Stop audio capture and storage
2. Set `s_is_recording = false`
3. **Restart BLE advertising** now that recording is finished

```c
audio_capture_stop();
esp_err_t ret = raw_audio_storage_stop_recording();
if (ret == ESP_OK) {
    s_is_recording = false;
    // Restart BLE advertising now that recording is finished
    ble_start_advertising_if_not_recording();
}
```

### 5. BLE Callback Updates

#### `ble_app_on_sync()`
- Only starts advertising if not currently recording
- Prevents advertising during system initialization if recording is active

#### `ble_app_on_disconnect()`
- Only restarts advertising if not currently recording
- Prevents advertising restart during active recording

#### `ble_app_on_adv_complete()`
- Only restarts advertising if not currently recording
- Prevents advertising restart during active recording

### 6. BLE GATT Service Integration

The BLE GATT service for recording control also implements the same logic:

- **Start Recording Command**: Stops BLE advertising before starting recording
- **Stop Recording Command**: Restarts BLE advertising after stopping recording
- **Error Handling**: Restarts BLE advertising if recording operations fail

## Benefits

1. **Clean Audio Signal**: BLE advertising is disabled during recording to prevent RF interference
2. **Predictable Behavior**: Device is only discoverable when not recording
3. **Automatic Management**: No manual intervention required - system handles BLE state automatically
4. **Error Recovery**: BLE advertising is restored if recording operations fail

## Testing

To test the implementation:

1. **Initial State**: Device should be advertising and discoverable
2. **Start Recording**: Press button or send BLE command - device should stop advertising
3. **During Recording**: Device should not be discoverable via BLE
4. **Stop Recording**: Press button or send BLE command - device should restart advertising
5. **Error Scenarios**: If recording fails, BLE advertising should be restored

## Log Messages

The implementation includes comprehensive logging:

- `"Stopping BLE advertising to prevent audio interference"`
- `"Starting BLE advertising (not currently recording)"`
- `"Skipping BLE advertising start (currently recording)"`
- `"BLE advertising stopped successfully"`
- `"Restart BLE advertising since recording failed"`

## Future Enhancements

1. **BLE Connection Management**: Consider disconnecting active BLE connections when recording starts
2. **Power Optimization**: Implement BLE power management during recording
3. **User Feedback**: Add BLE status indicators in the GATT service
4. **Configuration**: Make BLE behavior configurable via settings

## Files Modified

- `main/main.c`: Added dynamic BLE state management functions and updated all recording logic
