/**
 * @file ble_integrity.h
 * @brief BLE packet format with CRC32C integrity checking
 */

#ifndef BLE_INTEGRITY_H
#define BLE_INTEGRITY_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// BLE chunk structure for file transfer with integrity protection
typedef struct __attribute__((packed)) {
    uint16_t proto_ver;     // Protocol version (0x0001)
    uint16_t seq;           // Sequence number (increments by 1)
    uint32_t file_id;       // File ID or session ID
    uint32_t offset;        // Byte offset in file
    uint16_t payload_len;   // Number of payload bytes following
    uint16_t flags;         // 0=mid, 1=start, 2=end, 3=single
    // payload bytes [payload_len]
    // uint32_t crc32c;     // CRC32C over header+payload (appended after payload)
} ble_chunk_header_t;

// Static assert to ensure header size is predictable
_Static_assert(sizeof(ble_chunk_header_t) == 16, "BLE chunk header must be 16 bytes");

// Chunk flags
#define BLE_CHUNK_FLAG_MID    0
#define BLE_CHUNK_FLAG_START  1
#define BLE_CHUNK_FLAG_END    2
#define BLE_CHUNK_FLAG_SINGLE 3

// Protocol version
#define BLE_PROTOCOL_VERSION  0x0001

/**
 * @brief Validate RAW audio file header before BLE transfer
 * @param f File pointer (will be repositioned)
 * @return true if header is valid, false otherwise
 */
bool validate_raw_header_from_sd(FILE *f);

/**
 * @brief Calculate CRC32C for BLE chunk (header + payload)
 * @param header Pointer to chunk header
 * @param payload Pointer to payload data
 * @param payload_len Length of payload
 * @return CRC32C value
 */
uint32_t ble_chunk_calculate_crc(const ble_chunk_header_t *header, 
                                 const uint8_t *payload, 
                                 uint16_t payload_len);

/**
 * @brief Quick CRC check of first 4KB of file for integrity verification
 * @param f File pointer (positioned after header)
 * @return CRC32C of first 4KB
 */
uint32_t raw_file_quick_crc_check(FILE *f);

#endif // BLE_INTEGRITY_H
