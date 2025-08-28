#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "ui.h"
#include "sd_storage.h"
#include "audio_capture.h"
#include "raw_audio_storage.h"
#include "esp_timer.h"
#include <errno.h>
#include <dirent.h>
#include <string.h>

static const char *TAG = "salestag-sd";

#define BTN_GPIO 4
#define LED_GPIO 40
#define DEBOUNCE_MS 50

// Global state for recording
static int s_recording_count = 0;
static bool s_audio_capture_enabled = false;
static bool s_is_recording = false;
static char s_current_raw_file[128] = {0};


// Raw ADC callback function for direct storage
static void raw_adc_callback(uint16_t mic1_adc, uint16_t mic2_adc, void *user_ctx) {
    (void)user_ctx;  // Unused
    
    if (s_is_recording) {
        esp_err_t ret = raw_audio_storage_add_sample(mic1_adc, mic2_adc);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to add raw audio sample");
        }
    }
}



// Button callback function - Toggle Recording (Option A)
static void button_callback(bool pressed, uint32_t timestamp_ms, void *ctx) {
    (void)ctx;  // Unused
    
    ESP_LOGI(TAG, "=== BUTTON CALLBACK === Button %s at %lu ms", pressed ? "PRESSED" : "RELEASED", timestamp_ms);
    
    // LED shows RECORDING state, not button state
    // (LED will be controlled by recording logic below)
    

    
    // When button is pressed, handle recording
    if (pressed && sd_storage_is_available()) {
        s_recording_count++;
        
        // Check if this is a long press (for power cycling)
        static uint32_t button_press_start = 0;
        uint32_t current_time = esp_timer_get_time() / 1000; // Convert to milliseconds
        
        if (button_press_start == 0) {
            button_press_start = current_time;
        }
        
        // If button has been held for more than 3 seconds, trigger power cycling (DISABLED - causes crashes)
        if ((current_time - button_press_start) > 3000) {
            ESP_LOGI(TAG, "Long button press detected - SD card power cycle DISABLED (causes crashes)");
            // TODO: Fix race condition in sd_storage_power_cycle()
            // esp_err_t power_cycle_ret = sd_storage_power_cycle();
            // if (power_cycle_ret == ESP_OK) {
            //     ESP_LOGI(TAG, "Manual SD card power cycle successful");
            //     // Reset button press timer
            //     button_press_start = 0;
            //     return; // Don't proceed with normal recording
            // } else {
            //     ESP_LOGE(TAG, "Manual SD card power cycle failed: %s", esp_err_to_name(power_cycle_ret));
            // }
        }
        
        // === TOGGLE RECORDING LOGIC (Option A) ===
        if (s_audio_capture_enabled) {
            if (!s_is_recording) {
                // START RECORDING
                s_recording_count++;
                snprintf(s_current_raw_file, sizeof(s_current_raw_file), "/sdcard/r%03d.raw", s_recording_count);
                
                ESP_LOGI(TAG, "🎤 Starting audio recording: %s", s_current_raw_file);
                esp_err_t ret = raw_audio_storage_start_recording(s_current_raw_file);
                
                if (ret == ESP_OK) {
                    ret = audio_capture_start();  // Actually start ADC sampling!
                    if (ret == ESP_OK) {
                        s_is_recording = true;
                        ui_set_led(true);  // LED ON = Recording
                        ESP_LOGI(TAG, "✅ Recording started successfully");
                        return; // Skip file creation logic below
                    } else {
                        ESP_LOGE(TAG, "❌ Failed to start audio capture: %s", esp_err_to_name(ret));
                        raw_audio_storage_stop_recording(); // Clean up file
                    }
                } else {
                    ESP_LOGE(TAG, "❌ Failed to start recording storage: %s", esp_err_to_name(ret));
                }
            } else {
                // STOP RECORDING
                ESP_LOGI(TAG, "⏹️ Stopping audio recording...");
                audio_capture_stop();  // Stop ADC sampling first
                esp_err_t ret = raw_audio_storage_stop_recording();
                
                if (ret == ESP_OK) {
                    s_is_recording = false;
                    ui_set_led(false); // LED OFF = Not recording
                    ESP_LOGI(TAG, "✅ Recording stopped: %s", s_current_raw_file);
                    return; // Skip file creation logic below
                } else {
                    ESP_LOGE(TAG, "❌ Failed to stop recording: %s", esp_err_to_name(ret));
                }
            }
        }
        
        // If audio capture is disabled, just toggle LED
        if (!s_audio_capture_enabled) {
            static bool led_state = false;
            led_state = !led_state;
            ui_set_led(led_state);
            ESP_LOGI(TAG, "💡 LED toggled %s (audio capture disabled)", led_state ? "ON" : "OFF");
        }
    } else if (pressed && !sd_storage_is_available()) {
        // SD card not available - simple LED toggle mode
        static bool led_state = false;
        led_state = !led_state; // Toggle LED state
        ui_set_led(led_state);
        ESP_LOGI(TAG, "💡 LED toggled %s (SD card not available)", led_state ? "ON" : "OFF");
    } else if (!pressed) {
        // Button released - reset long press timer handled in press logic
        ESP_LOGD(TAG, "Button released - reset long press timer");
        
        // Only update LED to reflect recording state when SD card is available
        // When SD card unavailable, leave LED in toggle state set by button press
        if (sd_storage_is_available()) {
            ui_set_led(s_is_recording);
        }
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "=== SalesTag SD Storage Test ===");
    ESP_LOGI(TAG, "BOOT: Testing UI module + SD card storage...");
    
    // Initialize UI module with proper debouncing
    esp_err_t ret = ui_init(BTN_GPIO, LED_GPIO, DEBOUNCE_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize UI module: %s", esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI(TAG, "UI module initialized successfully:");
    ESP_LOGI(TAG, "  Button: GPIO[%d] (pullup enabled, %dms debounce)", BTN_GPIO, DEBOUNCE_MS);
    ESP_LOGI(TAG, "  LED: GPIO[%d] (output mode)", LED_GPIO);
    
    // Initialize SD card storage
    ESP_LOGI(TAG, "Initializing SD card storage...");
    ret = sd_storage_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "SD card initialization failed: %s", esp_err_to_name(ret));
        ESP_LOGW(TAG, "Continuing without SD card - button will still control LED");
    } else {
        ESP_LOGI(TAG, "SD card storage initialized successfully");
        
        // Get SD card information
        sd_info_t sd_info;
        if (sd_storage_get_info(&sd_info) == ESP_OK) {
            ESP_LOGI(TAG, "SD Card Info:");
            ESP_LOGI(TAG, "  Status: %s", sd_info.is_mounted ? "MOUNTED" : "UNMOUNTED");
            ESP_LOGI(TAG, "  Total: %llu bytes", sd_info.total_bytes);
            ESP_LOGI(TAG, "  Available: %s", sd_storage_is_available() ? "YES" : "NO");
        }
    }
    
    ESP_LOGI(TAG, "Continuing with UI setup...");
    
    // Set button callback
    ui_set_button_callback(button_callback, NULL);
    ESP_LOGI(TAG, "Button callback registered");
    
    // Start with LED OFF (not recording initially)
    ui_set_led(s_is_recording);  // Will be false initially
    ESP_LOGI(TAG, "LED initialized to reflect recording state: %s", s_is_recording ? "ON" : "OFF");
    
    ESP_LOGI(TAG, "=== UI System Ready ===");
    ESP_LOGI(TAG, "Button and LED functionality confirmed working");
    
    // NOW initialize audio capture after UI is working
    ESP_LOGI(TAG, "Initializing audio capture...");
    ret = audio_capture_init(1000, 2);   // 1kHz, stereo (FreeRTOS tick-limited)
    if (ret == ESP_OK) {
        s_audio_capture_enabled = true;
        ESP_LOGI(TAG, "Audio capture initialized successfully");
        ESP_LOGI(TAG, "  Real audio recording ENABLED");
        ESP_LOGI(TAG, "  Microphones: GPIO9 (MIC1), GPIO12 (MIC2)");
        
        // Initialize raw audio storage system
        ESP_LOGI(TAG, "Initializing raw audio storage system...");
        esp_err_t raw_ret = raw_audio_storage_init();
        if (raw_ret == ESP_OK) {
            ESP_LOGI(TAG, "Raw audio storage initialized successfully");
            // Register raw ADC callback for direct storage
            audio_capture_set_raw_adc_callback(raw_adc_callback, NULL);
            ESP_LOGI(TAG, "Raw ADC callback registered - direct ADC storage enabled");
        } else {
            ESP_LOGE(TAG, "Failed to initialize raw audio storage: %s", esp_err_to_name(raw_ret));
        }
        

        
        // Reassert button config after audio init (ChatGPT 5 fix)
        ESP_LOGI(TAG, "Reasserting button config after audio init");
        gpio_config_t btn_config = {
            .pin_bit_mask = 1ULL << BTN_GPIO,
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&btn_config);
        
        // Wait for GPIO to stabilize and check level
        vTaskDelay(pdMS_TO_TICKS(100));
        int gpio_level = gpio_get_level(BTN_GPIO);
        ESP_LOGI(TAG, "GPIO[%d] level post-reassert: %d", BTN_GPIO, gpio_level);
        
        // If still stuck LOW after reassert, log warning
        if (gpio_level == 0) {
            ESP_LOGW(TAG, "GPIO[%d] still stuck LOW after config reassert - may be hardware issue", BTN_GPIO);
        }
    } else {
        ESP_LOGW(TAG, "Audio capture initialization failed: %s", esp_err_to_name(ret));
        ESP_LOGW(TAG, "Continuing without audio - will create test files only");
        s_audio_capture_enabled = false;
    }
    
    ESP_LOGI(TAG, "=== System Ready ===");
    ESP_LOGI(TAG, "Button Functions:");
    if (sd_storage_is_available()) {
        if (s_audio_capture_enabled) {
            ESP_LOGI(TAG, "  📱 Short press: Toggle audio recording ON/OFF");
            ESP_LOGI(TAG, "  💡 LED ON = Recording, LED OFF = Stopped");
            ESP_LOGI(TAG, "  🔄 Long press (3s): SD card power cycle");
        } else {
            ESP_LOGI(TAG, "  📄 Short press: Create test file on SD card");  
            ESP_LOGI(TAG, "  🔄 Long press (3s): SD card power cycle");
        }
    } else {
        ESP_LOGI(TAG, "  💡 Press button to turn LED ON/OFF");
        ESP_LOGI(TAG, "  ❌ (SD card not available)");
    }
    
    // Main application loop - just keep the system running
    while (true) {
        // UI module handles button polling in background task
        // Just keep main application alive
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_LOGD(TAG, "Main loop heartbeat");
        
        // Periodically test SD card availability and show PSRAM stats
        static int heartbeat_count = 0;
        heartbeat_count++;
        if (heartbeat_count % 10 == 0) { // Every 10 seconds
            
                                        // Show raw audio storage statistics
            uint32_t samples_written, file_size_bytes;
            if (raw_audio_storage_get_stats(&samples_written, &file_size_bytes) == ESP_OK) {
                ESP_LOGI(TAG, "Raw Audio Stats - Samples: %lu, File Size: %lu bytes", samples_written, file_size_bytes);
            }
            

            
            // Show system status every 30 seconds
            if (heartbeat_count % 30 == 0) {
                ESP_LOGI(TAG, "=== System Status ===");
                ESP_LOGI(TAG, "Recording: %s", s_is_recording ? "ACTIVE" : "IDLE");
                ESP_LOGI(TAG, "Audio Capture: %s", s_audio_capture_enabled ? "ENABLED" : "DISABLED");
                ESP_LOGI(TAG, "SD Card: %s", sd_storage_is_available() ? "AVAILABLE" : "NOT AVAILABLE");
                ESP_LOGI(TAG, "=== End System Status ===");
            }
        }
    }
}
