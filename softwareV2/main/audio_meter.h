#pragma once
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t audio_meter_init(int sample_rate_hz, int bits_per_sample, int channel_count);
float audio_meter_read_dbfs(void);
void audio_meter_deinit(void);

#ifdef __cplusplus
}
#endif
