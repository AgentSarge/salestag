#include "raw_audio_storage.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdatomic.h>

static const char* TAG = "raw_audio_storage";

// ADC corruption counters (atomic for thread safety)
static atomic_uint_fast32_t g_adc_oob_count = 0;
static atomic_uint_fast32_t g_adc_ffff_count = 0;
static atomic_uint_fast32_t g_sample_seq = 0;

// Global state
static int s_current_fd = -1;  // File descriptor instead of FILE*
static bool s_is_recording = false;
static uint32_t s_samples_written = 0;
static uint32_t s_start_timestamp = 0;
static uint32_t s_file_size_bytes = 0;
static raw_audio_header_t s_file_header;

// Sample buffer for efficient writing
static raw_audio_sample_t s_sample_buffer[RAW_AUDIO_BUFFER_SIZE];
static uint32_t s_buffer_index = 0;

// Helper functions
static inline void put_u32_le(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v; 
    p[1] = (uint8_t)(v >> 8); 
    p[2] = (uint8_t)(v >> 16); 
    p[3] = (uint8_t)(v >> 24);
}

static inline uint16_t sanitize_adc(uint16_t v) {
    if (v == 0xFFFF) {
        atomic_fetch_add(&g_adc_ffff_count, 1);
        ESP_LOGW(TAG, "⚠️ 0xFFFF corruption detected, using neutral sample");
        return 2048; // neutral sample
    }
    if (v > 4095) {
        atomic_fetch_add(&g_adc_oob_count, 1);
        ESP_LOGW(TAG, "⚠️ ADC out of range: %d, clamping to 4095", v);
        return 4095;
    }
    return v;
}

static void raw_header_fill(uint8_t *buf, uint32_t total, uint32_t start_ms, uint32_t end_ms) {
    put_u32_le(buf + 0,  0x52415741);  // "RAWA"
    put_u32_le(buf + 4,  1);           // version
    put_u32_le(buf + 8,  16000);       // sample_rate
    put_u32_le(buf + 12, total);       // total_samples
    put_u32_le(buf + 16, start_ms);    // start_timestamp
    put_u32_le(buf + 20, end_ms);      // end_timestamp
    for (int i = 0; i < 4; i++) {
        put_u32_le(buf + 24 + 4*i, 0); // reserved
    }
}

esp_err_t raw_audio_storage_init(void) {
    ESP_LOGI(TAG, "Initializing raw audio storage system");
    
    // Reset all state variables
    s_current_fd = -1;
    s_is_recording = false;
    s_samples_written = 0;
    s_start_timestamp = 0;
    s_file_size_bytes = 0;
    s_buffer_index = 0;
    
    // Initialize file header template with explicit little-endian writes
    memset(&s_file_header, 0, sizeof(raw_audio_header_t));
    s_file_header.magic_number = RAW_AUDIO_MAGIC_NUMBER;  // 0x52415741 = "RAWA"
    s_file_header.version = RAW_AUDIO_VERSION;
    s_file_header.sample_rate = RAW_AUDIO_SAMPLE_RATE;
    
    // Validate header structure integrity at startup
    ESP_LOGI(TAG, "Header validation: magic=0x%08X, size=%d bytes", 
             RAW_AUDIO_MAGIC_NUMBER, sizeof(raw_audio_header_t));
    
    // Verify magic number bytes for endianness debugging
    uint8_t *magic_bytes = (uint8_t*)&s_file_header.magic_number;
    ESP_LOGI(TAG, "Magic bytes: %02X %02X %02X %02X (should be 41 57 41 52 for RAWA)", 
             magic_bytes[0], magic_bytes[1], magic_bytes[2], magic_bytes[3]);
    
    ESP_LOGI(TAG, "Raw audio storage initialized successfully");
    return ESP_OK;
}

esp_err_t raw_audio_storage_start_recording(const char* filename) {
    if (s_is_recording) {
        ESP_LOGW(TAG, "Already recording, stopping current session first");
        raw_audio_storage_stop_recording();
    }
    
    ESP_LOGI(TAG, "Starting raw audio recording: %s", filename);
    
    // Open file for writing using low-level API
    s_current_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (s_current_fd < 0) {
        ESP_LOGE(TAG, "Failed to open file for recording: %s (errno: %d)", filename, errno);
        return ESP_FAIL;
    }
    
    // Initialize recording state
    s_is_recording = true;
    s_samples_written = 0;
    s_start_timestamp = esp_timer_get_time() / 1000; // Convert to milliseconds
    s_buffer_index = 0;
    s_file_size_bytes = 0;
    
    // Write file header using explicit little-endian format
    uint8_t header_buf[32];
    raw_header_fill(header_buf, 0, s_start_timestamp, 0);  // total_samples=0, end_timestamp=0 for now
    
    ssize_t header_written = write(s_current_fd, header_buf, 32);
    if (header_written != 32) {
        ESP_LOGE(TAG, "Failed to write file header (errno: %d)", errno);
        close(s_current_fd);
        s_current_fd = -1;
        s_is_recording = false;
        return ESP_FAIL;
    }
    
    // Verify header was written correctly
    ESP_LOGI(TAG, "Header written: magic bytes should be 41 57 41 52");
    ESP_LOGI(TAG, "Actual header bytes: %02X %02X %02X %02X", 
             header_buf[0], header_buf[1], header_buf[2], header_buf[3]);
    
    s_file_size_bytes += header_written;
    ESP_LOGI(TAG, "Raw audio recording started successfully");
    return ESP_OK;
}

