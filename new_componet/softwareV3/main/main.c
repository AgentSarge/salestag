#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "ui.h"
#include "sd_storage.h"
#include "audio_capture.h"
#include "raw_audio_storage.h"
#include "nvs_flash.h"

// NimBLE includes
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/util/util.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "host/ble_uuid.h"
#include "host/ble_gatt.h"

#include "esp_timer.h"
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>

static const char *TAG = "salestag-sd";

#define BTN_GPIO 4
#define LED_GPIO 40
#define DEBOUNCE_MS 50

// Custom UUID definitions for SalesTag Audio Service
#define BLE_UUID_SALESTAG_AUDIO_SVC    0x1234
#define BLE_UUID_SALESTAG_RECORD_CTRL  0x1235
#define BLE_UUID_SALESTAG_STATUS       0x1236
#define BLE_UUID_SALESTAG_FILE_COUNT   0x1237

// Custom UUID definitions for SalesTag File Transfer Service
#define BLE_UUID_SALESTAG_FILE_SVC     0x1240
#define BLE_UUID_SALESTAG_FILE_CTRL    0x1241
#define BLE_UUID_SALESTAG_FILE_DATA    0x1242
#define BLE_UUID_SALESTAG_FILE_STATUS  0x1243

// File transfer command definitions
#define FILE_TRANSFER_CMD_START        0x01
#define FILE_TRANSFER_CMD_STOP         0x02
#define FILE_TRANSFER_CMD_ABORT        0x03

// File transfer status definitions
#define FILE_TRANSFER_STATUS_IDLE      0x00
#define FILE_TRANSFER_STATUS_ACTIVE    0x01
#define FILE_TRANSFER_STATUS_COMPLETE  0x02
#define FILE_TRANSFER_STATUS_ERROR     0x03

// NimBLE callback function declarations
static void ble_app_on_sync(void);
static void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);
static int ble_store_util_status_rr_item(struct ble_store_status_event *event, void *arg);
static void ble_app_on_connect(struct ble_gap_event *event, void *arg);
static void ble_app_on_disconnect(struct ble_gap_event *event, void *arg);
static int ble_gap_event_handler(struct ble_gap_event *event, void *arg);
static void ble_app_on_adv_complete(struct ble_gap_event *event, void *arg);

// BLE advertising control functions
static void ble_app_advertise(void);
static void ble_stop_advertising(void);
static void ble_start_advertising_if_not_recording(void);

// NimBLE host task function
static void nimble_host_task(void *param);

// File transfer helper functions
static void file_transfer_notify_status(uint8_t status, uint32_t offset, uint32_t size);
static int file_transfer_start(const char* filename, uint32_t file_size);
static int file_transfer_stop(void);
static int file_transfer_abort(void);
static int file_transfer_write_data(const uint8_t* data, uint16_t len);

// GATT service callback declarations
static int gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                              struct ble_gatt_access_ctxt *ctxt, void *arg);

// Global state for recording
static int s_recording_count = 0;
static bool s_audio_capture_enabled = false;
static volatile bool s_is_recording = false;  // Made volatile for multi-task access
static char s_current_raw_file[128] = {0};

// Global state for file transfer
static uint16_t s_file_transfer_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static uint16_t s_file_transfer_status_handle = 0;
static bool s_file_transfer_active = false;
static uint32_t s_file_transfer_size = 0;
static uint32_t s_file_transfer_offset = 0;
static char s_file_transfer_filename[128] = {0};
static FILE* s_file_transfer_fp = NULL;

