# BLE File Transfer Protocol Implementation

## Overview

This document describes the implementation of a custom BLE file transfer protocol for the SalesTag device. The protocol allows clients to transfer files to the ESP32-S3 device via BLE, storing them on the SD card.

## Service and Characteristic Definitions

### File Transfer Service
- **Service UUID**: `0x1240` (BLE_UUID_SALESTAG_FILE_SVC)
- **Type**: Primary Service

### Characteristics

#### 1. File Control Characteristic
- **UUID**: `0x1241` (BLE_UUID_SALESTAG_FILE_CTRL)
- **Properties**: Read, Write
- **Purpose**: Control file transfer operations

**Commands**:
- `0x01` - FILE_TRANSFER_CMD_START: Start a new file transfer
- `0x02` - FILE_TRANSFER_CMD_STOP: Complete the current file transfer
- `0x03` - FILE_TRANSFER_CMD_ABORT: Abort the current file transfer

#### 2. File Data Characteristic
- **UUID**: `0x1242` (BLE_UUID_SALESTAG_FILE_DATA)
- **Properties**: Read, Write
- **Purpose**: Transfer file data in chunks
- **Max Chunk Size**: 512 bytes

#### 3. File Status Characteristic
- **UUID**: `0x1243` (BLE_UUID_SALESTAG_FILE_STATUS)
- **Properties**: Read, Notify
- **Purpose**: Report transfer status and progress

**Status Values**:
- `0x00` - FILE_TRANSFER_STATUS_IDLE: No transfer active
- `0x01` - FILE_TRANSFER_STATUS_ACTIVE: Transfer in progress
- `0x02` - FILE_TRANSFER_STATUS_COMPLETE: Transfer completed successfully
- `0x03` - FILE_TRANSFER_STATUS_ERROR: Transfer failed or aborted

## Protocol Flow

### 1. Starting a File Transfer

1. Client writes to File Control characteristic with command `0x01` (START)
2. Server creates a new file on SD card (default: `ble_transfer.dat`)
3. Server sends status notification: `ACTIVE` with offset=0, size=1024
4. Server is ready to receive data

### 2. Transferring File Data

1. Client writes file data to File Data characteristic in chunks (max 512 bytes)
2. Server writes data to SD card file
3. Server tracks transfer progress (offset)
4. Server sends progress notifications every 1KB transferred

### 3. Completing a File Transfer

1. Client writes to File Control characteristic with command `0x02` (STOP)
2. Server closes the file
3. Server sends status notification: `COMPLETE` with final offset and size

### 4. Aborting a File Transfer

1. Client writes to File Control characteristic with command `0x03` (ABORT)
2. Server closes the file and deletes it
3. Server sends status notification: `ERROR` with current offset and size

## Implementation Details

### State Management

The implementation maintains the following global state variables:

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

#### `file_transfer_start()`
- Validates SD card availability
- Creates file on SD card
- Initializes transfer state
- Sends initial status notification

#### `file_transfer_write_data()`
- Writes data chunks to SD card file
- Tracks transfer progress
- Sends progress notifications every 1KB

#### `file_transfer_stop()`
- Closes the file
- Sends completion notification
- Resets transfer state

#### `file_transfer_abort()`
- Closes and deletes the file
- Sends error notification
- Resets transfer state

#### `file_transfer_notify_status()`
- Sends status notifications to connected client
- Includes current offset and total size

### Error Handling

- **SD Card Unavailable**: Returns error if SD card is not mounted
- **File Creation Failure**: Returns error if file cannot be created
- **Write Failures**: Returns error if data cannot be written to file
- **Connection Loss**: Automatically aborts transfer if BLE connection is lost

### Connection Management

- Connection handle is stored when client connects
- File transfer status handle is stored during service registration
- Transfer state is cleaned up when client disconnects
- Active transfers are automatically aborted on disconnect

## Usage Example

### Client-Side Protocol

1. **Connect to device** and discover services
2. **Subscribe to File Status characteristic** for notifications
3. **Start transfer**: Write `0x01` to File Control characteristic
4. **Send data**: Write file data in chunks to File Data characteristic
5. **Complete transfer**: Write `0x02` to File Control characteristic

### Status Notifications

The File Status characteristic provides notifications with the following structure:
```c
struct {
    uint8_t status;    // Transfer status (IDLE/ACTIVE/COMPLETE/ERROR)
    uint32_t offset;   // Current transfer offset in bytes
    uint32_t size;     // Total file size in bytes
} status_notification;
```

## Integration with Existing System

The file transfer service coexists with the existing audio recording service:

- **Audio Service**: UUID `0x1234` - Handles recording control and status
- **File Transfer Service**: UUID `0x1240` - Handles file transfers
- **Independent Operation**: Both services can operate simultaneously
- **Resource Management**: File transfers are paused during audio recording to prevent interference

## Future Enhancements

1. **Dynamic Filename Support**: Allow client to specify custom filenames
2. **File Size Validation**: Validate file size before starting transfer
3. **Resume Capability**: Support resuming interrupted transfers
4. **File Listing**: Add service to list available files on SD card
5. **File Deletion**: Add capability to delete files via BLE
6. **Transfer Encryption**: Add optional encryption for sensitive files

## Testing

To test the file transfer protocol:

1. **Build and flash** the firmware to the ESP32-S3 device
2. **Connect** using a BLE client (e.g., nRF Connect app)
3. **Discover services** and locate the File Transfer service
4. **Subscribe** to File Status characteristic notifications
5. **Start transfer** by writing `0x01` to File Control
6. **Send test data** by writing to File Data characteristic
7. **Complete transfer** by writing `0x02` to File Control
8. **Verify file** exists on SD card with transferred data

## Security Considerations

- **No Authentication**: Current implementation has no authentication
- **No Encryption**: File data is transferred in plain text
- **Access Control**: Any connected BLE client can transfer files
- **File System**: Files are stored in `/sdcard/` directory without restrictions

For production use, consider implementing:
- BLE pairing and bonding
- File transfer encryption
- Access control mechanisms
- File size limits and validation
