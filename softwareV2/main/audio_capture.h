#pragma once
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*audio_capture_callback_t)(const int16_t *interleaved_frames, size_t num_frames, void *user_ctx);

esp_err_t audio_capture_init(int sample_rate_hz, int channels);
void audio_capture_set_callback(audio_capture_callback_t cb, void *user_ctx);
esp_err_t audio_capture_start(void);
esp_err_t audio_capture_stop(void);
void audio_capture_deinit(void);

#ifdef __cplusplus
}
#endif