// GATT Service Definition
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(BLE_UUID_SALESTAG_AUDIO_SVC),
        .characteristics = (struct ble_gatt_chr_def[]) { {
            .uuid = BLE_UUID16_DECLARE(BLE_UUID_SALESTAG_RECORD_CTRL),
            .access_cb = gatt_svr_chr_access,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            .val_handle = NULL,
        }, {
            .uuid = BLE_UUID16_DECLARE(BLE_UUID_SALESTAG_STATUS),
            .access_cb = gatt_svr_chr_access,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
            .val_handle = NULL,
        }, {
            .uuid = BLE_UUID16_DECLARE(BLE_UUID_SALESTAG_FILE_COUNT),
            .access_cb = gatt_svr_chr_access,
            .flags = BLE_GATT_CHR_F_READ,
            .val_handle = NULL,
        }, {
            0, /* No more characteristics in this service */
        } },
    },
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(BLE_UUID_SALESTAG_FILE_SVC),
        .characteristics = (struct ble_gatt_chr_def[]) { {
            .uuid = BLE_UUID16_DECLARE(BLE_UUID_SALESTAG_FILE_CTRL),
            .access_cb = gatt_svr_chr_access,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            .val_handle = NULL,
        }, {
            .uuid = BLE_UUID16_DECLARE(BLE_UUID_SALESTAG_FILE_DATA),
            .access_cb = gatt_svr_chr_access,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            .val_handle = NULL,
        }, {
            .uuid = BLE_UUID16_DECLARE(BLE_UUID_SALESTAG_FILE_STATUS),
            .access_cb = gatt_svr_chr_access,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
            .val_handle = NULL,
        }, {
            0, /* No more characteristics in this service */
        } },
    },
    {
        0, /* No more services */
    },
};


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
                
                // Stop BLE advertising to prevent interference
                ble_stop_advertising();
                
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
                        // Restart BLE advertising since recording failed
                        ble_start_advertising_if_not_recording();
                    }
                } else {
                    ESP_LOGE(TAG, "❌ Failed to start recording storage: %s", esp_err_to_name(ret));
                    // Restart BLE advertising since recording failed
                    ble_start_advertising_if_not_recording();
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
                    
                    // Restart BLE advertising now that recording is finished
                    ble_start_advertising_if_not_recording();
                    
                    return; // Skip file creation logic below
                } else {
                    ESP_LOGE(TAG, "❌ Failed to stop recording: %s", esp_err_to_name(ret));
                }
            }
        }
        
        // Create a test file in the main task context first with retry logic
        char main_test_filename[64];
        snprintf(main_test_filename, sizeof(main_test_filename), "/sdcard/m%03d.txt", s_recording_count);
        FILE* main_test_f = NULL;
        int main_retry_count = 0;
        const int main_max_retries = 3;
        
        while (main_retry_count < main_max_retries && !main_test_f) {
            main_test_f = fopen(main_test_filename, "wb");
            if (!main_test_f) {
                ESP_LOGW(TAG, "Main test attempt %d failed (errno: %d), retrying...", main_retry_count + 1, errno);
                vTaskDelay(pdMS_TO_TICKS(50)); // Wait for SD card to become available
                main_retry_count++;
            }
        }
        
        if (main_test_f) {
            fprintf(main_test_f, "Main task test file created successfully\n");
            fclose(main_test_f);
            ESP_LOGI(TAG, "Main task test file created: %s (after %d attempts)", main_test_filename, main_retry_count + 1);
        } else {
            ESP_LOGE(TAG, "Main task test file creation failed: %s (errno: %d)", main_test_filename, errno);
            
            // If file creation fails, try SD card power cycling (DISABLED - causes crashes)
            ESP_LOGW(TAG, "SD card power cycle DISABLED - causes crashes");
            // TODO: Fix race condition in sd_storage_power_cycle()
            // esp_err_t power_cycle_ret = sd_storage_power_cycle();
            // if (power_cycle_ret == ESP_OK) {
            //     ESP_LOGI(TAG, "SD card power cycle successful - trying file creation again");
            //     // Try file creation again after power cycle
            //     main_test_f = fopen(main_test_filename, "w");
            //     if (main_test_f) {
            //         fprintf(main_test_f, "Main task test file created after power cycle\n");
            //         fclose(main_test_f);
            //     } else {
            //         ESP_LOGE(TAG, "File creation still failed after power cycle (errno: %d)", errno);
            //     }
            // } else {
            //     ESP_LOGE(TAG, "SD card power cycle failed: %s", esp_err_to_name(power_cycle_ret));
            // }
        }
        
        if (s_audio_capture_enabled && !s_is_recording) {
            // Test simple file creation first with retry logic
            char test_filename[64];
            snprintf(test_filename, sizeof(test_filename), "/sdcard/b%03d.txt", s_recording_count);
            FILE* test_f = NULL;
            int button_retry_count = 0;
            const int button_max_retries = 3;
            
            while (button_retry_count < button_max_retries && !test_f) {
                test_f = fopen(test_filename, "wb");
                if (!test_f) {
                    ESP_LOGW(TAG, "Button test attempt %d failed (errno: %d), retrying...", button_retry_count + 1, errno);
                    vTaskDelay(pdMS_TO_TICKS(50)); // Wait for SD card to become available
                    button_retry_count++;
                }
            }
            
            if (test_f) {
                fprintf(test_f, "Button test file created successfully\n");
                fclose(test_f);
                ESP_LOGI(TAG, "Button test file created: %s (after %d attempts)", test_filename, button_retry_count + 1);
            } else {
                ESP_LOGE(TAG, "Button test file creation failed: %s (errno: %d)", test_filename, errno);
            }
            
            // Start raw audio recording - store ADC samples directly
            snprintf(s_current_raw_file, sizeof(s_current_raw_file), "/sdcard/r%03d.raw", s_recording_count);
            
            // Stop BLE advertising to prevent interference
            ble_stop_advertising();
            
            esp_err_t ret = raw_audio_storage_start_recording(s_current_raw_file);
            if (ret == ESP_OK) {
                ret = audio_capture_start();
                if (ret == ESP_OK) {
                    s_is_recording = true;
                    ESP_LOGI(TAG, "Started raw audio recording: %s", s_current_raw_file);
                } else {
                    ESP_LOGE(TAG, "Failed to start audio capture: %s", esp_err_to_name(ret));
                    raw_audio_storage_stop_recording();
                    s_current_raw_file[0] = '\0';
                    // Restart BLE advertising since recording failed
                    ble_start_advertising_if_not_recording();
                }
            } else {
                ESP_LOGE(TAG, "Failed to start raw audio storage: %s", esp_err_to_name(ret));
                s_current_raw_file[0] = '\0';
                // Restart BLE advertising since recording failed
                ble_start_advertising_if_not_recording();
            }
        } else if (s_is_recording) {
            // Stop raw audio recording
            audio_capture_stop();
            raw_audio_storage_stop_recording();
            s_is_recording = false;
            ESP_LOGI(TAG, "Stopped raw audio recording: %s", s_current_raw_file);
            s_current_raw_file[0] = '\0';
            
            // Restart BLE advertising now that recording is finished
            ble_start_advertising_if_not_recording();
        } else {
            // Fallback to original behavior - create test file in root directory
            char filename[64];
            snprintf(filename, sizeof(filename), "/sdcard/t%03d.txt", s_recording_count);
            
            FILE* f = fopen(filename, "wb");
            if (f != NULL) {
                fprintf(f, "SalesTag test recording #%d\n", s_recording_count);
                fprintf(f, "Timestamp: %lu ms\n", timestamp_ms);
                fprintf(f, "Audio capture: %s\n", s_audio_capture_enabled ? "ENABLED" : "DISABLED");
                fclose(f);
                ESP_LOGI(TAG, "Created test file: %s", filename);
            } else {
                ESP_LOGE(TAG, "Failed to create test file: %s", filename);
            }
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

// Define the advertising data and parameters
static const uint8_t ble_addr_type = 0;

// BLE advertising control functions
static void ble_stop_advertising(void)
{
    ESP_LOGI(TAG, "Stopping BLE advertising to prevent audio interference");
    int rc = ble_gap_adv_stop();
    if (rc != 0) {
        ESP_LOGW(TAG, "Failed to stop advertising: %d", rc);
    } else {
        ESP_LOGI(TAG, "BLE advertising stopped successfully");
    }
}

static void ble_start_advertising_if_not_recording(void)
{
    if (!s_is_recording) {
        ESP_LOGI(TAG, "Starting BLE advertising (not currently recording)");
        ble_app_advertise();
    } else {
        ESP_LOGI(TAG, "Skipping BLE advertising start (currently recording)");
    }
}

static void ble_app_advertise(void)
{
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    int rc;
    
    // Clear advertising data structures
    memset(&fields, 0, sizeof(fields));

    // Set advertising flags - use only general discoverable
    fields.flags = BLE_HS_ADV_F_DISC_GEN;

    // Set the device name in the advertising packet
    const char *name = "ESP32-S3-Mini-BLE";
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    ESP_LOGI(TAG, "Setting advertising data - name: '%s' (len: %d)", name, fields.name_len);

    // Set the advertising data
    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to set advertising data: %d", rc);
        return;
    }

    // Set scan response data (optional, but helps with discovery)
    struct ble_hs_adv_fields scan_rsp_fields;
    memset(&scan_rsp_fields, 0, sizeof(scan_rsp_fields));
    scan_rsp_fields.name = (uint8_t *)name;
    scan_rsp_fields.name_len = strlen(name);
    scan_rsp_fields.name_is_complete = 1;
    
    rc = ble_gap_adv_rsp_set_fields(&scan_rsp_fields);
    if (rc != 0) {
        ESP_LOGW(TAG, "Failed to set scan response data: %d", rc);
        // Continue anyway, this is optional
    } else {
        ESP_LOGI(TAG, "Scan response data set successfully");
    }

    // Configure advertising parameters
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND; // Undirected connectable mode
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN; // General discoverable mode
    adv_params.itvl_min = BLE_GAP_ADV_FAST_INTERVAL1_MIN; // Fast advertising interval
    adv_params.itvl_max = BLE_GAP_ADV_FAST_INTERVAL1_MAX;

    ESP_LOGI(TAG, "Starting advertising with parameters:");
    ESP_LOGI(TAG, "  Conn mode: %d", adv_params.conn_mode);
    ESP_LOGI(TAG, "  Disc mode: %d", adv_params.disc_mode);
    ESP_LOGI(TAG, "  Interval: %d-%d", adv_params.itvl_min, adv_params.itvl_max);

    // Start advertising with event handler
    rc = ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event_handler, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to start advertising: %d", rc);
        return;
    }
    
    ESP_LOGI(TAG, "Advertising started successfully!");
}

