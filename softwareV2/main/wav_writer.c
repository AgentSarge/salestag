#include "wav_writer.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

static const char *TAG = "wav_writer";
static FILE *s_file = NULL;
static uint32_t s_data_size = 0;
static uint32_t s_sample_rate = 0;
static uint16_t s_bits_per_sample = 0;
static uint16_t s_channels = 0;

// WAV file header structure
typedef struct {
    char riff[4];           // "RIFF"
    uint32_t file_size;     // File size - 8
    char wave[4];           // "WAVE"
    char fmt[4];            // "fmt "
    uint32_t fmt_size;      // Format chunk size
    uint16_t audio_format;  // PCM = 1
    uint16_t num_channels;  // Mono = 1, Stereo = 2
    uint32_t sample_rate;   // 16000, 44100, etc.
    uint32_t byte_rate;     // Sample rate * num channels * bits per sample / 8
    uint16_t block_align;   // Num channels * bits per sample / 8
    uint16_t bits_per_sample; // 8, 16, etc.
    char data[4];           // "data"
    uint32_t data_size;     // Size of audio data
} wav_header_t;

esp_err_t wav_writer_begin(const char *path, int sample_rate, int bits_per_sample, int channels) {
    ESP_LOGI(TAG, "Opening WAV file: %s (rate=%d, bits=%d, ch=%d)", 
             path, sample_rate, bits_per_sample, channels);
    
    s_file = fopen(path, "wb");
    if (!s_file) {
        ESP_LOGE(TAG, "Failed to open file: %s", path);
        return ESP_FAIL;
    }
    
    s_sample_rate = sample_rate;
    s_bits_per_sample = bits_per_sample;
    s_channels = channels;
    s_data_size = 0;
    
    // Write placeholder WAV header
    wav_header_t header;
    memset(&header, 0, sizeof(header));
    
    strcpy(header.riff, "RIFF");
    strcpy(header.wave, "WAVE");
    strcpy(header.fmt, "fmt ");
    strcpy(header.data, "data");
    
    header.fmt_size = 16;
    header.audio_format = 1; // PCM
    header.num_channels = channels;
    header.sample_rate = sample_rate;
    header.bits_per_sample = bits_per_sample;
    header.block_align = channels * bits_per_sample / 8;
    header.byte_rate = sample_rate * header.block_align;
    
    // Write header (data_size will be updated later)
    size_t written = fwrite(&header, 1, sizeof(header), s_file);
    if (written != sizeof(header)) {
        ESP_LOGE(TAG, "Failed to write WAV header");
        fclose(s_file);
        s_file = NULL;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "WAV file opened successfully");
    return ESP_OK;
}

esp_err_t wav_writer_write(const void *data, size_t bytes) {
    if (!s_file) {
        ESP_LOGE(TAG, "No WAV file open");
        return ESP_ERR_INVALID_STATE;
    }
    
    size_t written = fwrite(data, 1, bytes, s_file);
    if (written != bytes) {
        ESP_LOGE(TAG, "Failed to write audio data");
        return ESP_FAIL;
    }
    
    s_data_size += bytes;
    return ESP_OK;
}

esp_err_t wav_writer_end(void) {
    if (!s_file) {
        ESP_LOGE(TAG, "No WAV file open");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Update file size and data size in header
    uint32_t file_size = s_data_size + sizeof(wav_header_t) - 8;
    
    // Seek back to update header
    fseek(s_file, 4, SEEK_SET);
    fwrite(&file_size, 1, 4, s_file);
    
    fseek(s_file, 40, SEEK_SET);
    fwrite(&s_data_size, 1, 4, s_file);
    
    fclose(s_file);
    s_file = NULL;
    
    ESP_LOGI(TAG, "WAV file closed. Total data: %" PRIu32 " bytes", s_data_size);
    return ESP_OK;
}
