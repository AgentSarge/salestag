#ifndef USB_AUDIO_TRANSFER_H
#define USB_AUDIO_TRANSFER_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

// USB transfer configuration
#define USB_AUDIO_BUFFER_SIZE 32768  // 32KB internal buffer
#define USB_AUDIO_MAX_RECORDINGS 10  // Store up to 10 recordings
#define USB_AUDIO_SAMPLE_RATE 16000
#define USB_AUDIO_CHANNELS 2

// USB transfer states
typedef enum {
    USB_AUDIO_IDLE,
    USB_AUDIO_RECORDING,
    USB_AUDIO_TRANSFERRING,
    USB_AUDIO_ERROR
} usb_audio_state_t;

// Audio recording metadata
typedef struct {
    uint32_t recording_id;
    uint32_t start_timestamp;
    uint32_t end_timestamp;
    uint32_t sample_count;
    uint32_t data_size;
    bool is_complete;
} usb_audio_metadata_t;

// Initialize USB audio transfer system
esp_err_t usb_audio_transfer_init(void);

// Start recording raw audio to internal buffer
esp_err_t usb_audio_transfer_start_recording(uint32_t recording_id);

// Stop recording and finalize the recording
esp_err_t usb_audio_transfer_stop_recording(void);

// Add raw audio sample to current recording
esp_err_t usb_audio_transfer_add_sample(uint16_t mic1_adc, uint16_t mic2_adc);

// Get list of available recordings
esp_err_t usb_audio_transfer_get_recordings(usb_audio_metadata_t *recordings, uint32_t *count);

// Get raw audio data for a specific recording
esp_err_t usb_audio_transfer_get_recording_data(uint32_t recording_id, uint8_t *buffer, uint32_t buffer_size, uint32_t *data_size);

// Delete a specific recording
esp_err_t usb_audio_transfer_delete_recording(uint32_t recording_id);

// Get current transfer state
usb_audio_state_t usb_audio_transfer_get_state(void);

// Check if currently recording
bool usb_audio_transfer_is_recording(void);

// Get current recording statistics
esp_err_t usb_audio_transfer_get_stats(uint32_t *samples_written, uint32_t *buffer_used);

// Deinitialize USB audio transfer system
esp_err_t usb_audio_transfer_deinit(void);

#endif // USB_AUDIO_TRANSFER_H