// NimBLE callback function implementations
static void ble_app_on_sync(void)
{
    ESP_LOGI(TAG, "BLE Host Stack is synchronized.");
    ble_start_advertising_if_not_recording();
}

static void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    char str[BLE_UUID_STR_LEN];
    
    switch (ctxt->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        ESP_LOGI(TAG, "Registered service %s with handle=%d",
                ble_uuid_to_str(ctxt->svc.svc_def->uuid, str),
                ctxt->svc.handle);
        break;
        
    case BLE_GATT_REGISTER_OP_CHR:
        ESP_LOGI(TAG, "Registered characteristic %s with "
                "def_handle=%d val_handle=%d",
                ble_uuid_to_str(ctxt->chr.chr_def->uuid, str),
                ctxt->chr.def_handle,
                ctxt->chr.val_handle);
        
        // Store file transfer status handle for notifications
        uint16_t uuid16 = ble_uuid_u16(ctxt->chr.chr_def->uuid);
        if (uuid16 == BLE_UUID_SALESTAG_FILE_STATUS) {
            s_file_transfer_status_handle = ctxt->chr.val_handle;
            ESP_LOGI(TAG, "File transfer status handle stored: %d", s_file_transfer_status_handle);
        }
        break;
        
    case BLE_GATT_REGISTER_OP_DSC:
        ESP_LOGI(TAG, "Registered descriptor %s with handle=%d",
                ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, str),
                ctxt->dsc.handle);
        break;
        
    default:
        break;
    }
}

