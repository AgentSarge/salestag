#ifndef WAV_WRITER_H
#define WAV_WRITER_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

// WAV file header structure for mono audio
typedef struct {
    // RIFF header
    char riff_header[4];        // "RIFF"
    uint32_t wav_size;          // File size - 8
    char wave_header[4];        // "WAVE"
    
    // Format chunk
    char fmt_header[4];         // "fmt "
    uint32_t fmt_chunk_size;    // 16 for PCM
    uint16_t audio_format;      // 1 for PCM
    uint16_t num_channels;      // 1 for mono (UPDATED from stereo)
    uint32_t sample_rate;       // 16000 Hz
    uint32_t byte_rate;         // sample_rate * channels * (bits/8)
    uint16_t sample_alignment;  // channels * (bits/8)
    uint16_t bit_depth;         // 16 bits
    
    // Data chunk
    char data_header[4];        // "data"
    uint32_t data_bytes;        // Number of bytes in data
} wav_header_t;

// WAV file configuration
#define WAV_SAMPLE_RATE 16000   // 16kHz sampling rate
#define WAV_BIT_DEPTH 16        // 16-bit audio
#define WAV_CHANNELS 1          // Mono (1 channel)
#define WAV_BYTES_PER_SAMPLE (WAV_BIT_DEPTH / 8)
#define WAV_BYTES_PER_FRAME (WAV_CHANNELS * WAV_BYTES_PER_SAMPLE)
#define WAV_BYTE_RATE (WAV_SAMPLE_RATE * WAV_BYTES_PER_FRAME)

// Initialize WAV writer
esp_err_t wav_writer_init(void);

// Start writing a new WAV file
esp_err_t wav_writer_start_file(const char* filename);

// Write audio data to the current WAV file
esp_err_t wav_writer_write_audio_data(const int16_t* audio_data, size_t num_samples);

// Stop writing and finalize the WAV file
esp_err_t wav_writer_stop_file(void);

// Get current file statistics
esp_err_t wav_writer_get_stats(uint32_t* samples_written, uint32_t* file_size_bytes);

// Check if currently writing a file
bool wav_writer_is_writing(void);

// Deinitialize WAV writer
esp_err_t wav_writer_deinit(void);

#endif // WAV_WRITER_H
