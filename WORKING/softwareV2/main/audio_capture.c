#include "audio_capture.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include <string.h>

// Hardware configuration - single MAX9814 microphone
#define MIC_ADC_CHANNEL ADC_CHANNEL_3  // GPIO 9 (ADC1_CH3) - MIC_DATA1

// ADC configuration constants
#define ADC_SAMPLE_FREQ_HZ       1000   // Actual rate due to FreeRTOS tick limitations
#define ADC_CHANNELS_COUNT       1      // Mono instead of stereo
#define AUDIO_BUFFER_FRAMES      512
#define SAMPLE_RATE_DELAY_US     (1000000 / ADC_SAMPLE_FREQ_HZ)

static const char *TAG_CAP = "audio_cap";

// State variables
static audio_capture_callback_t s_cb = NULL;
static void *s_cb_ctx = NULL;
static raw_adc_callback_t s_raw_adc_cb = NULL;
static void *s_raw_adc_cb_ctx = NULL;
static TaskHandle_t s_capture_task = NULL;
static adc_oneshot_unit_handle_t s_adc_handle = NULL;
static adc_cali_handle_t s_adc_cali_mic = NULL;
static int s_rate = 16000;
static int s_ch = 1;  // Mono instead of stereo
static volatile bool s_running = false;
static volatile bool s_adc_initialized = false;

// Audio buffer (mono instead of stereo)
static int16_t s_audio_frame_buffer[AUDIO_BUFFER_FRAMES]; // 1 channel

// Forward declarations
static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);
static void adc_calibration_deinit(adc_cali_handle_t handle);

// ADC oneshot sampling task
static void audio_capture_task(void *pvParameters) {
    ESP_LOGI(TAG_CAP, "Audio capture task started");
    
    int raw_mic;
    uint32_t sample_count = 0;
    
    while (s_running) {
        // Sample single microphone
        esp_err_t ret = adc_oneshot_read(s_adc_handle, MIC_ADC_CHANNEL, &raw_mic);
        
        if (ret == ESP_OK) {
            // Call raw ADC callback if registered
            if (s_raw_adc_cb) {
                s_raw_adc_cb((uint16_t)raw_mic, s_raw_adc_cb_ctx);
            }
            
            // Convert raw value to signed 16-bit audio sample
            // 12-bit ADC (0-4095) -> center around 0 (-2048 to +2047) -> scale to 16-bit range
            int32_t centered = (int32_t)raw_mic - 2048; // -2048 to +2047
            int32_t scaled = centered * 8; // Scale by 8x (was 16x - too aggressive, caused clipping)

            // Clamp to prevent clipping (16-bit signed range: -32768 to +32767)
            if (scaled > 32767) scaled = 32767;
            if (scaled < -32768) scaled = -32768;

            int16_t sample = (int16_t)scaled;
            
            // Store mono sample
            s_audio_frame_buffer[sample_count] = sample;
            
            sample_count++;
            
            // When we have enough samples for a frame, call the callback
            if (sample_count >= AUDIO_BUFFER_FRAMES) {
                if (s_cb) {
                    s_cb(s_audio_frame_buffer, AUDIO_BUFFER_FRAMES, s_cb_ctx);
                }
                sample_count = 0;
            }
        } else {
            ESP_LOGW(TAG_CAP, "ADC read failed: %s", esp_err_to_name(ret));
        }
        
        // Simple delay to achieve ~1kHz sampling rate
        // Using vTaskDelay instead of vTaskDelayUntil to avoid timing assertion errors
        vTaskDelay(pdMS_TO_TICKS(1));  // 1ms delay = 1000 Hz max rate
    }
    
    ESP_LOGI(TAG_CAP, "Audio capture task ended");
    vTaskDelete(NULL);
}

esp_err_t audio_capture_init(int sample_rate, int channels) {
    ESP_LOGI(TAG_CAP, "Initializing audio capture (ADC oneshot mode)");
    
    if (s_adc_initialized) {
        ESP_LOGW(TAG_CAP, "Audio capture already initialized");
        return ESP_OK;
    }
    
    s_rate = sample_rate;
    s_ch = channels;
    
    // Initialize ADC oneshot unit
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    
    esp_err_t ret = adc_oneshot_new_unit(&init_config, &s_adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_CAP, "Failed to initialize ADC unit: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Configure ADC channel
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12,
    };
    
    ret = adc_oneshot_config_channel(s_adc_handle, MIC_ADC_CHANNEL, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_CAP, "Failed to configure MIC channel: %s", esp_err_to_name(ret));
        adc_oneshot_del_unit(s_adc_handle);
        return ret;
    }
    
    // Initialize calibration for the channel
    bool cali_enable = adc_calibration_init(ADC_UNIT_1, MIC_ADC_CHANNEL, ADC_ATTEN_DB_12, &s_adc_cali_mic);
    if (!cali_enable) {
        ESP_LOGW(TAG_CAP, "MIC calibration scheme not supported, using raw values");
    }
    
    s_adc_initialized = true;
    
    ESP_LOGI(TAG_CAP, "Audio capture initialized successfully");
    ESP_LOGI(TAG_CAP, "  Mode: ADC oneshot (no GPIO conflicts)");
    ESP_LOGI(TAG_CAP, "  Sample rate: %d Hz", s_rate);
    ESP_LOGI(TAG_CAP, "  Channels: %d (MIC: GPIO9)", s_ch);
    ESP_LOGI(TAG_CAP, "  Buffer size: %d frames", AUDIO_BUFFER_FRAMES);
    
    return ESP_OK;
}