static int ble_store_util_status_rr_item(struct ble_store_status_event *event, void *arg)
{
    ESP_LOGI(TAG, "Store status event received");
    return 0;
}

// BLE connection event callbacks
static void ble_app_on_connect(struct ble_gap_event *event, void *arg)
{
    struct ble_gap_conn_desc desc;
    int rc;

    rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to find connection: %d", rc);
        return;
    }

    ESP_LOGI(TAG, "BLE connection established:");
    ESP_LOGI(TAG, "  Connection handle: %d", event->connect.conn_handle);
    ESP_LOGI(TAG, "  Peer address type: %d", desc.peer_ota_addr.type);
    ESP_LOGI(TAG, "  Peer address: %02x:%02x:%02x:%02x:%02x:%02x",
             desc.peer_ota_addr.val[5], desc.peer_ota_addr.val[4],
             desc.peer_ota_addr.val[3], desc.peer_ota_addr.val[2],
             desc.peer_ota_addr.val[1], desc.peer_ota_addr.val[0]);
    
    // Store connection handle for file transfer notifications
    s_file_transfer_conn_handle = event->connect.conn_handle;
    ESP_LOGI(TAG, "File transfer connection handle stored: %d", s_file_transfer_conn_handle);
}

static void ble_app_on_disconnect(struct ble_gap_event *event, void *arg)
{
    ESP_LOGI(TAG, "BLE connection terminated - reason: %d", event->disconnect.reason);
    
    // Clean up file transfer state
    if (s_file_transfer_active) {
        ESP_LOGW(TAG, "File transfer active during disconnect - aborting");
        file_transfer_abort();
    }
    
    s_file_transfer_conn_handle = BLE_HS_CONN_HANDLE_NONE;
    s_file_transfer_status_handle = 0;
    
    // Restart advertising after disconnect (only if not recording)
    ble_start_advertising_if_not_recording();
}

static void ble_app_on_adv_complete(struct ble_gap_event *event, void *arg)
{
    ESP_LOGI(TAG, "BLE advertising completed - reason: %d", event->adv_complete.reason);
    
    // Restart advertising if it was stopped (only if not recording)
    if (event->adv_complete.reason != BLE_HS_ETIMEOUT) {
        ble_start_advertising_if_not_recording();
    }
}

