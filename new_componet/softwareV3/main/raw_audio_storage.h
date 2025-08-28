#ifndef RAW_AUDIO_STORAGE_H
#define RAW_AUDIO_STORAGE_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

// Raw audio sample structure (single mic) - PACKED for BLE integrity
typedef struct __attribute__((packed)) {
    uint16_t mic_sample;   // Raw ADC value from GPIO 9 (MIC) - MUST be 0-4095
    uint32_t timestamp_ms; // Timestamp in milliseconds
    uint32_t sample_count; // Sequential sample number
} raw_audio_sample_t;

// Static assert to ensure struct packing integrity
_Static_assert(sizeof(raw_audio_sample_t) == 10, "Sample must be 10 bytes for BLE compatibility");

// Raw audio file header - PACKED for BLE integrity
typedef struct __attribute__((packed)) {
    uint32_t magic_number;     // Magic number to identify file format (0x52415741 = "RAWA")
    uint32_t version;          // File format version
    uint32_t sample_rate;      // Samples per second
    uint32_t total_samples;    // Total number of samples in file
    uint32_t start_timestamp;  // Start timestamp in milliseconds
    uint32_t end_timestamp;    // End timestamp in milliseconds
    uint32_t reserved[4];      // Reserved for future use
} raw_audio_header_t;

// Static assert to ensure header packing integrity
_Static_assert(sizeof(raw_audio_header_t) == 32, "RAW header must be 32 bytes for BLE compatibility");

// Configuration
#define RAW_AUDIO_MAGIC_NUMBER 0x52415741  // "RAWA" in ASCII
#define RAW_AUDIO_VERSION 1
#define RAW_AUDIO_SAMPLE_RATE 16000  // Updated to 16kHz for high quality
#define RAW_AUDIO_BUFFER_SIZE 512  // Number of samples to buffer before writing

// Initialize raw audio storage
esp_err_t raw_audio_storage_init(void);

// Start recording raw audio to a new file
esp_err_t raw_audio_storage_start_recording(const char* filename);

// Stop recording and close the current file
esp_err_t raw_audio_storage_stop_recording(void);

// Add a raw audio sample to the current recording (single mic)
esp_err_t raw_audio_storage_add_sample(uint16_t mic_adc);

// Check if currently recording
bool raw_audio_storage_is_recording(void);

// Get current recording statistics
esp_err_t raw_audio_storage_get_stats(uint32_t* samples_written, uint32_t* file_size_bytes);

// Deinitialize raw audio storage
esp_err_t raw_audio_storage_deinit(void);

// Get corruption counters for monitoring
void raw_audio_storage_get_counters(uint32_t *oob, uint32_t *ffff);

// Reset corruption counters
void raw_audio_storage_reset_counters(void);

#endif // RAW_AUDIO_STORAGE_H
