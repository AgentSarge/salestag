#pragma once
#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize WAV file writing
esp_err_t wav_writer_begin(const char *path, int sample_rate, int bits_per_sample, int channels);

// Write audio data
esp_err_t wav_writer_write(const void *data, size_t bytes);

// Finalize WAV file
esp_err_t wav_writer_end(void);

#ifdef __cplusplus
}
#endif