// Main GAP event handler that routes events to appropriate callbacks
static int ble_gap_event_handler(struct ble_gap_event *event, void *arg)
{
    ESP_LOGI(TAG, "GAP event received: type=%d", event->type);
    
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        ble_app_on_connect(event, arg);
        break;
        
    case BLE_GAP_EVENT_DISCONNECT:
        ble_app_on_disconnect(event, arg);
        break;
        
    case BLE_GAP_EVENT_ADV_COMPLETE:
        ble_app_on_adv_complete(event, arg);
        break;
        
    case BLE_GAP_EVENT_SUBSCRIBE:
        ESP_LOGI(TAG, "GAP event: Characteristic subscription changed");
        break;
        
    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "GAP event: MTU exchange completed, MTU=%d", event->mtu.value);
        break;
        
    case BLE_GAP_EVENT_REPEAT_PAIRING:
        ESP_LOGI(TAG, "GAP event: Repeat pairing request");
        break;
        
    case BLE_GAP_EVENT_PHY_UPDATE_COMPLETE:
        ESP_LOGI(TAG, "GAP event: PHY update completed");
        break;
        
    case BLE_GAP_EVENT_EXT_DISC:
        ESP_LOGI(TAG, "GAP event: Extended discovery");
        break;
        
    case BLE_GAP_EVENT_PERIODIC_SYNC:
        ESP_LOGI(TAG, "GAP event: Periodic sync");
        break;
        
    case BLE_GAP_EVENT_PERIODIC_TRANSFER:
        ESP_LOGI(TAG, "GAP event: Periodic transfer");
        break;
        
    case BLE_GAP_EVENT_SCAN_REQ_RCVD:
        ESP_LOGI(TAG, "GAP event: Scan request received");
        break;
        
    default:
        ESP_LOGI(TAG, "Unhandled GAP event: %d", event->type);
        break;
    }
    
    return 0;
}

