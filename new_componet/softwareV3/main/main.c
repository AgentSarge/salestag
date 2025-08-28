#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
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
// Note: ble_gatts functions are available through existing headers
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "host/ble_uuid.h"
#include "host/ble_gatt.h"
#include "host/ble_att.h"
#include "host/ble_hs_mbuf.h"
#include "os/os_mempool.h"

#include "esp_timer.h"
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <assert.h>
#include <strings.h>  // for strcasecmp
#include <stdbool.h>  // for bool
#include <stdio.h>    // for FILE, snprintf
#include <time.h>     // for time_t
#include <ctype.h>    // for isalnum

// Set to 1 for deep debug, 0 for normal operation
#define FILE_XFER_VERBOSE 1

static const char *TAG = "salestag-sd";

#define BTN_GPIO 4
#define LED_GPIO 40
#define DEBOUNCE_MS 50

// Add worker-task pipeline defines
#ifndef SD_MAX_PATH
#define SD_MAX_PATH 256
#endif

#define FT_PKT_MAX 200
#define FT_MAX_RETRIES 8

// Custom UUID definitions for SalesTag Audio Service
#define BLE_UUID_SALESTAG_AUDIO_SVC    0x1234
#define BLE_UUID_SALESTAG_RECORD_CTRL  0x1235
#define BLE_UUID_SALESTAG_STATUS       0x1236
#define BLE_UUID_SALESTAG_FILE_COUNT   0x1237

// Custom UUID definitions for SalesTag File Transfer Service
#define BLE_UUID_SALESTAG_FILE_SVC         0x1240
#define BLE_UUID_SALESTAG_FILE_CTRL        0x1241  // Write: Commands (START, START_WITH_FILENAME, PAUSE, RESUME, STOP, LIST_FILES, SELECT_FILE)
#define BLE_UUID_SALESTAG_FILE_DATA        0x1242  // Notify: File data chunks
#define BLE_UUID_SALESTAG_FILE_STATUS      0x1243  // Notify: Transfer status
#define BLE_UUID_SALESTAG_FILE_LIST        0x1244  // Read: List available .raw filenames (legacy)
#define BLE_UUID_SALESTAG_AUTO_SELECT_LIST 0x1245  // Read: Auto-selection file list (returns latest file)

// UUID objects
static const ble_uuid16_t UUID_AUDIO_SVC   = BLE_UUID16_INIT(BLE_UUID_SALESTAG_AUDIO_SVC);
static const ble_uuid16_t UUID_RECORD_CTRL = BLE_UUID16_INIT(BLE_UUID_SALESTAG_RECORD_CTRL);
static const ble_uuid16_t UUID_STATUS      = BLE_UUID16_INIT(BLE_UUID_SALESTAG_STATUS);
static const ble_uuid16_t UUID_FILE_COUNT  = BLE_UUID16_INIT(BLE_UUID_SALESTAG_FILE_COUNT);

static const ble_uuid16_t UUID_FILE_SVC            = BLE_UUID16_INIT(BLE_UUID_SALESTAG_FILE_SVC);
static const ble_uuid16_t UUID_FILE_CTRL           = BLE_UUID16_INIT(BLE_UUID_SALESTAG_FILE_CTRL);
static const ble_uuid16_t UUID_FILE_DATA           = BLE_UUID16_INIT(BLE_UUID_SALESTAG_FILE_DATA);
static const ble_uuid16_t UUID_FILE_STATUS         = BLE_UUID16_INIT(BLE_UUID_SALESTAG_FILE_STATUS);
static const ble_uuid16_t UUID_FILE_LIST           = BLE_UUID16_INIT(BLE_UUID_SALESTAG_FILE_LIST);
static const ble_uuid16_t UUID_AUTO_SELECT_LIST    = BLE_UUID16_INIT(BLE_UUID_SALESTAG_AUTO_SELECT_LIST);

// File transfer command definitions (updated for auto-selection)
//
// MOBILE APP USAGE:
//
// 1. FILE_TRANSFER_CMD_START (0x01) - Start transfer with auto-selected latest file
//    Data: [0x01]
//    Use: When you want the ESP32 to automatically choose the latest .raw file
//
// 2. FILE_TRANSFER_CMD_START_WITH_FILENAME (0x07) - Start transfer with specific filename
//    Data: [0x07][filename_string]
//    Use: When you want to download a specific file
//    Example: [0x07]['r','0','0','1','.','r','a','w'] for "r001.raw"
//    Notes:
//    - Filename should not include path (just the base filename)
//    - .raw extension is optional (will be added if missing)
//    - Only alphanumeric, dots, underscores, and hyphens allowed
//    - Maximum 255 characters
//    - Path traversal characters (.., /, \) are blocked for security
//
// 3. FILE_TRANSFER_CMD_LIST_FILES (0x05) - Get list of available files for auto-selection
//    Data: [0x05]
//    Use: Request list of available .raw files (returns latest file first)
//    Response: Auto-selection list via UUID 0x1245 characteristic
//
// 4. FILE_TRANSFER_CMD_SELECT_FILE (0x04) - Select specific file from auto-selection list
//    Data: [0x04][index_byte]
//    Use: Select file by index from the auto-selection list
//    Example: [0x04][0x00] to select the first (latest) file
//
// WORKFLOW RECOMMENDATION:
// 1. Send LIST_FILES command (0x05) to get available files
// 2. Send SELECT_FILE command (0x04) with index to choose file
// 3. Or send START command (0x01) for immediate auto-selection of latest file
// 4. Or send START_WITH_FILENAME command (0x07) with known filename
//
#define FILE_TRANSFER_CMD_START                   0x01
#define FILE_TRANSFER_CMD_PAUSE                   0x02
#define FILE_TRANSFER_CMD_RESUME                  0x03
#define FILE_TRANSFER_CMD_SELECT_FILE             0x04  // Select file by index from auto-selection list
#define FILE_TRANSFER_CMD_LIST_FILES              0x05  // Get auto-selection file list
#define FILE_TRANSFER_CMD_STOP                    0x06  // Moved to avoid conflict
#define FILE_TRANSFER_CMD_START_WITH_FILENAME     0x07  // Moved to avoid conflict


// File transfer status codes (updated to 1-byte values)
#define STAT_STARTED                   0x01
#define STAT_COMPLETE                  0x02
#define STAT_STOPPED_BY_HOST           0x03
#define STAT_FILE_OPEN_FAIL            0x10
#define STAT_NOTIFY_FAIL               0x11
#define STAT_BAD_CMD                   0x20
#define STAT_ALREADY_RUNNING           0x21
#define STAT_PAUSED                    0x30
#define STAT_SUBSCRIPTION_REQUIRED     0x40
#define STAT_NO_FILE                   0x50
#define STAT_BUSY                      0x22
#define STAT_NO_CONN                   0x23
#define STAT_FILE_READ_FAIL            0x13
#define STAT_LIST_READY                0x60  // Auto-selection file list ready
#define STAT_FILE_SELECTED             0x61  // File selected from auto-selection list
#define STAT_INVALID_INDEX             0x62  // Invalid file index in SELECT_FILE command

