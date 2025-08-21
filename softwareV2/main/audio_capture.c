#include "audio_capture.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG_CAP = "audio_cap";
static audio_capture_callback_t s_cb = NULL;
static void *s_cb_ctx = NULL;
static TaskHandle_t s_task = NULL;
static int s_rate = 16000;
static int s_ch = 2;
static volatile bool s_running = false;

static void capture_task(void *arg){
    const size_t frames = 512; // per callback
    int16_t *buf = (int16_t *)heap_caps_malloc(frames * s_ch * sizeof(int16_t), MALLOC_CAP_DEFAULT);
    if (!buf) {
        ESP_LOGE(TAG_CAP, "alloc failed");
        vTaskDelete(NULL);
        return;
    }
    memset(buf, 0, frames * s_ch * sizeof(int16_t)); // silence placeholder
    TickType_t period = pdMS_TO_TICKS((frames * 1000) / s_rate);
    if (period == 0) period = 1;
    while (s_running) {
        if (s_cb) s_cb(buf, frames, s_cb_ctx);
        vTaskDelay(period);
    }
    free(buf);
    vTaskDelete(NULL);
}

esp_err_t audio_capture_init(int sample_rate_hz, int channels){
    s_rate = sample_rate_hz;
    s_ch = channels;
    return ESP_OK;
}

void audio_capture_set_callback(audio_capture_callback_t cb, void *user_ctx){
    s_cb = cb;
    s_cb_ctx = user_ctx;
}

esp_err_t audio_capture_start(void){
    if (s_running) return ESP_OK;
    s_running = true;
    BaseType_t ok = xTaskCreate(capture_task, "cap", 4096, NULL, 5, &s_task);
    return ok == pdPASS ? ESP_OK : ESP_FAIL;
}

esp_err_t audio_capture_stop(void){
    if (!s_running) return ESP_OK;
    s_running = false;
    // task will delete itself
    return ESP_OK;
}

void audio_capture_deinit(void){
    audio_capture_stop();
}