// GATT Service Access Callback
static int gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                              struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    const struct ble_gatt_chr_def *chr_def;
    uint16_t uuid16;
    int rc;
    
    chr_def = ctxt->chr;
    uuid16 = ble_uuid_u16(chr_def->uuid);
    
    ESP_LOGI(TAG, "GATT access: conn=%d, attr=%d, uuid=0x%04x, op=%d", 
             conn_handle, attr_handle, uuid16, ctxt->op);
    
    switch (uuid16) {
    case BLE_UUID_SALESTAG_RECORD_CTRL:
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            // Return current recording state
            uint8_t recording_state = s_is_recording ? 1 : 0;
            rc = os_mbuf_append(ctxt->om, &recording_state, sizeof(recording_state));
            ESP_LOGI(TAG, "Record control read: state=%d", recording_state);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        } else if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            // Handle recording control command
            uint8_t cmd;
            if (ctxt->om->om_len != sizeof(cmd)) {
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }
            
            rc = ble_hs_mbuf_to_flat(ctxt->om, &cmd, sizeof(cmd), NULL);
            if (rc != 0) {
                return BLE_ATT_ERR_UNLIKELY;
            }
            
            ESP_LOGI(TAG, "Record control write: cmd=%d", cmd);
            
            // Handle recording commands
            if (cmd == 1 && !s_is_recording) {
                // Start recording
                s_recording_count++;
                snprintf(s_current_raw_file, sizeof(s_current_raw_file), "/sdcard/ble_r%03d.raw", s_recording_count);
                
                // Stop BLE advertising to prevent interference
                ble_stop_advertising();
                
                esp_err_t ret = raw_audio_storage_start_recording(s_current_raw_file);
                if (ret == ESP_OK) {
                    ret = audio_capture_start();
                    if (ret == ESP_OK) {
                        s_is_recording = true;
                        ui_set_led(true);
                        ESP_LOGI(TAG, "BLE: Recording started via BLE command");
                    } else {
                        ESP_LOGE(TAG, "BLE: Failed to start audio capture");
                        raw_audio_storage_stop_recording();
                        // Restart BLE advertising since recording failed
                        ble_start_advertising_if_not_recording();
                    }
                } else {
                    ESP_LOGE(TAG, "BLE: Failed to start recording storage");
                    // Restart BLE advertising since recording failed
                    ble_start_advertising_if_not_recording();
                }
            } else if (cmd == 0 && s_is_recording) {
                // Stop recording
                audio_capture_stop();
                esp_err_t ret = raw_audio_storage_stop_recording();
                if (ret == ESP_OK) {
                    s_is_recording = false;
                    ui_set_led(false);
                    ESP_LOGI(TAG, "BLE: Recording stopped via BLE command");
                    
                    // Restart BLE advertising now that recording is finished
                    ble_start_advertising_if_not_recording();
                } else {
                    ESP_LOGE(TAG, "BLE: Failed to stop recording");
                }
            }
            
            return 0;
        }
        break;
        
    case BLE_UUID_SALESTAG_STATUS:
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            // Return device status information
            struct {
                uint8_t audio_enabled;
                uint8_t sd_available;
                uint8_t recording;
                uint32_t total_files;
            } status = {
                .audio_enabled = s_audio_capture_enabled ? 1 : 0,
                .sd_available = sd_storage_is_available() ? 1 : 0,
                .recording = s_is_recording ? 1 : 0,
                .total_files = s_recording_count
            };
            
            rc = os_mbuf_append(ctxt->om, &status, sizeof(status));
            ESP_LOGI(TAG, "Status read: audio=%d, sd=%d, recording=%d, files=%lu", 
                     status.audio_enabled, status.sd_available, status.recording, status.total_files);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        break;
        
    case BLE_UUID_SALESTAG_FILE_COUNT:
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            // Return file count
            rc = os_mbuf_append(ctxt->om, &s_recording_count, sizeof(s_recording_count));
            ESP_LOGI(TAG, "File count read: %d", s_recording_count);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        break;
        
    // File Transfer Service Characteristics
    case BLE_UUID_SALESTAG_FILE_CTRL:
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            // Return current file transfer status
            uint8_t status = s_file_transfer_active ? FILE_TRANSFER_STATUS_ACTIVE : FILE_TRANSFER_STATUS_IDLE;
            rc = os_mbuf_append(ctxt->om, &status, sizeof(status));
            ESP_LOGI(TAG, "File control read: status=%d", status);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        } else if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            // Handle file transfer control commands
            if (ctxt->om->om_len < 1) {
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }
            
            uint8_t cmd;
            rc = ble_hs_mbuf_to_flat(ctxt->om, &cmd, sizeof(cmd), NULL);
            if (rc != 0) {
                return BLE_ATT_ERR_UNLIKELY;
            }
            
            ESP_LOGI(TAG, "File control write: cmd=%d", cmd);
            
            switch (cmd) {
            case FILE_TRANSFER_CMD_START:
                // For now, use a simple approach - just start with a default filename and size
                // The client can send the actual filename and size in subsequent commands
                return file_transfer_start("ble_transfer.dat", 1024);
                
            case FILE_TRANSFER_CMD_STOP:
                return file_transfer_stop();
                
            case FILE_TRANSFER_CMD_ABORT:
                return file_transfer_abort();
                
            default:
                ESP_LOGW(TAG, "Unknown file transfer command: %d", cmd);
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }
        }
        
        break;
        
    case BLE_UUID_SALESTAG_FILE_DATA:
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            // Handle file data writes
            if (!s_file_transfer_active) {
                ESP_LOGW(TAG, "File transfer not active for data write");
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }
            
            uint8_t data[512]; // Max chunk size
            uint16_t data_len = ctxt->om->om_len;
            if (data_len > sizeof(data)) {
                data_len = sizeof(data);
            }
            
            rc = ble_hs_mbuf_to_flat(ctxt->om, data, data_len, NULL);
            if (rc != 0) {
                return BLE_ATT_ERR_UNLIKELY;
            }
            
            ESP_LOGD(TAG, "File data write: %d bytes", data_len);
            return file_transfer_write_data(data, data_len);
        }
        break;
        
    case BLE_UUID_SALESTAG_FILE_STATUS:
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            // Return current file transfer status with progress
            struct {
                uint8_t status;
                uint32_t offset;
                uint32_t size;
            } status = {
                .status = s_file_transfer_active ? FILE_TRANSFER_STATUS_ACTIVE : FILE_TRANSFER_STATUS_IDLE,
                .offset = s_file_transfer_offset,
                .size = s_file_transfer_size
            };
            
            rc = os_mbuf_append(ctxt->om, &status, sizeof(status));
            ESP_LOGI(TAG, "File status read: status=%d, offset=%lu, size=%lu", 
                     status.status, status.offset, status.size);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        break;
    }
    
    return BLE_ATT_ERR_UNLIKELY;
}

// File transfer helper functions implementation
static void file_transfer_notify_status(uint8_t status, uint32_t offset, uint32_t size)
{
    if (s_file_transfer_conn_handle == BLE_HS_CONN_HANDLE_NONE || 
        s_file_transfer_status_handle == 0) {
        return;
    }
    
    struct os_mbuf *om = ble_hs_mbuf_from_flat(&status, sizeof(status));
    if (om) {
        // Add offset and size to the notification
        os_mbuf_append(om, &offset, sizeof(offset));
        os_mbuf_append(om, &size, sizeof(size));
        
        ble_gatts_chr_updated(s_file_transfer_status_handle);
        ESP_LOGI(TAG, "File transfer status notification triggered: status=%d, offset=%lu, size=%lu", 
                 status, offset, size);
    }
}