// File transfer packet header size (5 bytes)
#define FILE_TRANSFER_HEADER_SIZE 5

// File transfer status notification (now 1 byte)
// Status codes are now sent as single bytes

// NimBLE callback function declarations
static void ble_app_on_sync(void);
static void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);
static int ble_store_util_status_rr_item(struct ble_store_status_event *event, void *arg);
static void ble_app_on_connect(struct ble_gap_event *event, void *arg);
static void ble_app_on_disconnect(struct ble_gap_event *event, void *arg);
static int ble_gap_event_handler(struct ble_gap_event *event, void *arg);
static void ble_app_on_adv_complete(struct ble_gap_event *event, void *arg);

// Add proper reset callback
static void on_reset(int reason) { 
    ESP_LOGW(TAG, "NimBLE reset reason=%d", reason); 
}

// BLE advertising control functions
static void ble_app_advertise(void);
static void ble_stop_advertising(void);
static void ble_start_advertising_if_not_recording(void);

// NimBLE host task function
static void nimble_host_task(void *param);

// File transfer helper functions
static int file_transfer_start(void);
static int file_transfer_stop(void);
static int file_transfer_pause(void);
static int file_transfer_resume(void);
static void send_status(uint8_t code);
static inline bool notifies_ready(void);
static void update_payload_len(uint16_t mtu);

// Forward declarations for functions called before definition
static bool is_valid_filename(const char *filename);
static int list_available_raw_files(struct os_mbuf *om);
static int list_auto_select_files(struct os_mbuf *om);
static int file_transfer_start_with_filename(const char *requested_filename);
static int file_transfer_list_files(void);
static int file_transfer_select_file(uint8_t file_index);

// GATT service callback declarations
static int gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                              struct ble_gatt_access_ctxt *ctxt, void *arg);

// Global state for recording
static int s_recording_count = 0;
static bool s_audio_capture_enabled = false;

// File transfer subscription tracking
static volatile bool s_is_recording = false;  // Made volatile for multi-task access
static char s_current_raw_file[128] = {0};

// Connection and characteristic handles
uint16_t s_file_transfer_conn_handle = 0;
uint16_t s_file_transfer_data_handle = 0;
uint16_t s_file_transfer_status_handle = 0;

// State
volatile bool s_file_transfer_active = false;
volatile bool s_file_transfer_paused = false;

// Progress
uint32_t s_file_transfer_size = 0;
uint32_t s_file_transfer_offset = 0;
uint32_t s_bytes_sent = 0;
uint16_t s_seq = 0;

// File
FILE *s_file_transfer_fp = NULL;

// Subscription tracking (GAP SUBSCRIBE approach)
static volatile uint8_t s_cccd_mask = 0; // bit0 = Data, bit1 = Status

// MTU and payload handling
static uint16_t s_mtu = 23;
static size_t s_payload_max = 20; // mtu - 3

// File transfer command queue for worker task
typedef enum { FT_CMD_START, FT_CMD_STOP } ft_cmd_t;

typedef struct {
    ft_cmd_t type;
} ft_msg_t;

static QueueHandle_t s_ft_q = NULL;

// Credit-based pacing for BLE notifications
static SemaphoreHandle_t s_notify_sem = NULL;
static const int kMaxInFlight = 3;  // Reduced from 4 to be more conservative with mbuf usage

// GATT characteristic arrays (sentinel-terminated)
static const struct ble_gatt_chr_def audio_chrs[] = {
    { .uuid = &UUID_RECORD_CTRL.u, .access_cb = gatt_svr_chr_access, .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE },
    { .uuid = &UUID_STATUS.u,      .access_cb = gatt_svr_chr_access, .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY },
    { .uuid = &UUID_FILE_COUNT.u,  .access_cb = gatt_svr_chr_access, .flags = BLE_GATT_CHR_F_READ },
    { 0 }
};

static const struct ble_gatt_chr_def file_chrs[] = {
    { .uuid = &UUID_FILE_CTRL.u,        .access_cb = gatt_svr_chr_access, .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP },
    { .uuid = &UUID_FILE_DATA.u,        .access_cb = gatt_svr_chr_access, .flags = BLE_GATT_CHR_F_NOTIFY },
    { .uuid = &UUID_FILE_STATUS.u,      .access_cb = gatt_svr_chr_access, .flags = BLE_GATT_CHR_F_NOTIFY },
    { .uuid = &UUID_FILE_LIST.u,        .access_cb = gatt_svr_chr_access, .flags = BLE_GATT_CHR_F_READ },
    { .uuid = &UUID_AUTO_SELECT_LIST.u, .access_cb = gatt_svr_chr_access, .flags = BLE_GATT_CHR_F_READ },
    { 0 }
};

// GATT Service Definition (sentinel-terminated)
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    { .type = BLE_GATT_SVC_TYPE_PRIMARY, .uuid = &UUID_AUDIO_SVC.u, .characteristics = audio_chrs },
    { .type = BLE_GATT_SVC_TYPE_PRIMARY, .uuid = &UUID_FILE_SVC.u,  .characteristics = file_chrs },
    { 0 }
};

static void gatt_preflight(void)
{
    for (const struct ble_gatt_svc_def *s = gatt_svr_svcs; s && s->type; s++) {
        if (!s->uuid) { ESP_LOGE(TAG, "svc null uuid"); abort(); }
        if (!s->characteristics) continue;
        for (const struct ble_gatt_chr_def *c = s->characteristics; c && c->uuid; c++) {
            if (!c->uuid) { ESP_LOGE(TAG, "chr null uuid"); abort(); }
            if (!c->access_cb) ESP_LOGW(TAG, "chr without access_cb");
        }
    }
}

// Removed unused gatt_validate function


// ADC sample queue for decoupling real-time sampling from file I/O
static QueueHandle_t s_adc_sample_queue = NULL;

// Raw ADC callback function - now lightweight (just queues samples)
static void raw_adc_callback(uint16_t mic_adc, void *user_ctx) {
    (void)user_ctx;  // Unused

    // Just queue the sample - no heavy I/O operations!
    // Use regular task context queue functions (not ISR versions)
    if (s_adc_sample_queue) {
        xQueueSend(s_adc_sample_queue, &mic_adc, 0);  // Don't block if queue is full
    }
}

