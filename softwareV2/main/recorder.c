#include "recorder.h"
#include "wav_writer.h"
#include "sd_storage.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include <string.h>
#include <math.h>

// Define M_PI if not available
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const char *TAG = "recorder";

// Recorder state
static recorder_state_t s_state = RECORDER_STATE_IDLE;
static recorder_config_t s_config = {0};
static bool s_initialized = false;
static bool s_microphone_available = false;  // Track if microphone is working

// Recording statistics
static uint32_t s_bytes_written = 0;
static uint32_t s_start_time_ms = 0;
static uint32_t s_duration_ms = 0;

// Task and queue handles
static TaskHandle_t s_recording_task = NULL;
static TaskHandle_t s_audio_task = NULL;
static QueueHandle_t s_command_queue = NULL;

// ADC configuration for microphone
#define MIC1_ADC_CHANNEL ADC_CHANNEL_3  // GPIO 9 (ADC1_CH3) - MIC_DATA1
#define MIC2_ADC_CHANNEL ADC_CHANNEL_6  // GPIO 12 (ADC1_CH6) - MIC_DATA2
#define MIC_ADC_UNIT    ADC_UNIT_1
#define MIC_ADC_ATTEN   ADC_ATTEN_DB_12 // 0-3.3V range (updated from deprecated DB_11)
#define MIC_ADC_WIDTH   ADC_BITWIDTH_12 // 12-bit resolution

// Audio buffer configuration
#define AUDIO_BUFFER_SIZE 512
#define SAMPLE_RATE 16000
#define STEREO_CHANNELS 2  // Dual microphone setup

// ADC handles
static adc_oneshot_unit_handle_t s_adc1_handle = NULL;

// Forward declarations
static void recorder_task(void *pvParameters);
static void audio_generation_task(void *pvParameters);
static void handle_recording_start(void);
static void handle_recording_stop(void);
static esp_err_t init_microphone_adc(void);

// Initialize microphone ADC
static esp_err_t init_microphone_adc(void) {
    ESP_LOGI(TAG, "Initializing dual microphone ADC setup with modern API");
    
    // ADC oneshot unit configuration
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = MIC_ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &s_adc1_handle));
    
    // ADC channel configuration for MIC1
    adc_oneshot_chan_cfg_t config1 = {
        .bitwidth = MIC_ADC_WIDTH,
        .atten = MIC_ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc1_handle, MIC1_ADC_CHANNEL, &config1));
    
    // ADC channel configuration for MIC2
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc1_handle, MIC2_ADC_CHANNEL, &config1));
    
    ESP_LOGI(TAG, "Dual microphone ADC initialized successfully");
    ESP_LOGI(TAG, "  MIC1: GPIO 9 (ADC1_CH3) - MIC_DATA1");
    ESP_LOGI(TAG, "  MIC2: GPIO 12 (ADC1_CH6) - MIC_DATA2");
    return ESP_OK;
}

// Command types for the recording task
typedef enum {
    REC_CMD_START = 0,
    REC_CMD_STOP,
    REC_CMD_EXIT
} recorder_cmd_type_t;

// Command structure
typedef struct {
    recorder_cmd_type_t cmd;
    void *data;
} recorder_cmd_t;

esp_err_t recorder_init(const recorder_config_t *config) {
    if (!config || !config->output_path) {
        ESP_LOGE(TAG, "Invalid configuration");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (s_initialized) {
        ESP_LOGW(TAG, "Recorder already initialized");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing recorder with output: %s", config->output_path);
    
    // Store configuration
    memcpy(&s_config, config, sizeof(recorder_config_t));
    
    // Initialize microphone ADC
    esp_err_t adc_ret = init_microphone_adc();
    if (adc_ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to initialize microphone ADC, continuing with synthetic audio");
        s_microphone_available = false;
    } else {
        s_microphone_available = true;
        ESP_LOGI(TAG, "Microphone ADC initialized successfully - real audio recording enabled");
    }
    
    // Create command queue
    s_command_queue = xQueueCreate(5, sizeof(recorder_cmd_t));
    if (!s_command_queue) {
        ESP_LOGE(TAG, "Failed to create command queue");
        return ESP_ERR_NO_MEM;
    }
    
    // Create recording task
    BaseType_t ret = xTaskCreate(recorder_task, "recorder_task", 4096, NULL, 5, &s_recording_task);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create recording task");
        vQueueDelete(s_command_queue);
        s_command_queue = NULL;
        return ESP_ERR_NO_MEM;
    }
    
    // Create audio generation task
    ret = xTaskCreate(audio_generation_task, "audio_task", 4096, NULL, 4, &s_audio_task);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create audio generation task");
        vTaskDelete(s_recording_task);
        s_recording_task = NULL;
        vQueueDelete(s_command_queue);
        s_command_queue = NULL;
        return ESP_ERR_NO_MEM;
    }
    
    s_initialized = true;
    s_state = RECORDER_STATE_IDLE;
    
    ESP_LOGI(TAG, "Recorder initialized successfully");
    return ESP_OK;
}