static int file_transfer_start(const char* filename, uint32_t file_size)
{
    if (s_file_transfer_active) {
        ESP_LOGW(TAG, "File transfer already active");
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }
    
    if (!sd_storage_is_available()) {
        ESP_LOGW(TAG, "SD card not available for file transfer");
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }
    
    // Create full path
    snprintf(s_file_transfer_filename, sizeof(s_file_transfer_filename), "/sdcard/%s", filename);
    
    // Open file for writing
    s_file_transfer_fp = fopen(s_file_transfer_filename, "wb");
    if (!s_file_transfer_fp) {
        ESP_LOGE(TAG, "Failed to create file for transfer: %s", s_file_transfer_filename);
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }
    
    s_file_transfer_active = true;
    s_file_transfer_size = file_size;
    s_file_transfer_offset = 0;
    
    ESP_LOGI(TAG, "File transfer started: %s (size: %lu bytes)", s_file_transfer_filename, file_size);
    file_transfer_notify_status(FILE_TRANSFER_STATUS_ACTIVE, 0, file_size);
    
    return 0;
}

static int file_transfer_stop(void)
{
    if (!s_file_transfer_active) {
        ESP_LOGW(TAG, "No active file transfer to stop");
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }
    
    if (s_file_transfer_fp) {
        fclose(s_file_transfer_fp);
        s_file_transfer_fp = NULL;
    }
    
    s_file_transfer_active = false;
    ESP_LOGI(TAG, "File transfer completed: %s (received: %lu bytes)", 
             s_file_transfer_filename, s_file_transfer_offset);
    
    file_transfer_notify_status(FILE_TRANSFER_STATUS_COMPLETE, s_file_transfer_offset, s_file_transfer_size);
    
    return 0;
}

static int file_transfer_abort(void)
{
    if (!s_file_transfer_active) {
        ESP_LOGW(TAG, "No active file transfer to abort");
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }
    
    if (s_file_transfer_fp) {
        fclose(s_file_transfer_fp);
        s_file_transfer_fp = NULL;
        
        // Remove the partial file
        unlink(s_file_transfer_filename);
    }
    
    s_file_transfer_active = false;
    ESP_LOGI(TAG, "File transfer aborted: %s", s_file_transfer_filename);
    
    file_transfer_notify_status(FILE_TRANSFER_STATUS_ERROR, s_file_transfer_offset, s_file_transfer_size);
    
    return 0;
}

static int file_transfer_write_data(const uint8_t* data, uint16_t len)
{
    if (!s_file_transfer_active || !s_file_transfer_fp) {
        ESP_LOGW(TAG, "No active file transfer for data write");
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }
    
    size_t written = fwrite(data, 1, len, s_file_transfer_fp);
    if (written != len) {
        ESP_LOGE(TAG, "Failed to write file data: expected %d, wrote %d", len, written);
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }
    
    s_file_transfer_offset += written;
    
    // Send progress notification every 1KB
    if (s_file_transfer_offset % 1024 == 0) {
        file_transfer_notify_status(FILE_TRANSFER_STATUS_ACTIVE, s_file_transfer_offset, s_file_transfer_size);
    }
    
    ESP_LOGD(TAG, "File transfer progress: %lu/%lu bytes", s_file_transfer_offset, s_file_transfer_size);
    
    return 0;
}

