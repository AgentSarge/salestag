# Mobile Development Guide for SalesTag ESP32-S3

## Table of Contents
1. [Overview](#overview)
2. [Prerequisites](#prerequisites)
3. [Platform-Specific Setup](#platform-specific-setup)
4. [Device Discovery](#device-discovery)
5. [Connection Management](#connection-management)
6. [Service Discovery](#service-discovery)
7. [Audio Recording Control](#audio-recording-control)
8. [File Transfer Implementation](#file-transfer-implementation)
9. [Error Handling](#error-handling)
10. [Testing and Debugging](#testing-and-debugging)
11. [Complete Implementation Examples](#complete-implementation-examples)

## Overview

This guide provides step-by-step instructions for mobile app developers to integrate with the SalesTag ESP32-S3 device. The device provides two main services:

- **Audio Recording Service**: Control audio recording and monitor device status
- **File Transfer Service**: Download files from the device's SD card (ESP32 acts as file sender)

## Prerequisites

### Required Knowledge
- Basic understanding of Bluetooth Low Energy (BLE)
- Familiarity with your chosen mobile platform (iOS/Android/React Native)
- Understanding of GATT services and characteristics

### Device Specifications
- **Device Name**: `ESP32-S3-Mini-BLE`
- **BLE Stack**: NimBLE (peripheral device)
- **Connection Type**: Undirected connectable, general discoverable
- **Max Connections**: 3
- **MTU Size**: 256 bytes

### Service UUIDs
- **Audio Service**: `0x1234`
- **File Transfer Service**: `0x1240`

## Platform-Specific Setup

### Android Setup

#### 1. Add Permissions to AndroidManifest.xml
```xml
<uses-permission android:name="android.permission.BLUETOOTH" />
<uses-permission android:name="android.permission.BLUETOOTH_ADMIN" />
<uses-permission android:name="android.permission.BLUETOOTH_SCAN" />
<uses-permission android:name="android.permission.BLUETOOTH_CONNECT" />
<uses-permission android:name="android.permission.ACCESS_FINE_LOCATION" />
<uses-permission android:name="android.permission.ACCESS_COARSE_LOCATION" />

<!-- For Android 12+ -->
<uses-permission android:name="android.permission.BLUETOOTH_ADVERTISE" />
```

#### 2. Add Feature Declarations
```xml
<uses-feature android:name="android.hardware.bluetooth_le" android:required="true" />
```

#### 3. Request Runtime Permissions
```kotlin
// Request permissions at runtime
private fun requestPermissions() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
        requestPermissions(
            arrayOf(
                Manifest.permission.BLUETOOTH_SCAN,
                Manifest.permission.BLUETOOTH_CONNECT,
                Manifest.permission.ACCESS_FINE_LOCATION
            ),
            PERMISSION_REQUEST_CODE
        )
    } else {
        requestPermissions(
            arrayOf(
                Manifest.permission.ACCESS_FINE_LOCATION
            ),
            PERMISSION_REQUEST_CODE
        )
    }
}
```

### iOS Setup

#### 1. Add to Info.plist
```xml
<key>NSBluetoothAlwaysUsageDescription</key>
<string>This app needs Bluetooth to connect to SalesTag device</string>
<key>NSBluetoothPeripheralUsageDescription</key>
<string>This app needs Bluetooth to connect to SalesTag device</string>
```

#### 2. Enable Background Modes (Optional)
```xml
<key>UIBackgroundModes</key>
<array>
    <string>bluetooth-central</string>
</array>
```

### React Native Setup

#### 1. Install Dependencies
```bash
npm install react-native-ble-plx
# or
yarn add react-native-ble-plx
```

#### 2. Link Library
```bash
npx react-native link react-native-ble-plx
```

#### 3. Configure Permissions
Follow the Android and iOS setup steps above for your React Native project.

## Device Discovery

### Step 1: Initialize BLE Manager

#### Android (Kotlin)
```kotlin
class SalesTagManager(private val context: Context) {
    private val bluetoothManager: BluetoothManager = context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
    private val bluetoothAdapter: BluetoothAdapter? = bluetoothManager.adapter
    private val bluetoothLeScanner: BluetoothLeScanner? = bluetoothAdapter?.bluetoothLeScanner
    
    fun startScan() {
        val scanFilter = ScanFilter.Builder()
            .setDeviceName("ESP32-S3-Mini-BLE")
            .build()
            
        val scanSettings = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build()
            
        bluetoothLeScanner?.startScan(listOf(scanFilter), scanSettings, scanCallback)
    }
}
```

#### iOS (Swift)
```swift
import CoreBluetooth

class SalesTagManager: NSObject, CBCentralManagerDelegate {
    private var centralManager: CBCentralManager!
    private var discoveredDevices: [CBPeripheral] = []
    
    override init() {
        super.init()
        centralManager = CBCentralManager(delegate: self, queue: nil)
    }
    
    func startScan() {
        guard centralManager.state == .poweredOn else { return }
        
        centralManager.scanForPeripherals(
            withServices: nil,
            options: [CBCentralManagerScanOptionAllowDuplicatesKey: false]
        )
    }
    
    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String : Any], rssi RSSI: NSNumber) {
        if peripheral.name == "ESP32-S3-Mini-BLE" {
            discoveredDevices.append(peripheral)
            // Notify UI of discovered device
        }
    }
}
```

#### React Native
```javascript
import { BleManager } from 'react-native-ble-plx';

class SalesTagManager {
    constructor() {
        this.manager = new BleManager();
    }
    
    startScan() {
        this.manager.startDeviceScan(
            null, // null for all services
            { allowDuplicates: false },
            (error, device) => {
                if (error) {
                    console.error('Scan error:', error);
                    return;
                }
                
                if (device && device.name === 'ESP32-S3-Mini-BLE') {
                    console.log('Found SalesTag device:', device);
                    // Handle discovered device
                }
            }
        );
    }
}
```

### Step 2: Handle Scan Results

```kotlin
// Android
private val scanCallback = object : ScanCallback() {
    override fun onScanResult(callbackType: Int, result: ScanResult) {
        val device = result.device
        val rssi = result.rssi
        
        if (device.name == "ESP32-S3-Mini-BLE") {
            // Device found - check RSSI for signal strength
            when {
                rssi > -50 -> println("Excellent signal")
                rssi > -60 -> println("Good signal")
                rssi > -70 -> println("Acceptable signal")
                else -> println("Poor signal - may have connection issues")
            }
            
            // Store device for connection
            salesTagDevice = device
        }
    }
}
```

## Connection Management

### Step 1: Connect to Device

#### Android
```kotlin
fun connectToDevice(device: BluetoothDevice) {
    device.connectGatt(context, false, gattCallback)
}

private val gattCallback = object : BluetoothGattCallback() {
    override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
        when (newState) {
            BluetoothProfile.STATE_CONNECTED -> {
                println("Connected to SalesTag device")
                gatt.discoverServices()
            }
            BluetoothProfile.STATE_DISCONNECTED -> {
                println("Disconnected from SalesTag device")
                // Implement reconnection logic
            }
        }
    }
}
```

#### iOS
```swift
func connectToDevice(_ peripheral: CBPeripheral) {
    centralManager.connect(peripheral, options: nil)
}

func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
    print("Connected to SalesTag device")
    peripheral.delegate = self
    peripheral.discoverServices(nil)
}
```

#### React Native
```javascript
connectToDevice(device) {
    device.connect()
        .then((device) => {
            console.log('Connected to SalesTag device');
            return device.discoverAllServicesAndCharacteristics();
        })
        .then((device) => {
            // Services discovered
            this.discoverServices(device);
        })
        .catch((error) => {
            console.error('Connection failed:', error);
        });
}
```

### Step 2: Handle Disconnections

```kotlin
// Android
override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
    when (newState) {
        BluetoothProfile.STATE_DISCONNECTED -> {
            println("Device disconnected: ${gatt.device.address}")
            // Implement reconnection with exponential backoff
            scheduleReconnection(gatt.device)
        }
    }
}

private fun scheduleReconnection(device: BluetoothDevice) {
    // Implement reconnection logic with backoff
    Handler(Looper.getMainLooper()).postDelayed({
        device.connectGatt(context, false, gattCallback)
    }, 1000) // 1 second delay
}
```

## Service Discovery

### Step 1: Discover Services

```kotlin
// Android
override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
    if (status == BluetoothGatt.GATT_SUCCESS) {
        // Find Audio Service
        val audioService = gatt.getService(UUID.fromString("00001234-0000-1000-8000-00805f9b34fb"))
        
        // Find File Transfer Service
        val fileService = gatt.getService(UUID.fromString("00001240-0000-1000-8000-00805f9b34fb"))
        
        if (audioService != null && fileService != null) {
            setupCharacteristics(audioService, fileService)
        }
    }
}
```

### Step 2: Setup Characteristics

```kotlin
private fun setupCharacteristics(audioService: BluetoothGattService, fileService: BluetoothGattService) {
    // Audio Service Characteristics
    recordControlChar = audioService.getCharacteristic(UUID.fromString("00001235-0000-1000-8000-00805f9b34fb"))
    statusChar = audioService.getCharacteristic(UUID.fromString("00001236-0000-1000-8000-00805f9b34fb"))
    fileCountChar = audioService.getCharacteristic(UUID.fromString("00001237-0000-1000-8000-00805f9b34fb"))
    
    // File Transfer Service Characteristics
    fileControlChar = fileService.getCharacteristic(UUID.fromString("00001241-0000-1000-8000-00805f9b34fb"))
    fileDataChar = fileService.getCharacteristic(UUID.fromString("00001242-0000-1000-8000-00805f9b34fb"))
    fileStatusChar = fileService.getCharacteristic(UUID.fromString("00001243-0000-1000-8000-00805f9b34fb"))
    
    // Enable notifications
    enableNotifications()
}
```

### Step 3: Enable Notifications

```kotlin
private fun enableNotifications() {
    // Enable status notifications
    gatt.setCharacteristicNotification(statusChar, true)
    val statusDescriptor = statusChar.getDescriptor(UUID.fromString("00002902-0000-1000-8000-00805f9b34fb"))
    statusDescriptor.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
    gatt.writeDescriptor(statusDescriptor)
    
    // Enable file status notifications
    gatt.setCharacteristicNotification(fileStatusChar, true)
    val fileStatusDescriptor = fileStatusChar.getDescriptor(UUID.fromString("00002902-0000-1000-8000-00805f9b34fb"))
    fileStatusDescriptor.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
    gatt.writeDescriptor(fileStatusDescriptor)
}
```

## Audio Recording Control

### Step 1: Start Recording

```kotlin
fun startRecording() {
    recordControlChar?.let { char ->
        val command = byteArrayOf(0x01) // START command
        char.value = command
        gatt.writeCharacteristic(char)
    }
}
```

### Step 2: Stop Recording

```kotlin
fun stopRecording() {
    recordControlChar?.let { char ->
        val command = byteArrayOf(0x00) // STOP command
        char.value = command
        gatt.writeCharacteristic(char)
    }
}
```

### Step 3: Read Device Status

```kotlin
fun readDeviceStatus() {
    statusChar?.let { char ->
        gatt.readCharacteristic(char)
    }
}

override fun onCharacteristicRead(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic, status: Int) {
    if (status == BluetoothGatt.GATT_SUCCESS) {
        when (characteristic.uuid) {
            UUID.fromString("00001236-0000-1000-8000-00805f9b34fb") -> {
                // Parse status data
                val data = characteristic.value
                val audioEnabled = data[0] == 1.toByte()
                val sdAvailable = data[1] == 1.toByte()
                val recording = data[2] == 1.toByte()
                val totalFiles = ByteBuffer.wrap(data, 3, 4).order(ByteOrder.LITTLE_ENDIAN).int
                
                println("Device Status: Audio=$audioEnabled, SD=$sdAvailable, Recording=$recording, Files=$totalFiles")
            }
        }
    }
}
```

### Step 4: Handle Status Notifications

```kotlin
override fun onCharacteristicChanged(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic) {
    when (characteristic.uuid) {
        UUID.fromString("00001236-0000-1000-8000-00805f9b34fb") -> {
            // Handle status notification
            val data = characteristic.value
            val audioEnabled = data[0] == 1.toByte()
            val sdAvailable = data[1] == 1.toByte()
            val recording = data[2] == 1.toByte()
            val totalFiles = ByteBuffer.wrap(data, 3, 4).order(ByteOrder.LITTLE_ENDIAN).int
            
            // Update UI with new status
            updateDeviceStatus(audioEnabled, sdAvailable, recording, totalFiles)
        }
    }
}
```

## File Transfer Implementation

**Protocol Overview**: The ESP32 acts as a **file sender**. The mobile app requests files from the ESP32, which reads them from the SD card and sends them to the mobile app.

### Step 1: Start File Transfer

```kotlin
fun startFileTransfer() {
    fileControlChar?.let { char ->
        val command = byteArrayOf(0x01) // START command
        char.value = command
        gatt.writeCharacteristic(char)
    }
}
```

**What happens**: ESP32 prepares to send a file from the SD card to the mobile app.

### Step 2: Enable File Data Notifications

```kotlin
fun enableFileDataNotifications() {
    fileDataChar?.let { char ->
        // Enable notifications for file data characteristic
        gatt.setCharacteristicNotification(char, true)
        val descriptor = char.getDescriptor(UUID.fromString("00002902-0000-1000-8000-00805f9b34fb"))
        descriptor.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
        gatt.writeDescriptor(descriptor)
    }
}
```

**What happens**: Mobile app enables notifications on the File Data characteristic to receive data chunks from the ESP32.

### Step 3: Handle File Data Notifications

```kotlin
// Global variable to collect file data
private val fileDataBuffer = mutableListOf<Byte>()
private var isReceivingFile = false

override fun onCharacteristicChanged(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic) {
    when (characteristic.uuid) {
        UUID.fromString("00001242-0000-1000-8000-00805f9b34fb") -> {
            // Handle file data notifications
            val data = characteristic.value
            fileDataBuffer.addAll(data.toList())
            
            println("Received file chunk: ${data.size} bytes, total: ${fileDataBuffer.size} bytes")
            
            // Check if transfer is complete based on file status notifications
            // The ESP32 will send status notifications indicating completion
        }
    }
}
```

**What happens**: ESP32 pushes file data chunks to the mobile app via notifications. The app collects these chunks in a buffer and waits for the completion signal before finalizing the transfer.

### Step 4: Complete File Transfer

```kotlin
fun completeFileTransfer() {
    fileControlChar?.let { char ->
        val command = byteArrayOf(0x02) // STOP command
        char.value = command
        gatt.writeCharacteristic(char)
    }
}

// Get the complete file data
fun getReceivedFileData(): ByteArray {
    return fileDataBuffer.toByteArray()
}

// Clear the file data buffer
fun clearFileDataBuffer() {
    fileDataBuffer.clear()
    isReceivingFile = false
}
```

**What happens**: ESP32 sends completion notification when file transfer is done. The mobile app waits for this signal before sending the STOP command and retrieving the complete file data from the buffer.

### Step 5: Handle File Transfer Status

```kotlin
override fun onCharacteristicChanged(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic) {
    when (characteristic.uuid) {
        UUID.fromString("00001243-0000-1000-8000-00805f9b34fb") -> {
            // Handle file transfer status
            val data = characteristic.value
            val status = data[0]
            val offset = ByteBuffer.wrap(data, 1, 4).order(ByteOrder.LITTLE_ENDIAN).int
            val size = ByteBuffer.wrap(data, 5, 4).order(ByteOrder.LITTLE_ENDIAN).int
            
            when (status) {
                0x00.toByte() -> println("Transfer IDLE")
                0x01.toByte() -> println("Transfer ACTIVE: $offset/$size bytes")
                0x02.toByte() -> println("Transfer COMPLETE")
                0x03.toByte() -> println("Transfer ERROR")
            }
        }
    }
}
```

**Status Meanings**:
- `0x00` - **IDLE**: No transfer active
- `0x01` - **ACTIVE**: Transfer in progress, shows current progress
- `0x02` - **COMPLETE**: File successfully sent from ESP32's SD card
- `0x03` - **ERROR**: Transfer failed or was aborted

## Error Handling

### Step 1: Implement Error Handling

```kotlin
class SalesTagException(message: String, val errorCode: Int) : Exception(message)

fun handleGattError(status: Int) {
    when (status) {
        BluetoothGatt.GATT_SUCCESS -> {
            // Success
        }
        BluetoothGatt.GATT_INSUFFICIENT_AUTHENTICATION -> {
            throw SalesTagException("Authentication required", status)
        }
        BluetoothGatt.GATT_INSUFFICIENT_ENCRYPTION -> {
            throw SalesTagException("Encryption required", status)
        }
        BluetoothGatt.GATT_INVALID_ATTRIBUTE_LENGTH -> {
            throw SalesTagException("Invalid data length", status)
        }
        BluetoothGatt.GATT_REQUEST_NOT_SUPPORTED -> {
            throw SalesTagException("Request not supported", status)
        }
        else -> {
            throw SalesTagException("GATT error: $status", status)
        }
    }
}
```

### Step 2: Connection Retry Logic

```kotlin
class ConnectionManager {
    private var retryCount = 0
    private val maxRetries = 3
    private val baseDelay = 1000L // 1 second
    
    fun connectWithRetry(device: BluetoothDevice) {
        try {
            device.connectGatt(context, false, gattCallback)
        } catch (e: Exception) {
            if (retryCount < maxRetries) {
                retryCount++
                val delay = baseDelay * (2.0.pow(retryCount.toDouble())).toLong()
                Handler(Looper.getMainLooper()).postDelayed({
                    connectWithRetry(device)
                }, delay)
            } else {
                throw SalesTagException("Failed to connect after $maxRetries attempts", -1)
            }
        }
    }
}
```

### Step 3: Timeout Handling

```kotlin
class TimeoutManager {
    private val timeoutHandler = Handler(Looper.getMainLooper())
    private val timeoutRunnable = Runnable {
        // Handle timeout
        onTimeout()
    }
    
    fun startTimeout(duration: Long) {
        timeoutHandler.postDelayed(timeoutRunnable, duration)
    }
    
    fun cancelTimeout() {
        timeoutHandler.removeCallbacks(timeoutRunnable)
    }
    
    private fun onTimeout() {
        // Handle timeout - disconnect, retry, or show error
        gatt?.disconnect()
    }
}
```

## Testing and Debugging

### Step 1: Create Test Functions

```kotlin
class SalesTagTester {
    fun testDeviceDiscovery() {
        println("Testing device discovery...")
        startScan()
        
        // Wait for device discovery
        Handler(Looper.getMainLooper()).postDelayed({
            if (discoveredDevices.isNotEmpty()) {
                println("✅ Device discovery successful")
            } else {
                println("❌ Device discovery failed")
            }
        }, 10000) // 10 second timeout
    }
    
    fun testConnection() {
        println("Testing connection...")
        connectToDevice(selectedDevice)
        
        // Connection status will be reported in callback
    }
    
    fun testAudioRecording() {
        println("Testing audio recording...")
        startRecording()
        
        // Wait 5 seconds
        Handler(Looper.getMainLooper()).postDelayed({
            stopRecording()
            println("✅ Audio recording test completed")
        }, 5000)
    }
    
    fun testFileTransfer() {
        println("Testing file transfer...")
        
        // Enable notifications first
        enableFileDataNotifications()
        
        // Start transfer and wait for completion notification
        startFileTransfer()
        
        // In a real app, you would wait for the completion notification
        // For testing purposes, we'll wait a reasonable time
        Thread.sleep(5000) // Wait 5 seconds for transfer
        
        // Only complete if we haven't received completion notification
        if (fileDataBuffer.isNotEmpty()) {
            completeFileTransfer()
        }
        
        val fileData = getReceivedFileData()
        println("✅ File transfer test completed - received ${fileData.size} bytes from ESP32's SD card")
        
        // Clear buffer for next transfer
        clearFileDataBuffer()
    }
}
```

### Step 2: Debug Logging

```kotlin
class SalesTagLogger {
    companion object {
        private const val TAG = "SalesTag"
        
        fun d(message: String) {
            Log.d(TAG, message)
        }
        
        fun e(message: String, throwable: Throwable? = null) {
            Log.e(TAG, message, throwable)
        }
        
        fun w(message: String) {
            Log.w(TAG, message)
        }
    }
}
```

### Step 3: Performance Monitoring

```kotlin
class PerformanceMonitor {
    private val startTimes = mutableMapOf<String, Long>()
    
    fun startTimer(operation: String) {
        startTimes[operation] = System.currentTimeMillis()
    }
    
    fun endTimer(operation: String) {
        val startTime = startTimes[operation]
        if (startTime != null) {
            val duration = System.currentTimeMillis() - startTime
            println("⏱️ $operation took ${duration}ms")
            startTimes.remove(operation)
        }
    }
}
```

## Complete Implementation Examples

### Android Complete Example

```kotlin
class SalesTagManager(private val context: Context) {
    private var gatt: BluetoothGatt? = null
    private var salesTagDevice: BluetoothDevice? = null
    
    // Characteristics
    private var recordControlChar: BluetoothGattCharacteristic? = null
    private var statusChar: BluetoothGattCharacteristic? = null
    private var fileControlChar: BluetoothGattCharacteristic? = null
    private var fileDataChar: BluetoothGattCharacteristic? = null
    private var fileStatusChar: BluetoothGattCharacteristic? = null
    
    // Callbacks
    private val gattCallback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
            when (newState) {
                BluetoothProfile.STATE_CONNECTED -> {
                    SalesTagLogger.d("Connected to SalesTag device")
                    gatt.discoverServices()
                }
                BluetoothProfile.STATE_DISCONNECTED -> {
                    SalesTagLogger.d("Disconnected from SalesTag device")
                    // Implement reconnection logic
                }
            }
        }
        
        override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                setupServices(gatt)
            }
        }
        
        override fun onCharacteristicChanged(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic) {
            handleCharacteristicChange(characteristic)
        }
    }
    
    fun connectToDevice(device: BluetoothDevice) {
        salesTagDevice = device
        gatt = device.connectGatt(context, false, gattCallback)
    }
    
    fun startRecording() {
        recordControlChar?.let { char ->
            char.value = byteArrayOf(0x01)
            gatt?.writeCharacteristic(char)
        }
    }
    
    fun stopRecording() {
        recordControlChar?.let { char ->
            char.value = byteArrayOf(0x00)
            gatt?.writeCharacteristic(char)
        }
    }
    
    fun transferFile(data: ByteArray) {
        startFileTransfer()
        sendFileData(data)
        completeFileTransfer()
    }
    
    private fun setupServices(gatt: BluetoothGatt) {
        // Setup service characteristics
        // Enable notifications
        // Ready for operations
    }
    
    private fun handleCharacteristicChange(characteristic: BluetoothGattCharacteristic) {
        // Handle status and file transfer notifications
    }
}
```

### React Native Complete Example

```javascript
import { BleManager } from 'react-native-ble-plx';

class SalesTagManager {
    constructor() {
        this.manager = new BleManager();
        this.device = null;
        this.services = {};
        this.characteristics = {};
        this.fileDataBuffer = [];
    }
    
    async scanForDevice() {
        return new Promise((resolve, reject) => {
            this.manager.startDeviceScan(
                null,
                { allowDuplicates: false },
                (error, device) => {
                    if (error) {
                        reject(error);
                        return;
                    }
                    
                    if (device && device.name === 'ESP32-S3-Mini-BLE') {
                        this.manager.stopDeviceScan();
                        this.device = device;
                        resolve(device);
                    }
                }
            );
        });
    }
    
    async connect() {
        if (!this.device) {
            throw new Error('No device selected');
        }
        
        const connectedDevice = await this.device.connect();
        const discoveredDevice = await connectedDevice.discoverAllServicesAndCharacteristics();
        
        // Discover services and characteristics
        await this.discoverServices(discoveredDevice);
        
        return discoveredDevice;
    }
    
    async discoverServices(device) {
        const services = await device.services();
        
        for (const service of services) {
            const characteristics = await service.characteristics();
            
            // Store characteristics by UUID
            for (const characteristic of characteristics) {
                this.characteristics[characteristic.uuid] = characteristic;
            }
        }
        
        // Enable notifications
        await this.enableNotifications();
    }
    
    async enableNotifications() {
        const statusChar = this.characteristics['1236'];
        const fileStatusChar = this.characteristics['1243'];
        const fileDataChar = this.characteristics['1242'];
        
        if (statusChar) {
            await statusChar.monitor((error, characteristic) => {
                if (characteristic) {
                    this.handleStatusNotification(characteristic);
                }
            });
        }
        
        if (fileStatusChar) {
            await fileStatusChar.monitor((error, characteristic) => {
                if (characteristic) {
                    this.handleFileStatusNotification(characteristic);
                }
            });
        }
        
        if (fileDataChar) {
            await fileDataChar.monitor((error, characteristic) => {
                if (characteristic) {
                    this.handleFileDataNotification(characteristic);
                }
            });
        }
    }
    
    async startRecording() {
        const recordControl = this.characteristics['1235'];
        if (recordControl) {
            await recordControl.writeWithResponse(new Uint8Array([0x01]));
        }
    }
    
    async stopRecording() {
        const recordControl = this.characteristics['1235'];
        if (recordControl) {
            await recordControl.writeWithResponse(new Uint8Array([0x00]));
        }
    }
    
    async transferFile() {
        const fileControl = this.characteristics['1241'];
        const fileStatus = this.characteristics['1243'];
        
        if (!fileControl || !fileStatus) {
            throw new Error('File transfer characteristics not found');
        }
        
        return new Promise(async (resolve, reject) => {
            // Clear previous file data
            this.fileDataBuffer = [];
            
            // Set up the file status monitor to listen for transfer completion
            const transferMonitor = fileStatus.monitor((error, characteristic) => {
                if (error) {
                    reject(error);
                    return;
                }
                
                const status = new Uint8Array(characteristic.value)[0];
                if (status === 0x02) {
                    console.log('Transfer complete notification received');
                    // Send the STOP command and resolve the promise
                    fileControl.writeWithResponse(new Uint8Array([0x02]))
                        .then(() => resolve(new Uint8Array(this.fileDataBuffer)))
                        .catch(reject);
                    transferMonitor.remove(); // Unsubscribe from notifications
                } else if (status === 0x03) {
                    reject(new Error('Transfer failed'));
                    transferMonitor.remove(); // Unsubscribe from notifications
                }
            });
            
            try {
                // Start transfer - ESP32 prepares to send file from SD card
                await fileControl.writeWithResponse(new Uint8Array([0x01]));
                console.log('File transfer started...');
            } catch (error) {
                transferMonitor.remove(); // Clean up on error
                reject(error);
            }
        });
    }
    
    handleStatusNotification(characteristic) {
        const value = characteristic.value;
        // Parse status data and update UI
        console.log('Status notification:', value);
    }
    
    handleFileStatusNotification(characteristic) {
        const value = characteristic.value;
        // Parse file transfer status and update UI
        console.log('File status notification:', value);
    }
    
    handleFileDataNotification(characteristic) {
        const value = characteristic.value;
        // Collect file data chunks
        this.fileDataBuffer.push(...value);
        console.log('File data notification:', value.length, 'bytes, total:', this.fileDataBuffer.length);
    }
    
    disconnect() {
        if (this.device) {
            this.device.cancelConnection();
        }
    }
}

export default SalesTagManager;
```

## Summary

This guide provides comprehensive step-by-step instructions for implementing the SalesTag ESP32-S3 device in mobile applications. Key points to remember:

1. **Always check permissions** before scanning
2. **Handle disconnections gracefully** with retry logic
3. **Monitor device status** for recording state changes
4. **Use appropriate chunk sizes** for file transfers (max 512 bytes)
5. **Implement proper error handling** for robust operation
6. **Test thoroughly** with the provided test functions

**File Transfer Protocol**: The ESP32 acts as a **file sender**. Mobile apps download files from the ESP32's SD card. This is useful for retrieving recorded audio files, log files, or other data stored on the device.

The device provides a reliable BLE interface for audio recording control and file download operations, making it suitable for various mobile applications.

---

**Document Version**: 1.0  
**Last Updated**: December 2024  
**Compatible with**: SalesTag ESP32-S3 Firmware v1.0
