#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// WiFi configuration
typedef struct {
    char ssid[32];
    char password[64];
    bool enable_ap;  // Enable Access Point mode
    char ap_ssid[32];
    char ap_password[64];
} wifi_manager_config_t;

// Initialize WiFi
esp_err_t wifi_manager_init(const wifi_manager_config_t *config);

// Start WiFi connection
esp_err_t wifi_manager_start(void);

// Get WiFi status
bool wifi_manager_is_connected(void);

// Get IP address
esp_err_t wifi_manager_get_ip(char *ip_str, size_t max_len);

// Deinitialize WiFi
esp_err_t wifi_manager_deinit(void);

#ifdef __cplusplus
}
#endif