esp_err_t raw_audio_storage_stop_recording(void) {
    if (!s_is_recording || s_current_fd < 0) {
        ESP_LOGW(TAG, "Not currently recording");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Stopping raw audio recording");

    // CRITICAL: Set recording flag to false FIRST to stop storage task
    s_is_recording = false;

    // Give storage task a moment to finish processing any queued samples
    vTaskDelay(pdMS_TO_TICKS(50));

    // Now safely flush any remaining samples in buffer
    if (s_buffer_index > 0) {
        ESP_LOGI(TAG, "Flushing %lu samples from buffer", s_buffer_index);
        ssize_t bytes_written = write(s_current_fd, s_sample_buffer, s_buffer_index * sizeof(raw_audio_sample_t));
        if (bytes_written != (ssize_t)(s_buffer_index * sizeof(raw_audio_sample_t))) {
            ESP_LOGW(TAG, "Failed to write all buffered samples (%zd/%lu)", bytes_written, s_buffer_index * sizeof(raw_audio_sample_t));
        } else {
            s_samples_written += s_buffer_index;
            s_file_size_bytes += bytes_written;
        }
        s_buffer_index = 0;
    }
    
    // Update file header with final statistics using explicit little-endian format
    uint32_t end_timestamp = esp_timer_get_time() / 1000;
    uint8_t final_header[32];
    raw_header_fill(final_header, s_samples_written, s_start_timestamp, end_timestamp);
    
    // Seek back to beginning and rewrite header
    if (lseek(s_current_fd, 0, SEEK_SET) == 0) {
        ssize_t header_written = write(s_current_fd, final_header, 32);
        if (header_written != 32) {
            ESP_LOGW(TAG, "Failed to update file header (errno: %d)", errno);
        } else {
            ESP_LOGI(TAG, "Final header updated: %lu samples, %lu->%lu ms", 
                     s_samples_written, s_start_timestamp, end_timestamp);
        }
    } else {
        ESP_LOGW(TAG, "Failed to seek to file beginning for header update (errno: %d)", errno);
    }

    // Close file
    close(s_current_fd);
    s_current_fd = -1;
    s_is_recording = false;
    
    ESP_LOGI(TAG, "Raw audio recording stopped - %lu samples written, %lu bytes total", 
             s_samples_written, s_file_size_bytes);
    return ESP_OK;
}

esp_err_t raw_audio_storage_add_sample(uint16_t mic_adc) {
    if (!s_is_recording || s_current_fd < 0) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Create sample with sanitized ADC value
    raw_audio_sample_t sample;
    sample.mic_sample = sanitize_adc(mic_adc);  // Clamps and counts corruption
    sample.timestamp_ms = esp_timer_get_time() / 1000;
    sample.sample_count = atomic_fetch_add(&g_sample_seq, 1);
    
    // Add to buffer
    s_sample_buffer[s_buffer_index] = sample;
    s_buffer_index++;
    
    // If buffer is full, write to file
    if (s_buffer_index >= RAW_AUDIO_BUFFER_SIZE) {
        ssize_t bytes_written = write(s_current_fd, s_sample_buffer, s_buffer_index * sizeof(raw_audio_sample_t));
        if (bytes_written != (ssize_t)(s_buffer_index * sizeof(raw_audio_sample_t))) {
            ESP_LOGW(TAG, "Failed to write all samples (%zd/%lu) (errno: %d)", bytes_written, s_buffer_index * sizeof(raw_audio_sample_t), errno);
            return ESP_FAIL;
        }

        s_samples_written += s_buffer_index;
        s_file_size_bytes += bytes_written;
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

void raw_audio_storage_get_counters(uint32_t *oob, uint32_t *ffff) {
    if (oob) *oob = atomic_load(&g_adc_oob_count);
    if (ffff) *ffff = atomic_load(&g_adc_ffff_count);
}

void raw_audio_storage_reset_counters(void) {
    atomic_store(&g_adc_oob_count, 0);
    atomic_store(&g_adc_ffff_count, 0);
    atomic_store(&g_sample_seq, 0);
}

esp_err_t raw_audio_storage_deinit(void) {
    ESP_LOGI(TAG, "Deinitializing raw audio storage");
    
    if (s_is_recording) {
        raw_audio_storage_stop_recording();
    }
    
    // Log final corruption counters
    uint32_t oob_count, ffff_count;
    raw_audio_storage_get_counters(&oob_count, &ffff_count);
    ESP_LOGI(TAG, "Final corruption stats - OOB: %lu, 0xFFFF: %lu", oob_count, ffff_count);
    
    // Reset all state
    s_current_fd = -1;
    s_is_recording = false;
    s_samples_written = 0;
    s_start_timestamp = 0;
    s_file_size_bytes = 0;
    s_buffer_index = 0;
    
    ESP_LOGI(TAG, "Raw audio storage deinitialized");
    return ESP_OK;
}
