#include "raw_audio_storage.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <errno.h>
#include <unistd.h>

static const char* TAG = "raw_audio_storage";

// Global state
static FILE* s_current_file = NULL;
static bool s_is_recording = false;
static uint32_t s_samples_written = 0;
static uint32_t s_start_timestamp = 0;
static uint32_t s_file_size_bytes = 0;
static raw_audio_header_t s_file_header;

// Sample buffer for efficient writing
static raw_audio_sample_t s_sample_buffer[RAW_AUDIO_BUFFER_SIZE];
static uint32_t s_buffer_index = 0;

esp_err_t raw_audio_storage_init(void) {
    ESP_LOGI(TAG, "Initializing raw audio storage system");
    
    // Reset all state variables
    s_current_file = NULL;
    s_is_recording = false;
    s_samples_written = 0;
    s_start_timestamp = 0;
    s_file_size_bytes = 0;
    s_buffer_index = 0;
    
    // Initialize file header template
    memset(&s_file_header, 0, sizeof(raw_audio_header_t));
    s_file_header.magic_number = RAW_AUDIO_MAGIC_NUMBER;
    s_file_header.version = RAW_AUDIO_VERSION;
    s_file_header.sample_rate = RAW_AUDIO_SAMPLE_RATE;
    
    ESP_LOGI(TAG, "Raw audio storage initialized successfully");
    return ESP_OK;
}

esp_err_t raw_audio_storage_start_recording(const char* filename) {
    if (s_is_recording) {
        ESP_LOGW(TAG, "Already recording, stopping current session first");
        raw_audio_storage_stop_recording();
    }
    
    ESP_LOGI(TAG, "Starting raw audio recording: %s", filename);
    
    // Open file for writing
    s_current_file = fopen(filename, "wb");
    if (!s_current_file) {
        ESP_LOGE(TAG, "Failed to open file for recording: %s (errno: %d)", filename, errno);
        return ESP_FAIL;
    }
    
    // Initialize recording state
    s_is_recording = true;
    s_samples_written = 0;
    s_start_timestamp = esp_timer_get_time() / 1000; // Convert to milliseconds
    s_buffer_index = 0;
    s_file_size_bytes = 0;
    
    // Write file header (will be updated when stopping)
    s_file_header.start_timestamp = s_start_timestamp;
    s_file_header.total_samples = 0;
    s_file_header.end_timestamp = 0;
    
    size_t header_written = fwrite(&s_file_header, 1, sizeof(raw_audio_header_t), s_current_file);
    if (header_written != sizeof(raw_audio_header_t)) {
        ESP_LOGE(TAG, "Failed to write file header");
        fclose(s_current_file);
        s_current_file = NULL;
        s_is_recording = false;
        return ESP_FAIL;
    }
    
    s_file_size_bytes += header_written;
    ESP_LOGI(TAG, "Raw audio recording started successfully");
    return ESP_OK;
}

esp_err_t raw_audio_storage_stop_recording(void) {
    if (!s_is_recording || !s_current_file) {
        ESP_LOGW(TAG, "Not currently recording");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Stopping raw audio recording");
    
    // Flush any remaining samples in buffer
    if (s_buffer_index > 0) {
        ESP_LOGI(TAG, "Flushing %lu samples from buffer", s_buffer_index);
        size_t samples_written = fwrite(s_sample_buffer, sizeof(raw_audio_sample_t), s_buffer_index, s_current_file);
        if (samples_written != s_buffer_index) {
            ESP_LOGW(TAG, "Failed to write all buffered samples (%zu/%lu)", samples_written, s_buffer_index);
        }
        s_samples_written += samples_written;
        s_file_size_bytes += samples_written * sizeof(raw_audio_sample_t);
        s_buffer_index = 0;
    }
    
    // Update file header with final statistics
    s_file_header.total_samples = s_samples_written;
    s_file_header.end_timestamp = esp_timer_get_time() / 1000;
    
    // Seek back to beginning and rewrite header
    if (fseek(s_current_file, 0, SEEK_SET) == 0) {
        size_t header_written = fwrite(&s_file_header, 1, sizeof(raw_audio_header_t), s_current_file);
        if (header_written != sizeof(raw_audio_header_t)) {
            ESP_LOGW(TAG, "Failed to update file header");
        }
    } else {
        ESP_LOGW(TAG, "Failed to seek to file beginning for header update");
    }
    
    // Close file
    fclose(s_current_file);
    s_current_file = NULL;
    s_is_recording = false;
    
    ESP_LOGI(TAG, "Raw audio recording stopped - %lu samples written, %lu bytes total", 
             s_samples_written, s_file_size_bytes);
    return ESP_OK;
}

esp_err_t raw_audio_storage_add_sample(uint16_t mic_adc) {
    if (!s_is_recording || !s_current_file) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Create sample
    raw_audio_sample_t sample;
    sample.mic_sample = mic_adc;
    sample.timestamp_ms = esp_timer_get_time() / 1000;
    sample.sample_count = s_samples_written + s_buffer_index;
    
    // Add to buffer
    s_sample_buffer[s_buffer_index] = sample;
    s_buffer_index++;
    
    // If buffer is full, write to file
    if (s_buffer_index >= RAW_AUDIO_BUFFER_SIZE) {
        size_t samples_written = fwrite(s_sample_buffer, sizeof(raw_audio_sample_t), s_buffer_index, s_current_file);
        if (samples_written != s_buffer_index) {
            ESP_LOGW(TAG, "Failed to write all samples (%zu/%lu)", samples_written, s_buffer_index);
            return ESP_FAIL;
        }
        
        s_samples_written += samples_written;
        s_file_size_bytes += samples_written * sizeof(raw_audio_sample_t);
        s_buffer_index = 0;
        
        // Log progress every 1000 samples
        if (s_samples_written % 1000 == 0) {
            ESP_LOGI(TAG, "Raw audio progress: %lu samples written", s_samples_written);
        }
    }
    
    return ESP_OK;
}

bool raw_audio_storage_is_recording(void) {
    return s_is_recording;
}

esp_err_t raw_audio_storage_get_stats(uint32_t* samples_written, uint32_t* file_size_bytes) {
    if (samples_written) {
        *samples_written = s_samples_written + s_buffer_index;
    }
    if (file_size_bytes) {
        *file_size_bytes = s_file_size_bytes + (s_buffer_index * sizeof(raw_audio_sample_t));
    }
    return ESP_OK;
}

esp_err_t raw_audio_storage_deinit(void) {
    ESP_LOGI(TAG, "Deinitializing raw audio storage");
    
    if (s_is_recording) {
        raw_audio_storage_stop_recording();
    }
    
    // Reset all state
    s_current_file = NULL;
    s_is_recording = false;
    s_samples_written = 0;
    s_start_timestamp = 0;
    s_file_size_bytes = 0;
    s_buffer_index = 0;
    
    ESP_LOGI(TAG, "Raw audio storage deinitialized");
    return ESP_OK;
}
