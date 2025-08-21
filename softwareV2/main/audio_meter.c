#include "audio_meter.h"

// Placeholder implementation — wire up I2S ADC later
static float s_last_dbfs = -90.0f;

esp_err_t audio_meter_init(int sample_rate_hz, int bits_per_sample, int channel_count){
    (void)sample_rate_hz; (void)bits_per_sample; (void)channel_count;
    s_last_dbfs = -90.0f;
    return ESP_OK;
}

float audio_meter_read_dbfs(void){
    // TODO: compute from recent PCM; for now return placeholder
    return s_last_dbfs;
}

void audio_meter_deinit(void){
    // TODO: cleanup I2S ADC
}
