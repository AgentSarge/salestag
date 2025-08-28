/**
 * @file ble_integrity.c
 * @brief BLE packet format with CRC32C integrity checking
 */

#include "ble_integrity.h"
#include "crc32c.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "ble_integrity";

bool validate_raw_header_from_sd(FILE *f) {
    uint8_t hdr[32];
    long original_pos = ftell(f);
    
    // Read header
    if (fread(hdr, 1, 32, f) != 32) {
        ESP_LOGE(TAG, "Failed to read RAW header");
        fseek(f, original_pos, SEEK_SET);
        return false;
    }
    
    // Check magic number (little-endian "RAWA" = 0x52415741)
    uint32_t magic = (uint32_t)hdr[0] | ((uint32_t)hdr[1] << 8) | 
                     ((uint32_t)hdr[2] << 16) | ((uint32_t)hdr[3] << 24);
    
    if (magic != 0x52415741) {
        ESP_LOGE(TAG, "RAW magic mismatch. Got 0x%08X, expected 0x52415741", magic);
        ESP_LOGE(TAG, "Header bytes: %02X %02X %02X %02X", hdr[0], hdr[1], hdr[2], hdr[3]);
        fseek(f, original_pos, SEEK_SET);
        return false;
    }
    
    // Parse total samples for size validation
    uint32_t total_samples = (uint32_t)hdr[12] | ((uint32_t)hdr[13] << 8) | 
                            ((uint32_t)hdr[14] << 16) | ((uint32_t)hdr[15] << 24);
    
    // Check file size consistency
    long current_pos = ftell(f);
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, current_pos, SEEK_SET);
    
    long expected_size = 32 + (long)total_samples * 10;  // 32-byte header + 10-byte samples
    
    if (file_size != expected_size) {
        ESP_LOGW(TAG, "RAW size mismatch. File=%ld bytes, expected=%ld bytes (samples=%lu)", 
                 file_size, expected_size, (unsigned long)total_samples);
        // Continue anyway - might be partial file
    }
    
    ESP_LOGI(TAG, "RAW header validation passed: %lu samples, %ld bytes", 
             (unsigned long)total_samples, file_size);
    
    // Restore original position
    fseek(f, original_pos, SEEK_SET);
    return true;
}

uint32_t ble_chunk_calculate_crc(const ble_chunk_header_t *header, 
                                 const uint8_t *payload, 
                                 uint16_t payload_len) {
    uint32_t crc = 0xFFFFFFFF;
    
    // CRC over header (excluding the CRC field that comes after payload)
    crc = crc32c_update(crc, (const uint8_t*)header, sizeof(ble_chunk_header_t));
    
    // CRC over payload
    if (payload && payload_len > 0) {
        crc = crc32c_update(crc, payload, payload_len);
    }
    
    return ~crc;
}

uint32_t raw_file_quick_crc_check(FILE *f) {
    uint8_t buf[4096];
    long original_pos = ftell(f);
    
    // Position after header (32 bytes)
    fseek(f, 32, SEEK_SET);
    
    // Read up to 4KB of sample data
    size_t n = fread(buf, 1, sizeof(buf), f);
    
    // Calculate CRC
    uint32_t crc = crc32c_calculate(buf, n);
    
    ESP_LOGI(TAG, "Quick CRC32C check: %zu bytes, CRC=0x%08X", n, crc);
    
    // Restore position
    fseek(f, original_pos, SEEK_SET);
    
    return crc;
}