// Storage task for handling file I/O safely
static void storage_task(void *pvParameters) {
    (void)pvParameters;

    ESP_LOGI(TAG, "Storage task started");

    uint16_t mic_sample;
    uint32_t sample_counter = 0; // For professional logging intervals

    while (1) {
        // Wait for samples from the queue with a reasonable timeout
        if (xQueueReceive(s_adc_sample_queue, &mic_sample, pdMS_TO_TICKS(100))) {
            sample_counter++;

            // Professional audio status logging (every 8000 samples = 0.5 sec at 16kHz)
            if (sample_counter % 8000 == 0) {
                ESP_LOGI(TAG, "🎵 Audio Processing Status - Samples processed: %lu", sample_counter);
                ESP_LOGI(TAG, "  Recording: %s, Queue depth: %d",
                         s_is_recording ? "ACTIVE" : "STANDBY",
                         uxQueueMessagesWaiting(s_adc_sample_queue));
            }

            // Only do file I/O when recording is active
            if (s_is_recording) {
                esp_err_t ret = raw_audio_storage_add_sample(mic_sample);
                if (ret != ESP_OK) {
                    ESP_LOGW(TAG, "Failed to add raw audio sample: %s", esp_err_to_name(ret));
                    // Add a small delay to prevent rapid error logging
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            }
            // If not recording, just consume and discard samples to drain the queue
        }
        // Else: timeout occurred, continue loop to check recording status
    }
}



// Button callback function - Toggle Recording (Option A)
static void button_callback(bool pressed, uint32_t timestamp_ms, void *ctx) {
    (void)ctx;  // Unused
    
    ESP_LOGI(TAG, "=== BUTTON CALLBACK === Button %s at %u ms", pressed ? "PRESSED" : "RELEASED", (unsigned)timestamp_ms);
    
    // LED shows RECORDING state, not button state
    // (LED will be controlled by recording logic below)
    

    
    // When button is pressed, handle recording
    if (pressed && sd_storage_is_available()) {
        // Prevent recording from starting during file transfer
        if (s_file_transfer_active) {
            ESP_LOGW(TAG, "Recording blocked - file transfer in progress");
            return;
        }

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
                const char *rec_dir = "/sdcard/rec";
                snprintf(s_current_raw_file, sizeof(s_current_raw_file), "%s/ble_r%03d.raw", rec_dir, s_recording_count);
                
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

        if (s_audio_capture_enabled && !s_is_recording) {
            // Start raw audio recording - store ADC samples directly
            const char *rec_dir = "/sdcard/rec";
            snprintf(s_current_raw_file, sizeof(s_current_raw_file), "%s/r%03d.raw", rec_dir, s_recording_count);
            
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
    ESP_LOGI(TAG, "ble_start_advertising_if_not_recording: recording=%d", s_is_recording);
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

static void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg) {
    switch (ctxt->op) {
    case BLE_GATT_REGISTER_OP_CHR:  // Correct ESP-IDF NimBLE constant
        // match by UUID and save value handle
        if (ble_uuid_cmp(ctxt->chr.chr_def->uuid, &UUID_FILE_DATA.u) == 0) {
            s_file_transfer_data_handle = ctxt->chr.val_handle;
            ESP_LOGI(TAG, "File transfer data handle: %u", (unsigned)s_file_transfer_data_handle);
        } else if (ble_uuid_cmp(ctxt->chr.chr_def->uuid, &UUID_FILE_STATUS.u) == 0) {
            s_file_transfer_status_handle = ctxt->chr.val_handle;
            ESP_LOGI(TAG, "File transfer status handle: %u", (unsigned)s_file_transfer_status_handle);
        }
        break;
    case BLE_GATT_REGISTER_OP_DSC:  // Correct ESP-IDF NimBLE constant
        // if you want CCCD handles for logging, grab them here
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
    
    // Store connection handle for file transfer notifications (only on successful connect)
    if (event->connect.status == 0) {
        s_file_transfer_conn_handle = event->connect.conn_handle;
        ESP_LOGI(TAG, "File transfer connection handle stored: %d", s_file_transfer_conn_handle);
    } else {
        s_file_transfer_conn_handle = 0;
    }
    
    // Note: MTU negotiation is handled by the central (client)
    // We set preferred MTU during initialization and handle MTU events
}

static void ble_app_on_disconnect(struct ble_gap_event *event, void *arg)
{
    ESP_LOGI(TAG, "BLE connection terminated - reason: %d", event->disconnect.reason);
    
    // Clear file transfer state
    s_file_transfer_conn_handle = 0;
    s_file_transfer_active = false;
    if (s_file_transfer_fp) { 
        fclose(s_file_transfer_fp); 
        s_file_transfer_fp = NULL; 
    }
    // Clear state for clean next run
    s_bytes_sent = 0;
    s_seq = 0;
    s_file_transfer_offset = 0;
    
    // Clear subscription mask
    s_cccd_mask = 0;
    
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
#if FILE_XFER_VERBOSE
    ESP_LOGI(TAG, "GAP event received: type=%d", event->type);
#endif
    
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
        
    case BLE_GAP_EVENT_SUBSCRIBE: {
        // Direct access after including host/ble_gap.h
        if (event->subscribe.attr_handle == s_file_transfer_data_handle) {
            if (event->subscribe.cur_notify || event->subscribe.cur_indicate) s_cccd_mask |= 0x01;
            else s_cccd_mask &= ~0x01;
        } else if (event->subscribe.attr_handle == s_file_transfer_status_handle) {
            if (event->subscribe.cur_notify || event->subscribe.cur_indicate) s_cccd_mask |= 0x02;
            else s_cccd_mask &= ~0x02;
        }
        break;
    }
        
    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "MTU exchange completed: %d", event->mtu.value);
        update_payload_len(event->mtu.value);
        ESP_LOGI(TAG, "MTU updated: %d, payload_max: %zu", s_mtu, s_payload_max);
        break;
        
    case BLE_GAP_EVENT_NOTIFY_TX: {
        // Return a credit for successful DATA notifies
        if (event->notify_tx.attr_handle == s_file_transfer_data_handle) {
            if (event->notify_tx.status == 0 && s_notify_sem) {
                BaseType_t xHigher = pdFALSE;
                xSemaphoreGiveFromISR(s_notify_sem, &xHigher);
                ESP_LOGI(TAG, "Credit returned: TX complete for data handle");
                portYIELD_FROM_ISR(xHigher);
            }
        }
#if FILE_XFER_VERBOSE
        ESP_LOGI(TAG, "Notify TX complete: conn=%d, attr_handle=%d, status=%d",
                 event->notify_tx.conn_handle, event->notify_tx.attr_handle, event->notify_tx.status);
#endif
        break;
    }
        
    case BLE_GAP_EVENT_CONN_UPDATE:
        ESP_LOGI(TAG, "Connection parameters updated");
        break;
        
    case BLE_GAP_EVENT_CONN_UPDATE_REQ:
        ESP_LOGI(TAG, "Connection update request received");
        break;
        
    case BLE_GAP_EVENT_L2CAP_UPDATE_REQ:
        ESP_LOGI(TAG, "L2CAP update request received");
        break;
        
    // Note: BLE_GAP_EVENT_CONGEST is not available in this NimBLE version
    // Congestion handling is done via BLE_HS_ECONTROLLER return codes
        
    case BLE_GAP_EVENT_REPEAT_PAIRING:
        ESP_LOGI(TAG, "GAP event: Repeat pairing request");
        break;
        
    default:
        ESP_LOGI(TAG, "GAP event: type=%d", event->type);
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
            // Return current recording state (read-only now - writes disabled)
            uint8_t recording_state = s_is_recording ? 1 : 0;
            rc = os_mbuf_append(ctxt->om, &recording_state, sizeof(recording_state));
            ESP_LOGI(TAG, "Record control read: state=%d (use physical button to control)", recording_state);
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
            
            ESP_LOGI(TAG, "Record control write: cmd=%d (IGNORED - use physical button)", cmd);

            // DISABLE BLE RECORDING CONTROL - Use physical button only
            // This prevents mobile apps from accidentally triggering recording
            // when they meant to upload files
            ESP_LOGW(TAG, "BLE recording control DISABLED - use physical button only");
            
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
            ESP_LOGI(TAG, "Status read: audio=%d, sd=%d, recording=%d, files=%u", 
                     status.audio_enabled, status.sd_available, status.recording, (unsigned)status.total_files);
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

    case BLE_UUID_SALESTAG_FILE_LIST:
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            // Return list of available .raw files
            rc = list_available_raw_files(ctxt->om);
            ESP_LOGI(TAG, "File list read: %s", rc == 0 ? "success" : "failed");
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        break;

    case BLE_UUID_SALESTAG_AUTO_SELECT_LIST:
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            // Return auto-selection file list (latest file first)
            rc = list_auto_select_files(ctxt->om);
            ESP_LOGI(TAG, "Auto-select list read: %s", rc == 0 ? "success" : "failed");
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        break;
        
    // File Transfer Service Characteristics
    case BLE_UUID_SALESTAG_FILE_CTRL:
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            // Handle file transfer control commands - variable length based on command
            if (ctxt->om->om_len < 1) {
                ESP_LOGW(TAG, "Invalid file control write length: %d (minimum 1)", ctxt->om->om_len);
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }

            uint8_t cmd;
            rc = ble_hs_mbuf_to_flat(ctxt->om, &cmd, sizeof(cmd), NULL);
            if (rc != 0) {
                return BLE_ATT_ERR_UNLIKELY;
            }

            ESP_LOGI(TAG, "File control write: cmd=0x%02x, len=%d", cmd, ctxt->om->om_len);

            switch (cmd) {
            case FILE_TRANSFER_CMD_START:
                if (ctxt->om->om_len != 1) {
                    ESP_LOGW(TAG, "START command should have no additional data (len=%d)", ctxt->om->om_len);
                    return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
                }
                return file_transfer_start();

            case FILE_TRANSFER_CMD_SELECT_FILE: {
                if (ctxt->om->om_len != 2) {
                    ESP_LOGW(TAG, "SELECT_FILE command needs 1-byte index (len=%d)", ctxt->om->om_len);
                    return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
                }

                uint8_t file_index;
                // Extract the file index from the second byte (skip command byte)
                uint8_t *data_ptr = ctxt->om->om_data + 1; // Skip command byte
                file_index = *data_ptr;

                ESP_LOGI(TAG, "SELECT_FILE: index=%d", file_index);
                return file_transfer_select_file(file_index);
            }

            case FILE_TRANSFER_CMD_LIST_FILES:
                if (ctxt->om->om_len != 1) {
                    ESP_LOGW(TAG, "LIST_FILES command should have no additional data (len=%d)", ctxt->om->om_len);
                    return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
                }
                return file_transfer_list_files();

            case FILE_TRANSFER_CMD_START_WITH_FILENAME: {
                if (ctxt->om->om_len < 2) {
                    ESP_LOGW(TAG, "START_WITH_FILENAME command needs filename data (len=%d)", ctxt->om->om_len);
                    return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
                }

                // Extract filename from the remaining data
                size_t filename_len = ctxt->om->om_len - 1;
                char requested_filename[SD_MAX_PATH] = {0};

                if (filename_len >= sizeof(requested_filename)) {
                    ESP_LOGW(TAG, "Filename too long: %d bytes", filename_len);
                    return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
                }

                // Copy filename data (skip the command byte by using offset)
                uint8_t *data_ptr = ctxt->om->om_data + 1; // Skip command byte
                memcpy(requested_filename, data_ptr, filename_len);

                // Ensure null termination
                requested_filename[filename_len] = '\0';

                ESP_LOGI(TAG, "START_WITH_FILENAME: '%s'", requested_filename);

                // Validate filename (basic security check)
                if (!is_valid_filename(requested_filename)) {
                    ESP_LOGW(TAG, "Invalid filename requested: '%s'", requested_filename);
                    send_status(STAT_BAD_CMD);
                    return 0;
                }

                // Start transfer with specific filename
                return file_transfer_start_with_filename(requested_filename);
            }

            case FILE_TRANSFER_CMD_PAUSE:
                if (ctxt->om->om_len != 1) {
                    ESP_LOGW(TAG, "PAUSE command should have no additional data (len=%d)", ctxt->om->om_len);
                    return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
                }
                return file_transfer_pause();

            case FILE_TRANSFER_CMD_RESUME:
                if (ctxt->om->om_len != 1) {
                    ESP_LOGW(TAG, "RESUME command should have no additional data (len=%d)", ctxt->om->om_len);
                    return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
                }
                return file_transfer_resume();

            case FILE_TRANSFER_CMD_STOP:
                if (ctxt->om->om_len != 1) {
                    ESP_LOGW(TAG, "STOP command should have no additional data (len=%d)", ctxt->om->om_len);
                    return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
                }
                return file_transfer_stop();

            default:
                ESP_LOGW(TAG, "Unknown file transfer command: 0x%02x", cmd);
                send_status(STAT_BAD_CMD);
                return 0; // Return success, error communicated via status
            }
        }
        break;
        
    case BLE_UUID_SALESTAG_FILE_DATA:
        // File data characteristic is notify-only, no writes allowed
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            ESP_LOGW(TAG, "File data characteristic is notify-only, writes not allowed");
            return BLE_ATT_ERR_WRITE_NOT_PERMITTED;
        }
        break;
        
    case BLE_UUID_SALESTAG_FILE_STATUS:
        // File status characteristic is notify-only, no reads allowed
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            ESP_LOGW(TAG, "File status characteristic is notify-only, reads not allowed");
            return BLE_ATT_ERR_READ_NOT_PERMITTED;
        }
        break;
    }
    
    return BLE_ATT_ERR_UNLIKELY;
}

