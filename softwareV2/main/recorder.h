#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Recording states
typedef enum {
    RECORDER_STATE_IDLE = 0,
    RECORDER_STATE_RECORDING,
    RECORDER_STATE_STOPPING,
    RECORDER_STATE_ERROR
} recorder_state_t;

// Recording configuration
typedef struct {
    const char *output_path;
    int sample_rate;
    int bits_per_sample;
    int channels;
} recorder_config_t;

// Initialize recorder with configuration
esp_err_t recorder_init(const recorder_config_t *config);

// Start recording
esp_err_t recorder_start(void);

// Stop recording
esp_err_t recorder_stop(void);

// Get current recording state
recorder_state_t recorder_get_state(void);

// Check if recording is active
bool recorder_is_recording(void);

// Get recording statistics
esp_err_t recorder_get_stats(uint32_t *bytes_written, uint32_t *duration_ms);

// Deinitialize recorder
esp_err_t recorder_deinit(void);

#ifdef __cplusplus
}
#endif

