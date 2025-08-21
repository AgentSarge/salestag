#include "wifi_manager.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include <string.h>

static const char *TAG = "wifi_manager";

// WiFi configuration
static wifi_manager_config_t s_wifi_config;
static bool s_wifi_initialized = false;
static bool s_wifi_connected = false;
static esp_netif_t *s_sta_netif = NULL;
static esp_netif_t *s_ap_netif = NULL;

// WiFi event handler
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi station started");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(TAG, "WiFi connected to AP");
        s_wifi_connected = true;
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "WiFi disconnected from AP");
        s_wifi_connected = false;
        // Try to reconnect
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START) {
        ESP_LOGI(TAG, "WiFi AP started");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

esp_err_t wifi_manager_init(const wifi_manager_config_t *config) {
    if (s_wifi_initialized) {
        ESP_LOGW(TAG, "WiFi manager already initialized");
        return ESP_OK;
    }
    
    if (!config) {
        ESP_LOGE(TAG, "Invalid configuration");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Copy configuration
    memcpy(&s_wifi_config, config, sizeof(wifi_manager_config_t));
    
    ESP_LOGI(TAG, "Initializing WiFi manager...");
    
    // Initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());
    
    // Create default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // Create network interfaces
    s_sta_netif = esp_netif_create_default_wifi_sta();
    if (s_wifi_config.enable_ap) {
        s_ap_netif = esp_netif_create_default_wifi_ap();
    }
    
    // Initialize WiFi
    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));
    
    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));
    
    // Set WiFi mode
    if (s_wifi_config.enable_ap) {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    } else {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    }
    
    s_wifi_initialized = true;
    ESP_LOGI(TAG, "WiFi manager initialized successfully");
    
    return ESP_OK;
}

esp_err_t wifi_manager_start(void) {
    if (!s_wifi_initialized) {
        ESP_LOGE(TAG, "WiFi manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Starting WiFi...");
    
    // Start WiFi
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Configure and start AP if enabled
    if (s_wifi_config.enable_ap) {
        wifi_config_t ap_config = {
            .ap = {
                .max_connection = 4,
                .authmode = WIFI_AUTH_WPA2_PSK,
            }
        };
        strcpy((char*)ap_config.ap.ssid, s_wifi_config.ap_ssid);
        strcpy((char*)ap_config.ap.password, s_wifi_config.ap_password);
        ap_config.ap.ssid_len = strlen(s_wifi_config.ap_ssid);
        
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
        ESP_LOGI(TAG, "AP mode enabled - SSID: %s, Password: %s",
                 s_wifi_config.ap_ssid, s_wifi_config.ap_password);
    }
    
    // Configure and start STA
    wifi_config_t sta_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        }
    };
    strcpy((char*)sta_config.sta.ssid, s_wifi_config.ssid);
    strcpy((char*)sta_config.sta.password, s_wifi_config.password);
    
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    
    // Connect to AP
    ESP_ERROR_CHECK(esp_wifi_connect());
    
    ESP_LOGI(TAG, "WiFi started successfully");
    return ESP_OK;
}

bool wifi_manager_is_connected(void) {
    return s_wifi_connected;
}

esp_err_t wifi_manager_get_ip(char *ip_str, size_t max_len) {
    if (!s_wifi_connected || !s_sta_netif) {
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(s_sta_netif, &ip_info) != ESP_OK) {
        return ESP_FAIL;
    }
    
    snprintf(ip_str, max_len, IPSTR, IP2STR(&ip_info.ip));
    return ESP_OK;
}

esp_err_t wifi_manager_deinit(void) {
    if (!s_wifi_initialized) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Deinitializing WiFi manager...");
    
    // Stop WiFi
    esp_wifi_stop();
    esp_wifi_deinit();
    
    // Destroy network interfaces
    if (s_sta_netif) {
        esp_netif_destroy(s_sta_netif);
        s_sta_netif = NULL;
    }
    if (s_ap_netif) {
        esp_netif_destroy(s_ap_netif);
        s_ap_netif = NULL;
    }
    
    // Deinitialize TCP/IP stack
    esp_netif_deinit();
    
    s_wifi_initialized = false;
    s_wifi_connected = false;
    
    ESP_LOGI(TAG, "WiFi manager deinitialized");
    return ESP_OK;
}