// File transfer helper functions implementation

// Filename validation function for basic security
static bool is_valid_filename(const char *filename) {
    if (!filename || filename[0] == '\0') {
        return false;
    }

    size_t len = strlen(filename);

    // Check length constraints
    if (len > 255 || len < 1) {
        return false;
    }

    // Allow only alphanumeric characters, dots, underscores, and hyphens
    // This is a basic security measure to prevent path traversal attacks
    for (size_t i = 0; i < len; i++) {
        char c = filename[i];
        if (!isalnum(c) && c != '.' && c != '_' && c != '-') {
            return false;
        }
    }

    // Check for obvious path traversal attempts
    if (strstr(filename, "..") != NULL || strstr(filename, "/") != NULL || strstr(filename, "\\") != NULL) {
        return false;
    }

    return true;
}

// List available .raw files for BLE reading
static int list_available_raw_files(struct os_mbuf *om) {
    ESP_LOGI(TAG, "File list request received");

    // Temporarily disabled due to stack corruption issues
    // TODO: Fix stack corruption in directory listing
    const char *msg = "Feature temporarily disabled\n";
    int rc = os_mbuf_append(om, msg, strlen(msg));
    ESP_LOGW(TAG, "Filename listing disabled - stack corruption issue");
    return (rc == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

// File transfer start with specific filename
static int file_transfer_start_with_filename(const char *requested_filename) {
    if (s_file_transfer_active) {
        ESP_LOGW(TAG, "File transfer already active");
        send_status(STAT_ALREADY_RUNNING);
        return 0;
    }

    // Prevent file transfer from starting during recording
    if (s_is_recording) {
        ESP_LOGW(TAG, "File transfer blocked - recording in progress");
        send_status(STAT_BUSY);
        return 0;
    }

    // Check if both DATA and STATUS characteristics are subscribed
    if (!notifies_ready()) {
        send_status(STAT_SUBSCRIPTION_REQUIRED);
        return 0;
    }

    // Check if SD card is available
    if (!sd_storage_is_available()) {
        ESP_LOGE(TAG, "SD card not available for file transfer");
        send_status(STAT_FILE_OPEN_FAIL);
        return 0;
    }

    // Construct full path for requested filename
    char full_path[SD_MAX_PATH] = {0};
    const char *rec_dir = "/sdcard/rec";
    if (strstr(requested_filename, ".raw")) {
        // Filename already includes .raw extension
        snprintf(full_path, sizeof(full_path), "%s/%s", rec_dir, requested_filename);
    } else {
        // Add .raw extension
        snprintf(full_path, sizeof(full_path), "%s/%s.raw", rec_dir, requested_filename);
    }

    ESP_LOGI(TAG, "Requested filename: '%s' -> full path: '%s'", requested_filename, full_path);

    // Check if file exists
    struct stat st;
    if (stat(full_path, &st) != 0) {
        ESP_LOGE(TAG, "Requested file does not exist: %s", full_path);
        send_status(STAT_NO_FILE);
        return 0;
    }

    // Check if it's a regular file
    if (!S_ISREG(st.st_mode)) {
        ESP_LOGE(TAG, "Requested path is not a regular file: %s", full_path);
        send_status(STAT_FILE_OPEN_FAIL);
        return 0;
    }

    // Check file size (guard against empty files)
    if (st.st_size == 0) {
        ESP_LOGE(TAG, "Requested file is empty: %s", full_path);
        send_status(STAT_NO_FILE);
        return 0;
    }

    // Set the specific filename for transfer
    strncpy(s_current_raw_file, full_path, sizeof(s_current_raw_file) - 1);
    s_current_raw_file[sizeof(s_current_raw_file) - 1] = '\0';

    ESP_LOGI(TAG, "Set transfer filename to: %s", s_current_raw_file);

    // Enqueue the start command to worker task
    ft_msg_t m = { .type = FT_CMD_START };
    if (s_ft_q) xQueueSend(s_ft_q, &m, 0);  // non-blocking

    return 0; // success
}

static int file_transfer_start(void)
{
    if (s_file_transfer_active) {
        ESP_LOGW(TAG, "File transfer already active");
        send_status(STAT_ALREADY_RUNNING);
        return 0;
    }

    // Prevent file transfer from starting during recording
    if (s_is_recording) {
        ESP_LOGW(TAG, "File transfer blocked - recording in progress");
        send_status(STAT_BUSY);
        return 0;
    }

    // Check if both DATA and STATUS characteristics are subscribed
    if (!notifies_ready()) {
        send_status(STAT_SUBSCRIPTION_REQUIRED);
        return 0;
    }

    // Check if SD card is available
    if (!sd_storage_is_available()) {
        ESP_LOGE(TAG, "SD card not available for file transfer");
        send_status(STAT_FILE_OPEN_FAIL);
        return 0;
    }
    
    // Enqueue the start command to worker task
    ft_msg_t m = { .type = FT_CMD_START };
    if (s_ft_q) xQueueSend(s_ft_q, &m, 0);  // non-blocking
    
    return 0; // success
}

static int file_transfer_stop(void)
{
    // Enqueue the stop command to worker task
    ft_msg_t m = { .type = FT_CMD_STOP };
    if (s_ft_q) xQueueSend(s_ft_q, &m, 0);  // non-blocking
    
    return 0;
}

static int file_transfer_pause(void)
{
    if (!s_file_transfer_active) {
        ESP_LOGW(TAG, "No active file transfer to pause");
        return 0; // Return success, error communicated via status
    }
    
    s_file_transfer_paused = true;
    ESP_LOGI(TAG, "File transfer paused");
    send_status(STAT_PAUSED);
    
    return 0;
}

static int file_transfer_resume(void)
{
    if (!s_file_transfer_active) {
        ESP_LOGW(TAG, "No active file transfer to resume");
        return 0; // Return success, error communicated via status
    }

    s_file_transfer_paused = false;
    ESP_LOGI(TAG, "File transfer resumed");

    return 0;
}

// Static buffer to avoid stack overflow in auto-selection function
static char s_auto_select_buffer[1024] = {0};

// Auto-selection file list - returns latest file info for auto-selection
static int list_auto_select_files(struct os_mbuf *om) {
    ESP_LOGI(TAG, "Auto-selection file list request received");

    // Use static buffer to avoid stack overflow
    memset(s_auto_select_buffer, 0, sizeof(s_auto_select_buffer));

    // Check if SD card is available
    if (!sd_storage_is_available()) {
        ESP_LOGW(TAG, "SD card not available for file listing");
        const char *msg = "SD card not available\n";
        int rc = os_mbuf_append(om, msg, strlen(msg));
        return (rc == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    const char *rec_dir = "/sdcard/rec";
    DIR *dir = opendir(rec_dir);
    if (!dir) {
        ESP_LOGW(TAG, "Failed to open recordings directory: %s", rec_dir);
        const char *msg = "No recordings directory\n";
        int rc = os_mbuf_append(om, msg, strlen(msg));
        return (rc == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    struct dirent *entry;
    char latest_file[128] = {0};  // Reduced size
    time_t latest_time = 0;
    uint32_t file_count = 0;

    // Scan directory for .raw files and find the latest one
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_REG) continue;

        char *ext = strrchr(entry->d_name, '.');
        if (!ext || strcmp(ext, ".raw") != 0) continue;

        // Use static buffer for full path
        char *full_path = s_auto_select_buffer;
        size_t path_len = snprintf(full_path, sizeof(s_auto_select_buffer), "%s/%s", rec_dir, entry->d_name);

        if (path_len >= sizeof(s_auto_select_buffer)) {
            ESP_LOGW(TAG, "Path too long, skipping: %s", entry->d_name);
            continue;
        }

        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode)) {
            file_count++;
            if (st.st_mtime > latest_time) {
                latest_time = st.st_mtime;
                strncpy(latest_file, entry->d_name, sizeof(latest_file) - 1);
                latest_file[sizeof(latest_file) - 1] = '\0';
            }
        }
    }
    closedir(dir);

    if (file_count == 0) {
        const char *msg = "No .raw files found\n";
        int rc = os_mbuf_append(om, msg, strlen(msg));
        return (rc == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    // Get file size for the latest file using static buffer
    char *full_path = s_auto_select_buffer;
    snprintf(full_path, sizeof(s_auto_select_buffer), "%s/%s", rec_dir, latest_file);
    struct stat st;
    uint32_t file_size = 0;
    if (stat(full_path, &st) == 0) {
        file_size = st.st_size;
    }

    // Format response using static buffer
    char *response = s_auto_select_buffer + 256;  // Use different part of buffer
    int len = snprintf(response, sizeof(s_auto_select_buffer) - 256, "LATEST:%s:%lu:%lu\n", latest_file, (unsigned long)file_size, (unsigned long)file_count);

    if (len >= (int)(sizeof(s_auto_select_buffer) - 256)) {
        const char *msg = "Response too long\n";
        int rc = os_mbuf_append(om, msg, strlen(msg));
        return (rc == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    int rc = os_mbuf_append(om, response, strlen(response));
    ESP_LOGI(TAG, "Auto-select response: %s", response);
    return (rc == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

// LIST_FILES command - trigger auto-selection list refresh
static int file_transfer_list_files(void) {
    ESP_LOGI(TAG, "LIST_FILES command received");

    // Send status to indicate list is ready
    send_status(STAT_LIST_READY);

    return 0;
}

// SELECT_FILE command - select file by index from auto-selection list
static int file_transfer_select_file(uint8_t file_index) {
    ESP_LOGI(TAG, "SELECT_FILE command received, index: %d", file_index);

    if (s_file_transfer_active) {
        ESP_LOGW(TAG, "File transfer already active");
        send_status(STAT_ALREADY_RUNNING);
        return 0;
    }

    // Prevent file transfer from starting during recording
    if (s_is_recording) {
        ESP_LOGW(TAG, "File transfer blocked - recording in progress");
        send_status(STAT_BUSY);
        return 0;
    }

    // Check if both DATA and STATUS characteristics are subscribed
    if (!notifies_ready()) {
        send_status(STAT_SUBSCRIPTION_REQUIRED);
        return 0;
    }

    // Check if SD card is available
    if (!sd_storage_is_available()) {
        ESP_LOGE(TAG, "SD card not available for file transfer");
        send_status(STAT_FILE_OPEN_FAIL);
        return 0;
    }

    const char *rec_dir = "/sdcard/rec";
    DIR *dir = opendir(rec_dir);
    if (!dir) {
        ESP_LOGW(TAG, "Failed to open recordings directory: %s", rec_dir);
        send_status(STAT_FILE_OPEN_FAIL);
        return 0;
    }

    struct dirent *entry;
    char *raw_files[256]; // Support up to 256 files
    uint32_t file_count = 0;
    time_t *file_times = calloc(256, sizeof(time_t));

    if (!file_times) {
        closedir(dir);
        send_status(STAT_FILE_OPEN_FAIL);
        return 0;
    }

    // Collect all .raw files with their modification times
    while ((entry = readdir(dir)) != NULL && file_count < 256) {
        if (entry->d_type != DT_REG) continue;

        char *ext = strrchr(entry->d_name, '.');
        if (!ext || strcmp(ext, ".raw") != 0) continue;

        char full_path[SD_MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", rec_dir, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0) {
            raw_files[file_count] = strdup(entry->d_name);
            file_times[file_count] = st.st_mtime;
            file_count++;
        }
    }
    closedir(dir);

    if (file_count == 0) {
        free(file_times);
        ESP_LOGW(TAG, "No .raw files found for selection");
        send_status(STAT_NO_FILE);
        return 0;
    }

    // Sort files by modification time (newest first)
    for (uint32_t i = 0; i < file_count - 1; i++) {
        for (uint32_t j = i + 1; j < file_count; j++) {
            if (file_times[j] > file_times[i]) {
                // Swap times
                time_t temp_time = file_times[i];
                file_times[i] = file_times[j];
                file_times[j] = temp_time;
                // Swap filenames
                char *temp_name = raw_files[i];
                raw_files[i] = raw_files[j];
                raw_files[j] = temp_name;
            }
        }
    }

    // Check if index is valid
    if (file_index >= file_count) {
        ESP_LOGW(TAG, "Invalid file index: %d (max: %lu)", file_index, (unsigned long)(file_count - 1));
        for (uint32_t i = 0; i < file_count; i++) {
            free(raw_files[i]);
        }
        free(file_times);
        send_status(STAT_INVALID_INDEX);
        return 0;
    }

    // Construct full path for selected file
    char full_path[SD_MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%s/%s", rec_dir, raw_files[file_index]);

    // Set the selected filename for transfer
    strncpy(s_current_raw_file, full_path, sizeof(s_current_raw_file) - 1);
    s_current_raw_file[sizeof(s_current_raw_file) - 1] = '\0';

    ESP_LOGI(TAG, "Selected file %d: %s -> %s", file_index, raw_files[file_index], s_current_raw_file);

    // Clean up
    for (uint32_t i = 0; i < file_count; i++) {
        free(raw_files[i]);
    }
    free(file_times);

    // Send success status
    send_status(STAT_FILE_SELECTED);

    // Enqueue the start command to worker task
    ft_msg_t m = { .type = FT_CMD_START };
    if (s_ft_q) xQueueSend(s_ft_q, &m, 0);  // non-blocking

    return 0;
}







static void update_payload_len(uint16_t mtu)
{
    s_mtu = mtu;
    s_payload_max = (mtu > 23) ? (mtu - 3) : 20;
    if (s_payload_max > 180) {
        s_payload_max = 180;
    }
    
    ESP_LOGI(TAG, "MTU updated: %d, payload_max: %zu", s_mtu, (size_t)s_payload_max);
}

// Helper functions for worker task pipeline

static void send_status(uint8_t code) {
    uint8_t b[1] = { code };
    if (!s_file_transfer_conn_handle || !s_file_transfer_status_handle) return;
    struct os_mbuf *om = ble_hs_mbuf_from_flat(b, sizeof(b));
    if (om) ble_gatts_notify_custom(s_file_transfer_conn_handle, s_file_transfer_status_handle, om);
}

static inline bool notifies_ready(void) { 
    return (s_cccd_mask & 0x03) == 0x03; 
}

static inline size_t payload_budget(uint16_t conn_handle) {
    int mtu = ble_att_mtu(conn_handle);
    if (mtu <= 0) mtu = 23;
    int budget = mtu - 3 - FILE_TRANSFER_HEADER_SIZE;
    if (budget < 1) budget = 1;
    if (budget > (FT_PKT_MAX - FILE_TRANSFER_HEADER_SIZE)) {
        budget = FT_PKT_MAX - FILE_TRANSFER_HEADER_SIZE;
    }
    return (size_t)budget;
}

static bool handles_valid(void) {
    return s_file_transfer_conn_handle != 0 &&
           s_file_transfer_data_handle  != 0;
}

static esp_err_t find_latest_raw(char out_path[], size_t out_sz) {
    const char *rec_dir = "/sdcard/rec";
    DIR *dir = opendir(rec_dir);
    if (!dir) return ESP_FAIL;

    struct dirent *ent;
    struct stat st;
    time_t best_mtime = 0;
    char best[SD_MAX_PATH] = {0};

    while ((ent = readdir(dir)) != NULL) {
        const char *name = ent->d_name;
        size_t len = strlen(name);
        if (len < 4) continue;
        if (strcasecmp(name + len - 4, ".raw") != 0) continue;

        char full[SD_MAX_PATH];
        int n = snprintf(full, sizeof(full), "%s/%s", rec_dir, name);
        if (n <= 0 || n >= (int)sizeof(full)) continue;

        if (stat(full, &st) == 0 && S_ISREG(st.st_mode)) {
            if (st.st_mtime > best_mtime) {
                best_mtime = st.st_mtime;
                strncpy(best, full, sizeof(best) - 1);
            }
        }
    }
    closedir(dir);
    if (!best[0]) return ESP_ERR_NOT_FOUND;
    strncpy(out_path, best, out_sz - 1);
    out_path[out_sz - 1] = '\0';
    return ESP_OK;
}

// File transfer worker task
static void file_xfer_task(void *arg)
{
    (void)arg;
    ft_msg_t msg;
    for (;;) {
        if (!xQueueReceive(s_ft_q, &msg, portMAX_DELAY)) continue;

        if (msg.type == FT_CMD_START) {
            if (s_file_transfer_active) {
                ESP_LOGW(TAG, "Worker: START ignored, transfer already active");
                send_status(STAT_BUSY);
                continue;
            }
            if (!handles_valid()) {
                ESP_LOGE(TAG, "Worker: invalid BLE handles");
                send_status(STAT_NO_CONN);
                continue;
            }

            // choose file
            char path[SD_MAX_PATH] = {0};
            if (s_current_raw_file[0] != '\0') {
                strncpy(path, s_current_raw_file, sizeof(path) - 1);
            } else {
                if (find_latest_raw(path, sizeof(path)) != ESP_OK) {
                    ESP_LOGE(TAG, "Worker: no .raw file found");
                    send_status(STAT_NO_FILE);
                    continue;
                }
            }

            FILE *fp = fopen(path, "rb");
            if (!fp) {
                ESP_LOGE(TAG, "Worker: fopen failed %s errno=%d", path, errno);
                send_status(STAT_FILE_OPEN_FAIL);
                continue;
            }

            // size and init
            if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); send_status(STAT_FILE_READ_FAIL); continue; }
            long lsz = ftell(fp);
            if (lsz < 0)           { fclose(fp); send_status(STAT_FILE_READ_FAIL); continue; }
            rewind(fp);

            s_file_transfer_size   = (uint32_t)lsz;
            s_file_transfer_offset = 0;
            s_bytes_sent           = 0;
            s_seq                  = 0;
            s_file_transfer_active = true;
            s_file_transfer_paused = false;
            s_file_transfer_fp     = fp;

            // Guard against zero-size files
            if (s_file_transfer_size == 0) { 
                fclose(fp); 
                send_status(STAT_NO_FILE); 
                continue; 
            }

            ESP_LOGI(TAG, "Worker: start %s size=%" PRIu32, path, s_file_transfer_size);
            send_status(STAT_STARTED);

            uint8_t pkt[FT_PKT_MAX];
            const size_t hdr = FILE_TRANSFER_HEADER_SIZE;

            while (s_file_transfer_active && !s_file_transfer_paused) {
                // Connection check before each notify
                if (!s_file_transfer_conn_handle) { 
                    send_status(STAT_NO_CONN); 
                    break; 
                }

                uint32_t remain = s_file_transfer_size - s_file_transfer_offset;
                if (remain == 0) break;

                size_t budget = payload_budget(s_file_transfer_conn_handle);
                size_t to_read = remain < budget ? remain : budget;

                size_t n = fread(pkt + hdr, 1, to_read, fp);
                if (n == 0) {
                    if (feof(fp)) break;
                    ESP_LOGE(TAG, "Worker: fread error at %" PRIu32, s_file_transfer_offset);
                    send_status(STAT_FILE_READ_FAIL);
                    break;
                }

                bool eof = (s_file_transfer_offset + n >= s_file_transfer_size);

                // header little endian
                pkt[0] = (uint8_t)(s_seq & 0xFF);
                pkt[1] = (uint8_t)((s_seq >> 8) & 0xFF);
                pkt[2] = (uint8_t)(n & 0xFF);
                pkt[3] = (uint8_t)((n >> 8) & 0xFF);
                pkt[4] = eof ? 0x01 : 0x00;

                // Wait for a credit so we never exceed kMaxInFlight in-flight notifies
                if (s_notify_sem) {
                    ESP_LOGI(TAG, "Worker: Waiting for credit...");
                    // Use a finite wait to allow stop/abort responsiveness
                    if (xSemaphoreTake(s_notify_sem, pdMS_TO_TICKS(200)) != pdTRUE) {
                        // Timed out waiting for credit: treat as backpressure
                        ESP_LOGW(TAG, "Worker: Timed out waiting for credit - backpressure!");
                        vTaskDelay(pdMS_TO_TICKS(10));
                        continue;
                    }
                    ESP_LOGI(TAG, "Worker: Got credit, proceeding with send");
                }

                bool credit_taken = true;

                // bounded retries on allocation + controller backpressure
                int tries = 0;
                for (;;) {
                    struct os_mbuf *om = ble_hs_mbuf_from_flat(pkt, (uint16_t)(hdr + n));
                    if (!om) {
                        // transient mbuf starvation – back off and retry with exponential backoff
                        if (++tries < FT_MAX_RETRIES) {
                            // Use exponential backoff: 10ms, 20ms, 40ms, 80ms, 160ms
                            uint32_t delay_ms = 10 * (1 << (tries - 1));
                            if (delay_ms > 100) delay_ms = 100; // Cap at 100ms
                            ESP_LOGW(TAG, "Worker: mbuf alloc failed, retry %d/%d after %lu ms", tries, FT_MAX_RETRIES, (unsigned long)delay_ms);
                            vTaskDelay(pdMS_TO_TICKS(delay_ms));
                            continue;
                        }
                        ESP_LOGE(TAG, "Worker: mbuf alloc failed after %d tries", tries);
                        send_status(STAT_NOTIFY_FAIL);
                        // Return the credit we took
                        if (credit_taken && s_notify_sem) {
                            xSemaphoreGive(s_notify_sem);
                            ESP_LOGI(TAG, "Credit returned: mbuf alloc failed");
                        }
                        credit_taken = false;
                        break; // give up on this chunk / end transfer below
                    }

                    int rc = ble_gatts_notify_custom(s_file_transfer_conn_handle,
                                                     s_file_transfer_data_handle, om);
                    if (rc == 0) {
                        // Success: credit will be returned in BLE_GAP_EVENT_NOTIFY_TX
                        break;
                    }

                    // on error we still own 'om'
                    os_mbuf_free_chain(om);

                    // controller/backpressure → brief backoff and retry
                    if (rc == BLE_HS_ECONTROLLER || rc == BLE_HS_ENOMEM || rc == BLE_HS_EBUSY) {
                        if (++tries < FT_MAX_RETRIES) {
                            vTaskDelay(pdMS_TO_TICKS(8));
                            continue;
                        }
                    }

                    ESP_LOGE(TAG, "Worker: notify failed rc=%d after %d tries", rc, tries);
                    send_status(STAT_NOTIFY_FAIL);
                    // Return the credit we took
                    if (credit_taken && s_notify_sem) {
                        xSemaphoreGive(s_notify_sem);
                        ESP_LOGI(TAG, "Credit returned: notify failed rc=%d", rc);
                    }
                    credit_taken = false;
                    break;
                }

                if (tries >= FT_MAX_RETRIES) {
                    // abort the transfer cleanly
                    s_file_transfer_active = false;
                    break;
                }

                s_file_transfer_offset += (uint32_t)n;
                s_bytes_sent           += (uint32_t)n;
                s_seq++;

                if (eof) break;
                vTaskDelay(pdMS_TO_TICKS(4));   // gentle pacing
            }

            fclose(fp);
            s_file_transfer_fp     = NULL;
            bool completed         = (s_file_transfer_offset == s_file_transfer_size);
            s_file_transfer_active = false;

            if (completed) {
                ESP_LOGI(TAG, "Worker: complete bytes=%" PRIu32, s_bytes_sent);
                send_status(STAT_COMPLETE);
            } else if (!s_file_transfer_paused) {
                // treat as host stop or error
                send_status(STAT_STOPPED_BY_HOST);
            }
        }
        else if (msg.type == FT_CMD_STOP) {
            ESP_LOGI(TAG, "Worker: STOP");
            s_file_transfer_active = false;
            s_file_transfer_paused = false;
            if (s_file_transfer_fp) {
                fclose(s_file_transfer_fp);
                s_file_transfer_fp = NULL;
            }
            send_status(STAT_STOPPED_BY_HOST);
        }
    }
}

static void start_file_xfer_task(void)
{
    s_ft_q = xQueueCreate(8, sizeof(ft_msg_t));
    configASSERT(s_ft_q);
    s_notify_sem = xSemaphoreCreateCounting(kMaxInFlight, kMaxInFlight);
    configASSERT(s_notify_sem);
    ESP_LOGI(TAG, "Credit semaphore created with %d credits", kMaxInFlight);
    BaseType_t ok = xTaskCreate(file_xfer_task, "file_xfer", 8192, NULL, 5, NULL);
    configASSERT(ok == pdPASS);
    ESP_LOGI(TAG, "File transfer worker task started");
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
    ble_hs_cfg.sync_cb = ble_app_on_sync;
    ble_hs_cfg.reset_cb = on_reset;             // Add proper reset callback
    ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr_item;
    
    // Initialize GAP and GATT services
    ble_svc_gap_init();
    ble_svc_gatt_init();
    
    // Set preferred MTU for optimal file transfer performance
    ble_att_set_preferred_mtu(185);
    ESP_LOGI(TAG, "Preferred MTU set to 185");
    
    // Preflight GATT table
    gatt_preflight();

    // Register our custom GATT services
    // Note: gatt_svr_register_cb is already set via ble_hs_cfg.gatts_register_cb above
    ESP_ERROR_CHECK(ble_gatts_count_cfg(gatt_svr_svcs));
    ESP_ERROR_CHECK(ble_gatts_add_svcs(gatt_svr_svcs));
    ESP_LOGI(TAG, "GATT services registered");

// Start the worker task
start_file_xfer_task();

    // Do NOT call ble_gatts_start(); host starts GATT itself
    ESP_LOGI(TAG, "Handles - DATA=%u STATUS=%u",
             (unsigned)s_file_transfer_data_handle, (unsigned)s_file_transfer_status_handle);
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

    // Initialize audio capture after UI is working
    ESP_LOGI(TAG, "Initializing audio capture...");
    ret = audio_capture_init(16000, 1);   // 16kHz, mono (HIGH QUALITY!)
    if (ret == ESP_OK) {
        s_audio_capture_enabled = true;
        ESP_LOGI(TAG, "Audio capture initialized successfully");
        ESP_LOGI(TAG, "  Real audio recording ENABLED");
        ESP_LOGI(TAG, "  Microphone: GPIO9 (MIC)");
        ESP_LOGI(TAG, "  Sample Rate: 16kHz (HIGH QUALITY!)");
        ESP_LOGI(TAG, "  Audio Format: Mono, 16-bit");
        
        // Initialize raw audio storage system
        ESP_LOGI(TAG, "Initializing raw audio storage system...");
        esp_err_t raw_ret = raw_audio_storage_init();
        if (raw_ret == ESP_OK) {
            ESP_LOGI(TAG, "Raw audio storage initialized successfully");

            // Initialize ADC sample queue for decoupling real-time sampling from file I/O
            ESP_LOGI(TAG, "Creating ADC sample queue...");
            s_adc_sample_queue = xQueueCreate(2048, sizeof(uint16_t)); // Buffer for ~0.13 seconds of samples
            if (!s_adc_sample_queue) {
                ESP_LOGE(TAG, "Failed to create ADC sample queue");
                return;
            }

            // Create storage task for safe file I/O operations
            ESP_LOGI(TAG, "Creating storage task...");
            BaseType_t task_ret = xTaskCreate(
                storage_task,
                "audio_storage",
                4096,  // Same stack size as audio capture task
                NULL,
                4,     // Lower priority than audio capture (5) but higher than idle
                NULL
            );

            if (task_ret != pdPASS) {
                ESP_LOGE(TAG, "Failed to create storage task");
                return;
            }

            ESP_LOGI(TAG, "Storage task created successfully");

            // Register raw ADC callback for queue-based storage
            audio_capture_set_raw_adc_callback(raw_adc_callback, NULL);
            ESP_LOGI(TAG, "Raw ADC callback registered - queue-based ADC storage enabled");
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
        ESP_LOGW(TAG, "Audio capture disabled - button will only toggle LED");
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
            ESP_LOGI(TAG, "  💡 Short press: Toggle LED ON/OFF (audio disabled)");
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
                ESP_LOGI(TAG, "Raw Audio Stats - Samples: %u, File Size: %u bytes", (unsigned)samples_written, (unsigned)file_size_bytes);
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
