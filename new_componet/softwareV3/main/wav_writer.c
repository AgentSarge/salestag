#include "wav_writer.h"
#include "esp_log.h"
#include <string.h>
#include <errno.h>

static const char* TAG = "wav_writer";

// Global state
static FILE* s_current_file = NULL;
static bool s_is_writing = false;
static uint32_t s_samples_written = 0;
static uint32_t s_data_bytes = 0;
static wav_header_t s_wav_header;

esp_err_t wav_writer_init(void) {
    ESP_LOGI(TAG, "Initializing WAV writer");
    
    // Reset state
    s_current_file = NULL;
    s_is_writing = false;
    s_samples_written = 0;
    s_data_bytes = 0;
    
    ESP_LOGI(TAG, "WAV writer initialized");
    ESP_LOGI(TAG, "  Format: Mono, 16kHz, 16-bit PCM");
    ESP_LOGI(TAG, "  Data rate: %d bytes/second", WAV_BYTE_RATE);
    
    return ESP_OK;
}

esp_err_t wav_writer_start_file(const char* filename) {
    if (s_is_writing) {
        ESP_LOGW(TAG, "Already writing, stopping current file first");
        wav_writer_stop_file();
    }
    
    ESP_LOGI(TAG, "Starting WAV file: %s", filename);
    
    // Open file for writing
    s_current_file = fopen(filename, "wb");
    if (!s_current_file) {
        ESP_LOGE(TAG, "Failed to open WAV file: %s (errno: %d)", filename, errno);
        return ESP_FAIL;
    }
    
    // Initialize WAV header
    memset(&s_wav_header, 0, sizeof(wav_header_t));
    
    // RIFF header
    memcpy(s_wav_header.riff_header, "RIFF", 4);
    s_wav_header.wav_size = 0; // Will be updated when stopping
    memcpy(s_wav_header.wave_header, "WAVE", 4);
    
    // Format chunk
    memcpy(s_wav_header.fmt_header, "fmt ", 4);
    s_wav_header.fmt_chunk_size = 16;
    s_wav_header.audio_format = 1; // PCM
    s_wav_header.num_channels = WAV_CHANNELS; // 1 for mono
    s_wav_header.sample_rate = WAV_SAMPLE_RATE; // 16000 Hz
    s_wav_header.byte_rate = WAV_BYTE_RATE;
    s_wav_header.sample_alignment = WAV_BYTES_PER_FRAME;
    s_wav_header.bit_depth = WAV_BIT_DEPTH; // 16 bits
    
    // Data chunk
    memcpy(s_wav_header.data_header, "data", 4);
    s_wav_header.data_bytes = 0; // Will be updated when stopping
    
    // Write initial header (will be updated when stopping)
    size_t header_written = fwrite(&s_wav_header, 1, sizeof(wav_header_t), s_current_file);
    if (header_written != sizeof(wav_header_t)) {
        ESP_LOGE(TAG, "Failed to write WAV header");
        fclose(s_current_file);
        s_current_file = NULL;
        return ESP_FAIL;
    }
    
    // Initialize state
    s_is_writing = true;
    s_samples_written = 0;
    s_data_bytes = 0;
    
    ESP_LOGI(TAG, "WAV file started successfully");
    return ESP_OK;
}

esp_err_t wav_writer_write_audio_data(const int16_t* audio_data, size_t num_samples) {
    if (!s_is_writing || !s_current_file) {
        ESP_LOGE(TAG, "Not currently writing WAV file");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Calculate bytes to write
    size_t bytes_to_write = num_samples * WAV_BYTES_PER_FRAME;
    
    // Write audio data
    size_t bytes_written = fwrite(audio_data, 1, bytes_to_write, s_current_file);
    if (bytes_written != bytes_to_write) {
        ESP_LOGE(TAG, "Failed to write audio data: %zu/%zu bytes", bytes_written, bytes_to_write);
        return ESP_FAIL;
    }
    
    // Update statistics
    s_samples_written += num_samples;
    s_data_bytes += bytes_written;
    
    // Log progress every 1000 samples
    if (s_samples_written % 1000 == 0) {
        ESP_LOGI(TAG, "WAV progress: %lu samples, %lu bytes", s_samples_written, s_data_bytes);
    }
    
    return ESP_OK;
}

esp_err_t wav_writer_stop_file(void) {
    if (!s_is_writing || !s_current_file) {
        ESP_LOGW(TAG, "Not currently writing WAV file");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Stopping WAV file");
    
    // Update WAV header with final statistics
    s_wav_header.wav_size = s_data_bytes + sizeof(wav_header_t) - 8;
    s_wav_header.data_bytes = s_data_bytes;
    
    // Seek back to beginning and rewrite header
    if (fseek(s_current_file, 0, SEEK_SET) == 0) {
        size_t header_written = fwrite(&s_wav_header, 1, sizeof(wav_header_t), s_current_file);
        if (header_written != sizeof(wav_header_t)) {
            ESP_LOGW(TAG, "Failed to update WAV header");
        }
    } else {
        ESP_LOGW(TAG, "Failed to seek to file beginning for header update");
    }
    
    // Close file
    fclose(s_current_file);
    s_current_file = NULL;
    s_is_writing = false;
    
    ESP_LOGI(TAG, "WAV file completed: %lu samples, %lu bytes total", 
             s_samples_written, s_data_bytes);
    
    return ESP_OK;
}

esp_err_t wav_writer_get_stats(uint32_t* samples_written, uint32_t* file_size_bytes) {
    if (samples_written) {
        *samples_written = s_samples_written;
    }
    if (file_size_bytes) {
        *file_size_bytes = s_data_bytes + sizeof(wav_header_t);
    }
    return ESP_OK;
}

bool wav_writer_is_writing(void) {
    return s_is_writing;
}

esp_err_t wav_writer_deinit(void) {
    ESP_LOGI(TAG, "Deinitializing WAV writer");
    
    if (s_is_writing) {
        wav_writer_stop_file();
    }
    
    return ESP_OK;
}
