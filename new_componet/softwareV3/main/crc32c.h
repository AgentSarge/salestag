/**
 * @file crc32c.h
 * @brief CRC32C (Castagnoli) implementation for BLE data integrity
 */

#ifndef CRC32C_H
#define CRC32C_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Initialize CRC32C lookup table
 */
void crc32c_init(void);

/**
 * @brief Update CRC32C with new data
 * @param crc Current CRC value
 * @param data Data to process
 * @param len Length of data
 * @return Updated CRC value
 */
uint32_t crc32c_update(uint32_t crc, const uint8_t *data, size_t len);

/**
 * @brief Calculate CRC32C for data block
 * @param data Data to process
 * @param len Length of data
 * @return CRC32C value
 */
uint32_t crc32c_calculate(const uint8_t *data, size_t len);

#endif // CRC32C_H