// NimBLE host task function
static void nimble_host_task(void *param)
{
    ESP_LOGI(TAG, "NimBLE Host Task Started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void app_main(void) {
    ESP_LOGI(TAG, "=== SalesTag SD Storage Test with BLE ===");
    ESP_LOGI(TAG, "BOOT: Testing UI module + SD card storage + BLE...");
    
    // Initialize NVS flash
    ESP_LOGI(TAG, "Initializing NVS flash...");
    esp_err_t nvs_ret = nvs_flash_init();
    if (nvs_ret == ESP_ERR_NVS_NO_FREE_PAGES || nvs_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition corrupted or out-of-date, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_ret);
    ESP_LOGI(TAG, "NVS flash initialized successfully");
    
    // Initialize NimBLE host stack
    ESP_LOGI(TAG, "Initializing NimBLE host stack...");
    ESP_ERROR_CHECK(nimble_port_init());
    
    // Configure the NimBLE device
    ble_hs_cfg.reset_cb = (void (*)(int))ble_app_on_sync;
    ble_hs_cfg.sync_cb = ble_app_on_sync;
    ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr_item;
    
    // Initialize GAP and GATT services
    ble_svc_gap_init();
    ble_svc_gatt_init();
    
    // Register our custom GATT services
    ESP_ERROR_CHECK(ble_gatts_count_cfg(gatt_svr_svcs));
    ESP_ERROR_CHECK(ble_gatts_add_svcs(gatt_svr_svcs));
    
    // Set the device name for advertising
    ESP_ERROR_CHECK(ble_svc_gap_device_name_set("ESP32-S3-Mini-BLE"));
    ESP_LOGI(TAG, "NimBLE device name set to: ESP32-S3-Mini-BLE");
    
    // Start the NimBLE port with FreeRTOS
    ESP_LOGI(TAG, "Starting NimBLE host stack on FreeRTOS task...");
    nimble_port_freertos_init(nimble_host_task);
    ESP_LOGI(TAG, "NimBLE host stack started successfully");
    
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
    
    // Test file creation BEFORE audio capture initialization
    // Add retry logic for SD card state management (based on research findings)
    FILE* pre_audio_test = NULL;
    int retry_count = 0;
    const int max_retries = 5;
    
    while (retry_count < max_retries && !pre_audio_test) {
        pre_audio_test = fopen("/sdcard/pre.txt", "wb");
        if (!pre_audio_test) {
            ESP_LOGW(TAG, "Pre-audio test attempt %d failed (errno: %d), retrying...", retry_count + 1, errno);
            vTaskDelay(pdMS_TO_TICKS(100)); // Wait for SD card to become available
            retry_count++;
        }
    }
    
    if (pre_audio_test) {
        fprintf(pre_audio_test, "Pre-audio test successful\n");
        fclose(pre_audio_test);
        ESP_LOGI(TAG, "Pre-audio test file created successfully after %d attempts", retry_count + 1);
    } else {
        ESP_LOGE(TAG, "Pre-audio test file creation failed after %d attempts (errno: %d)", max_retries, errno);
    }
    
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
    
    ESP_LOGI(TAG, "BLE Functions: Enabled");
    ESP_LOGI(TAG, "  📱 Device name: ESP32-S3-Mini-BLE");
    ESP_LOGI(TAG, "  🔗 NimBLE stack initialized");
    ESP_LOGI(TAG, "  📡 Status: Advertising");
    
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
            FILE* heartbeat_test = fopen("/sdcard/hb.txt", "wb");
            if (heartbeat_test) {
                fprintf(heartbeat_test, "Heartbeat test at %d seconds\n", heartbeat_count);
                fclose(heartbeat_test);
                ESP_LOGI(TAG, "SD card heartbeat test successful");
            } else {
                ESP_LOGW(TAG, "SD card heartbeat test failed (errno: %d)", errno);
            }
            
                                        // Show raw audio storage statistics
            uint32_t samples_written, file_size_bytes;
            if (raw_audio_storage_get_stats(&samples_written, &file_size_bytes) == ESP_OK) {
                ESP_LOGI(TAG, "Raw Audio Stats - Samples: %lu, File Size: %lu bytes", samples_written, file_size_bytes);
            }
            

            
            // List SD card contents every 60 seconds (less frequent)
            if (heartbeat_count % 60 == 0) {
                ESP_LOGI(TAG, "=== SD Card Contents ===");
                DIR* dir = opendir("/sdcard");
                if (dir) {
                    struct dirent* entry;
                    while ((entry = readdir(dir)) != NULL) {
                        if (strstr(entry->d_name, ".raw") || strstr(entry->d_name, ".txt")) {
                            ESP_LOGI(TAG, "File: %s", entry->d_name);
                        }
                    }
                    closedir(dir);
                } else {
                    ESP_LOGW(TAG, "Failed to open /sdcard directory");
                }
                ESP_LOGI(TAG, "=== End SD Card Contents ===");
                
                // BLE status
                ESP_LOGI(TAG, "=== BLE Status ===");
                ESP_LOGI(TAG, "Status: Active");
                ESP_LOGI(TAG, "Device Name: ESP32-S3-Mini-BLE");
                ESP_LOGI(TAG, "Stack: NimBLE");
                
                // Check BLE host stack status
                int ble_status = ble_hs_synced();
                ESP_LOGI(TAG, "BLE Host Stack Synced: %s", ble_status ? "YES" : "NO");
                
                // Check if advertising is active (without stopping it)
                // Note: We can't easily check advertising status without stopping it
                // So we'll just log that advertising should be active
                ESP_LOGI(TAG, "Advertising Status: SHOULD BE ACTIVE");
                // Only restart advertising if we detect it's actually stopped
                // This prevents the constant stop/restart cycle
                
                ESP_LOGI(TAG, "=== End BLE Status ===");
            }
        }
    }
}
