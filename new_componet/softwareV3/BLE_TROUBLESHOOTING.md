# BLE Advertising Troubleshooting Guide

## Problem Description
The ESP32-S3 device is not appearing in BLE scanner apps despite the code reaching the advertising stage.

## Root Cause Analysis

### 1. Verify Application Logic
The logs show "BLE functionality removed - device operates in standalone mode" which indicates conditional logic is disabling BLE.

**Check for:**
- Conditional statements that disable BLE based on hardware detection
- Flags or configuration that bypass BLE initialization
- Early return statements that prevent advertising

### 2. Review Advertising Data
The advertising packet might be empty or malformed.

**Key Issues to Check:**

#### Device Name Configuration
```c
// Current implementation
const char *name = "ESP32-S3-Mini-BLE";
fields.name = (uint8_t *)name;
fields.name_len = strlen(name);
fields.name_is_complete = 1;
```

**Verify:**
- Device name is not empty or NULL
- Name length is within BLE limits (31 bytes max)
- `name_is_complete` is set to 1

#### Advertising Flags
```c
// Fixed implementation
fields.flags = BLE_HS_ADV_F_DISC_GEN;  // General discoverable only
```

**Issues Fixed:**
- Removed conflicting `BLE_HS_ADV_F_DISC_LTD` flag
- Using only general discoverable mode

#### Advertising Parameters
```c
adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;  // Undirected connectable
adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;  // General discoverable
adv_params.itvl_min = BLE_GAP_ADV_FAST_INTERVAL1_MIN;  // Fast advertising
adv_params.itvl_max = BLE_GAP_ADV_FAST_INTERVAL1_MAX;
```

## Testing Steps

### 1. Build and Flash
```bash
cd new_componet/softwareV3
./test_ble.sh
```

### 2. Monitor Logs
Look for these key messages:
```
I (xxx) salestag-sd: NimBLE Host Stack is synchronized.
I (xxx) salestag-sd: Setting advertising data - name: 'ESP32-S3-Mini-BLE' (len: 18)
I (xxx) salestag-sd: Scan response data set successfully
I (xxx) salestag-sd: Starting advertising with parameters:
I (xxx) salestag-sd: Advertising started successfully!
I (xxx) salestag-sd: BLE Host Stack Synced: YES
I (xxx) salestag-sd: Advertising Status: ACTIVE
```

### 3. Test with Scanner Apps
- **nRF Connect** (Android/iOS)
- **BLE Scanner** (Android)
- **LightBlue** (iOS)
- **Bluetooth Scanner** (Windows)

### 4. Check Device Visibility
- Ensure device is within 10 meters
- Check for interference from other BLE devices
- Verify scanner app has location permissions (Android)

## Common Issues and Solutions

### Issue 1: Device Not Appearing in Scanner
**Symptoms:** No device visible in any scanner app
**Causes:**
- Advertising not started
- Wrong advertising parameters
- Hardware issues

**Solutions:**
1. Check logs for "Advertising started successfully"
2. Verify advertising parameters
3. Test with different scanner apps

### Issue 2: Device Appears but Can't Connect
**Symptoms:** Device visible but connection fails
**Causes:**
- GATT service registration issues
- Connection parameters mismatch

**Solutions:**
1. Check GATT service registration logs
2. Verify connection parameters
3. Test with nRF Connect app

### Issue 3: Intermittent Visibility
**Symptoms:** Device appears/disappears randomly
**Causes:**
- Advertising restart issues
- Power management
- Interference

**Solutions:**
1. Check advertising restart logic
2. Monitor power consumption
3. Test in different environments

## Debug Commands

### Check BLE Status
```bash
# Monitor device logs
idf.py monitor

# Check BLE stack status
# Look for: "BLE Host Stack Synced: YES"
```

### Test Advertising
```bash
# Use nRF Connect to scan
# Look for device name: "ESP32-S3-Mini-BLE"
# Check RSSI values (should be > -70 dBm)
```

### Verify Hardware
```bash
# Check GPIO configuration
# Verify antenna connection
# Test with known working BLE device
```

## Expected Behavior

### Successful BLE Operation
1. Device boots and initializes NimBLE stack
2. GATT services register successfully
3. Advertising starts with proper parameters
4. Device appears in scanner apps
5. Connection can be established
6. GATT characteristics are accessible

### Log Output
```
I (xxx) salestag-sd: Initializing NimBLE host stack...
I (xxx) salestag-sd: NimBLE device name set to: ESP32-S3-Mini-BLE
I (xxx) salestag-sd: NimBLE host stack started successfully
I (xxx) salestag-sd: BLE Host Stack is synchronized.
I (xxx) salestag-sd: Setting advertising data - name: 'ESP32-S3-Mini-BLE' (len: 18)
I (xxx) salestag-sd: Scan response data set successfully
I (xxx) salestag-sd: Starting advertising with parameters:
I (xxx) salestag-sd: Advertising started successfully!
```

## Next Steps

1. **Flash the updated code** with improved error handling
2. **Monitor the logs** for advertising status
3. **Test with multiple scanner apps**
4. **Check for hardware issues** if software appears correct
5. **Verify power supply** and antenna connection

## Contact Information
If issues persist after following this guide, please provide:
- Complete log output from device boot
- Screenshots from scanner apps
- Hardware setup details
- Test environment information