esp_err_t audio_capture_start(void) {
    if (!s_adc_initialized) {
        ESP_LOGE(TAG_CAP, "Audio capture not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (s_running) {
        ESP_LOGW(TAG_CAP, "Audio capture already running");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG_CAP, "Starting audio capture task");
    
    s_running = true;
    
    // Create capture task with moderate priority (safe for system stability)
    BaseType_t ret = xTaskCreate(
        audio_capture_task,
        "audio_capture",
        4096,
        NULL,
        5, // Moderate priority - won't interfere with system tasks (USB, etc.)
        &s_capture_task
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG_CAP, "Failed to create audio capture task");
        s_running = false;
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG_CAP, "Audio capture started successfully");
    return ESP_OK;
}

esp_err_t audio_capture_stop(void) {
    if (!s_running) {
        ESP_LOGW(TAG_CAP, "Audio capture not running");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG_CAP, "Stopping audio capture");
    
    s_running = false;
    
    // Wait for task to finish
    if (s_capture_task) {
        // Task will delete itself, just wait a bit
        vTaskDelay(pdMS_TO_TICKS(100));
        s_capture_task = NULL;
    }
    
    ESP_LOGI(TAG_CAP, "Audio capture stopped");
    return ESP_OK;
}

void audio_capture_deinit(void) {
    ESP_LOGI(TAG_CAP, "Deinitializing audio capture");
    
    // Stop capture if running
    audio_capture_stop();
    
    // Clean up calibration
    if (s_adc_cali_mic) {
        adc_calibration_deinit(s_adc_cali_mic);
        s_adc_cali_mic = NULL;
    }
    
    // Clean up ADC unit
    if (s_adc_handle) {
        adc_oneshot_del_unit(s_adc_handle);
        s_adc_handle = NULL;
    }
    
    s_adc_initialized = false;
    s_cb = NULL;
    s_cb_ctx = NULL;
    
    ESP_LOGI(TAG_CAP, "Audio capture deinitialized");
}

void audio_capture_set_callback(audio_capture_callback_t cb, void *user_ctx) {
    s_cb = cb;
    s_cb_ctx = user_ctx;
    ESP_LOGI(TAG_CAP, "Audio callback registered: %p", cb);
}

bool audio_capture_is_running(void) {
    return s_running;
}

// Calibration helper functions
static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle) {
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGD(TAG_CAP, "Calibration scheme: Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_12,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGD(TAG_CAP, "Calibration scheme: Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_12,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGD(TAG_CAP, "Calibration success for channel %d", channel);
    } else {
        ESP_LOGW(TAG_CAP, "Calibration failed for channel %d", channel);
    }
    
    return calibrated;
}

static void adc_calibration_deinit(adc_cali_handle_t handle) {
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGD(TAG_CAP, "Deregister curve fitting calibration scheme");
    adc_cali_delete_scheme_curve_fitting(handle);
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGD(TAG_CAP, "Deregister line fitting calibration scheme");
    adc_cali_delete_scheme_line_fitting(handle);
#endif
}

void audio_capture_set_raw_adc_callback(raw_adc_callback_t cb, void *user_ctx) {
    s_raw_adc_cb = cb;
    s_raw_adc_cb_ctx = user_ctx;
    ESP_LOGI(TAG_CAP, "Raw ADC callback registered: %p", cb);
}

esp_err_t audio_capture_read_raw_adc(uint16_t *mic_adc) {
    if (!s_adc_initialized || !s_adc_handle) {
        ESP_LOGE(TAG_CAP, "ADC not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    int raw_mic;
    esp_err_t ret = adc_oneshot_read(s_adc_handle, MIC_ADC_CHANNEL, &raw_mic);
    
    if (ret == ESP_OK) {
        if (mic_adc) *mic_adc = (uint16_t)raw_mic;
        return ESP_OK;
    } else {
        ESP_LOGE(TAG_CAP, "ADC read failed: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }
}