/**
 * @file crc32c.c
 * @brief CRC32C (Castagnoli) implementation for BLE data integrity
 */

#include "crc32c.h"

static uint32_t crc32c_table[256];
static bool crc32c_initialized = false;

void crc32c_init(void) {
    if (crc32c_initialized) return;
    
    const uint32_t poly = 0x1EDC6F41;  // Castagnoli polynomial
    
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            crc = (crc & 1) ? (crc >> 1) ^ poly : (crc >> 1);
        }
        crc32c_table[i] = crc;
    }
    
    crc32c_initialized = true;
}

uint32_t crc32c_update(uint32_t crc, const uint8_t *data, size_t len) {
    if (!crc32c_initialized) {
        crc32c_init();
    }
    
    crc = ~crc;
    for (size_t i = 0; i < len; i++) {
        crc = crc32c_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return ~crc;
}

uint32_t crc32c_calculate(const uint8_t *data, size_t len) {
    return crc32c_update(0xFFFFFFFF, data, len);
}