esp_err_t recorder_start(void) {
    if (!s_initialized) {
        ESP_LOGE(TAG, "Recorder not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (s_state != RECORDER_STATE_IDLE) {
        ESP_LOGW(TAG, "Recorder not in idle state");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Check if SD card is available
    if (!sd_storage_is_available()) {
        ESP_LOGE(TAG, "SD card not available for recording");
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Starting recording...");
    
    // Send start command to recording task
    recorder_cmd_t cmd = {
        .cmd = REC_CMD_START,
        .data = NULL
    };
    
    if (xQueueSend(s_command_queue, &cmd, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to send start command");
        return ESP_ERR_TIMEOUT;
    }
    
    return ESP_OK;
}

esp_err_t recorder_stop(void) {
    if (!s_initialized) {
        ESP_LOGE(TAG, "Recorder not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (s_state != RECORDER_STATE_RECORDING) {
        ESP_LOGW(TAG, "Recorder not recording");
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Stopping recording...");
    
    // Send stop command to recording task
    recorder_cmd_t cmd = {
        .cmd = REC_CMD_STOP,
        .data = NULL
    };
    
    if (xQueueSend(s_command_queue, &cmd, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to send stop command");
        return ESP_ERR_TIMEOUT;
    }
    
    return ESP_OK;
}

recorder_state_t recorder_get_state(void) {
    return s_state;
}

bool recorder_is_recording(void) {
    return (s_state == RECORDER_STATE_RECORDING);
}

esp_err_t recorder_get_stats(uint32_t *bytes_written, uint32_t *duration_ms) {
    if (!bytes_written || !duration_ms) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *bytes_written = s_bytes_written;
    *duration_ms = s_duration_ms;
    
    return ESP_OK;
}

esp_err_t recorder_deinit(void) {
    if (!s_initialized) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Deinitializing recorder");
    
    // Send exit command to recording task
    if (s_command_queue) {
        recorder_cmd_t cmd = {
            .cmd = REC_CMD_EXIT,
            .data = NULL
        };
        xQueueSend(s_command_queue, &cmd, pdMS_TO_TICKS(100));
    }
    
    // Clean up ADC handles
    if (s_adc1_handle) {
        adc_oneshot_del_unit(s_adc1_handle);
        s_adc1_handle = NULL;
    }
    
    // Clean up tasks
    if (s_recording_task) {
        vTaskDelete(s_recording_task);
        s_recording_task = NULL;
    }
    
    if (s_audio_task) {
        vTaskDelete(s_audio_task);
        s_audio_task = NULL;
    }
    
    // Clean up queue
    if (s_command_queue) {
        vQueueDelete(s_command_queue);
        s_command_queue = NULL;
    }
    
    s_initialized = false;
    s_state = RECORDER_STATE_IDLE;
    s_bytes_written = 0;
    s_duration_ms = 0;
    
    ESP_LOGI(TAG, "Recorder deinitialized");
    return ESP_OK;
}

// Recording task implementation
static void recorder_task(void *pvParameters) {
    (void)pvParameters;
    
    ESP_LOGI(TAG, "Recording task started");
    
    recorder_cmd_t cmd;
    bool exit_task = false;
    
    while (!exit_task) {
        // Wait for command
        if (xQueueReceive(s_command_queue, &cmd, portMAX_DELAY) == pdTRUE) {
            switch (cmd.cmd) {
                case REC_CMD_START:
                    handle_recording_start();
                    break;
                    
                case REC_CMD_STOP:
                    handle_recording_stop();
                    break;
                    
                case REC_CMD_EXIT:
                    exit_task = true;
                    break;
                    
                default:
                    ESP_LOGW(TAG, "Unknown command: %d", cmd.cmd);
                    break;
            }
        }
    }
    
    ESP_LOGI(TAG, "Recording task exiting");
    vTaskDelete(NULL);
}

// Audio generation task implementation
static void audio_generation_task(void *pvParameters) {
    (void)pvParameters;
    
    ESP_LOGI(TAG, "Audio recording task started");
    
    // Debug: Show initial ADC reading
    int initial_adc1 = 0, initial_adc2 = 0;
    if (adc_oneshot_read(s_adc1_handle, MIC1_ADC_CHANNEL, &initial_adc1) == ESP_OK &&
        adc_oneshot_read(s_adc1_handle, MIC2_ADC_CHANNEL, &initial_adc2) == ESP_OK) {
        ESP_LOGI(TAG, "Initial ADC readings: MIC1=%d, MIC2=%d (should be around 2048 for silence)", 
                 initial_adc1, initial_adc2);
    } else {
        ESP_LOGW(TAG, "Failed to read initial ADC values");
    }
    
    while (1) {
        if (s_state == RECORDER_STATE_RECORDING) {
            if (s_microphone_available) {
                // Read real audio data from dual microphones for stereo recording
                int16_t audio_buffer[AUDIO_BUFFER_SIZE * STEREO_CHANNELS]; // Stereo buffer
                
                for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
                    // Read from MIC1 (ADC1_CH3 - GPIO 9)
                    int adc1_raw = 0;
                    esp_err_t ret1 = adc_oneshot_read(s_adc1_handle, MIC1_ADC_CHANNEL, &adc1_raw);
                    if (ret1 != ESP_OK) {
                        ESP_LOGE(TAG, "Failed to read MIC1: %s", esp_err_to_name(ret1));
                        adc1_raw = 2048; // Default to middle value on error
                    }
                    
                    // Read from MIC2 (ADC1_CH6 - GPIO 12) 
                    int adc2_raw = 0;
                    esp_err_t ret2 = adc_oneshot_read(s_adc1_handle, MIC2_ADC_CHANNEL, &adc2_raw);
                    if (ret2 != ESP_OK) {
                        ESP_LOGE(TAG, "Failed to read MIC2: %s", esp_err_to_name(ret2));
                        adc2_raw = 2048; // Default to middle value on error
                    }
                    
                    // Use raw ADC values directly
                    int adc1_value = adc1_raw;
                    int adc2_value = adc2_raw;
                    
                    // Convert 12-bit ADC values (0-4095) to 16-bit audio (-32768 to 32767)
                    // Center at 0 and scale appropriately for stereo
                    int16_t audio_sample_left = (int16_t)((adc1_value - 2048) * 16);  // Left channel
                    int16_t audio_sample_right = (int16_t)((adc2_value - 2048) * 16); // Right channel
                    
                    // Interleave stereo samples: [L, R, L, R, ...]
                    audio_buffer[i * 2] = audio_sample_left;      // Left channel
                    audio_buffer[i * 2 + 1] = audio_sample_right; // Right channel
                    
                    // Debug: Log first few samples
                    if (i < 5) {
                        ESP_LOGI(TAG, "Sample %d: MIC1(ADC=%d, L=%d), MIC2(ADC=%d, R=%d)", 
                                i, adc1_value, audio_sample_left, adc2_value, audio_sample_right);
                    }
                    
                    // Small delay between samples for proper timing
                    vTaskDelay(pdMS_TO_TICKS(1));
                }
                
                // Write stereo audio data to WAV file
                esp_err_t write_ret = wav_writer_write(audio_buffer, sizeof(audio_buffer));
                if (write_ret != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to write stereo audio data: %s", esp_err_to_name(write_ret));
                    // Don't break, just continue to next iteration
                    vTaskDelay(pdMS_TO_TICKS(10));
                    continue;
                }
                
                s_bytes_written += sizeof(audio_buffer);
                ESP_LOGD(TAG, "Stereo audio data written: %lu bytes total", s_bytes_written);
                
            } else {
                // Fallback to synthetic audio if microphone not available
                int16_t audio_buffer[AUDIO_BUFFER_SIZE];
                static float phase = 0.0f;
                
                for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
                    audio_buffer[i] = (int16_t)(sinf(phase) * 2000);
                    phase += 2.0f * M_PI * 440.0f / SAMPLE_RATE; // 440Hz at 16kHz
                }
                
                // Write synthetic audio data to WAV file
                esp_err_t write_ret = wav_writer_write(audio_buffer, sizeof(audio_buffer));
                if (write_ret != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to write synthetic audio data: %s", esp_err_to_name(write_ret));
                    vTaskDelay(pdMS_TO_TICKS(10));
                    continue;
                }
                
                s_bytes_written += sizeof(audio_buffer);
                ESP_LOGD(TAG, "Synthetic audio data written: %lu bytes total", s_bytes_written);
            }
            
            // Small delay to prevent overwhelming the system
            vTaskDelay(pdMS_TO_TICKS(10));
        } else {
            // Not recording, sleep longer to prevent race conditions
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
    
    ESP_LOGI(TAG, "Audio recording task exiting");
    vTaskDelete(NULL);
}

// Handle recording start
static void handle_recording_start(void) {
    ESP_LOGI(TAG, "Starting recording to: %s", s_config.output_path);
    
    // Initialize WAV writer
    esp_err_t ret = wav_writer_begin(s_config.output_path, 
                                    s_config.sample_rate,
                                    s_config.bits_per_sample,
                                    s_config.channels);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize WAV writer: %s", esp_err_to_name(ret));
        s_state = RECORDER_STATE_ERROR;
        return;
    }
    
    // Update state
    s_state = RECORDER_STATE_RECORDING;
    s_start_time_ms = esp_timer_get_time() / 1000; // Convert to milliseconds
    s_bytes_written = 0;
    
    ESP_LOGI(TAG, "Recording started successfully - audio generation task will handle data writing");
}

// Handle recording stop
static void handle_recording_stop(void) {
    ESP_LOGI(TAG, "Stopping recording");
    
    // Finalize WAV file
    esp_err_t ret = wav_writer_end();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to finalize WAV file: %s", esp_err_to_name(ret));
        s_state = RECORDER_STATE_ERROR;
        return;
    }
    
    // Calculate duration
    uint32_t end_time_ms = esp_timer_get_time() / 1000;
    s_duration_ms = end_time_ms - s_start_time_ms;
    
    // Update state
    s_state = RECORDER_STATE_IDLE;
    
    ESP_LOGI(TAG, "Recording stopped. Duration: %lu ms, Bytes: %lu", 
              s_duration_ms, s_bytes_written);
}

