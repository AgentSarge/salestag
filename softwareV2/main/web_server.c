#include "web_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "cJSON.h"
#include "sd_storage.h"
#include "recorder.h"
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

static const char *TAG = "web_server";

// HTTP server handle
static httpd_handle_t s_server = NULL;

// HTML templates
static const char *HTML_HEADER = 
    "<!DOCTYPE html>"
    "<html><head>"
    "<title>SalesTag Recorder</title>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<style>"
    "body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }"
    ".container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }"
    ".header { text-align: center; color: #333; margin-bottom: 30px; }"
    ".status { background: #e8f5e8; padding: 15px; border-radius: 5px; margin-bottom: 20px; }"
    ".controls { text-align: center; margin-bottom: 30px; }"
    ".btn { background: #007bff; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; margin: 5px; }"
    ".btn:hover { background: #0056b3; }"
    ".btn.recording { background: #dc3545; }"
    ".btn.recording:hover { background: #c82333; }"
    ".file-list { background: #f8f9fa; padding: 15px; border-radius: 5px; }"
    ".file-item { display: flex; justify-content: space-between; align-items: center; padding: 10px; border-bottom: 1px solid #dee2e6; }"
    ".file-item:last-child { border-bottom: none; }"
    ".file-info { flex-grow: 1; }"
    ".file-name { font-weight: bold; color: #333; }"
    ".file-meta { color: #666; font-size: 0.9em; }"
    ".file-actions { display: flex; gap: 10px; }"
    ".btn-small { padding: 5px 10px; font-size: 0.9em; }"
    ".btn-success { background: #28a745; }"
    ".btn-success:hover { background: #218838; }"
    ".btn-danger { background: #dc3545; }"
    ".btn-danger:hover { background: #c82333; }"
    "</style>"
    "</head><body>";

static const char *HTML_FOOTER = "</body></html>";

#define HTML_BUFFER_SIZE 8192

// Function prototypes
static esp_err_t root_handler(httpd_req_t *req);
static esp_err_t api_status_handler(httpd_req_t *req);
static esp_err_t api_record_start_handler(httpd_req_t *req);
static esp_err_t api_record_stop_handler(httpd_req_t *req);
static esp_err_t api_files_handler(httpd_req_t *req);
static esp_err_t download_handler(httpd_req_t *req);
static esp_err_t delete_file_handler(httpd_req_t *req);

// Helper function to format file size
static void format_file_size(uint64_t size, char *buffer, size_t buffer_size) {
    const char *units[] = {"B", "KB", "MB", "GB"};
    int unit_index = 0;
    double display_size = (double)size;
    
    while (display_size >= 1024.0 && unit_index < 3) {
        display_size /= 1024.0;
        unit_index++;
    }
    
    if (unit_index == 0) {
        snprintf(buffer, buffer_size, "%.0f %s", display_size, units[unit_index]);
    } else {
        snprintf(buffer, buffer_size, "%.1f %s", display_size, units[unit_index]);
    }
}

// Helper function to get file duration from WAV header
static uint32_t get_wav_duration(const char *filepath) {
    FILE *file = fopen(filepath, "rb");
    if (!file) return 0;
    
    // Skip to data chunk size (offset 40)
    fseek(file, 40, SEEK_SET);
    uint32_t data_size;
    if (fread(&data_size, sizeof(uint32_t), 1, file) != 1) {
        fclose(file);
        return 0;
    }
    
    fclose(file);
    
    // Calculate duration: data_size / (sample_rate * channels * bits_per_sample / 8)
    // For 16kHz, 16-bit, mono: data_size / (16000 * 1 * 16 / 8) = data_size / 32000
    return data_size / 32000; // Duration in seconds
}

esp_err_t web_server_init(void) {
    ESP_LOGI(TAG, "Initializing web server");
    return ESP_OK;
}

esp_err_t web_server_start(void) {
    if (s_server) {
        ESP_LOGW(TAG, "Web server already running");
        return ESP_OK;
    }
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16;
    config.stack_size = 8192;
    
    esp_err_t ret = httpd_start(&s_server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start web server: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Register URI handlers
    httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &root_uri);
    
    httpd_uri_t status_uri = {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = api_status_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &status_uri);
    
    httpd_uri_t record_start_uri = {
        .uri = "/api/record/start",
        .method = HTTP_POST,
        .handler = api_record_start_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &record_start_uri);
    
    httpd_uri_t record_stop_uri = {
        .uri = "/api/record/stop",
        .method = HTTP_POST,
        .handler = api_record_stop_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &record_stop_uri);
    
    httpd_uri_t files_uri = {
        .uri = "/api/files",
        .method = HTTP_GET,
        .handler = api_files_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &files_uri);
    
    httpd_uri_t download_uri = {
        .uri = "/download",
        .method = HTTP_GET,
        .handler = download_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &download_uri);
    
    httpd_uri_t delete_uri = {
        .uri = "/api/delete",
        .method = HTTP_POST,
        .handler = delete_file_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &delete_uri);
    
    ESP_LOGI(TAG, "Web server started successfully");
    return ESP_OK;
}

esp_err_t web_server_stop(void) {
    if (s_server) {
        httpd_stop(s_server);
        s_server = NULL;
        ESP_LOGI(TAG, "Web server stopped");
    }
    return ESP_OK;
}

esp_err_t web_server_deinit(void) {
    web_server_stop();
    ESP_LOGI(TAG, "Web server deinitialized");
    return ESP_OK;
}

// Root page handler
static esp_err_t root_handler(httpd_req_t *req) {
    char html_content[HTML_BUFFER_SIZE];
    
    // Set response type
    httpd_resp_set_type(req, "text/html");
    
    // Get recording status
    uint32_t bytes_written, duration_ms;
    recorder_get_stats(&bytes_written, &duration_ms);
    
    snprintf(html_content, sizeof(html_content),
        "%s"
        "<div class='container'>"
        "<div class='header'>"
        "<h1>🎵 SalesTag Recorder</h1>"
        "<p>Web Interface for Audio Recording Management</p>"
        "</div>"
        
        "<div class='status'>"
        "<h3>📊 Recording Status</h3>"
        "<p><strong>Bytes Written:</strong> %lu bytes</p>"
        "<p><strong>Duration:</strong> %lu ms</p>"
        "</div>"
        
        "<div class='controls'>"
        "<button class='btn' onclick='startRecording()'>🎙️ Start Recording</button>"
        "<button class='btn recording' onclick='stopRecording()'>⏹️ Stop Recording</button>"
        "<button class='btn' onclick='refreshFiles()'>🔄 Refresh Files</button>"
        "</div>"
        
        "<div class='file-list'>"
        "<h3>📁 Recorded Files</h3>"
        "<div id='fileList'>Loading...</div>"
        "</div>"
        "</div>"
        
        "<script>"
        "function startRecording() {"
        "  fetch('/api/record/start', {method: 'POST'})"
        "    .then(response => response.json())"
        "    .then(data => {"
        "      if (data.success) {"
        "        alert('Recording started!');"
        "        setTimeout(refreshFiles, 1000);"
        "      } else {"
        "        alert('Failed to start recording: ' + data.error);"
        "      }"
        "    });"
        "}"
        "function stopRecording() {"
        "  fetch('/api/record/stop', {method: 'POST'})"
        "    .then(response => response.json())"
        "    .then(data => {"
        "      if (data.success) {"
        "        alert('Recording stopped!');"
        "        setTimeout(refreshFiles, 1000);"
        "      } else {"
        "        alert('Failed to stop recording: ' + data.error);"
        "      }"
        "    });"
        "}"
        "function refreshFiles() {"
        "  fetch('/api/files')"
        "    .then(response => response.json())"
        "    .then(data => {"
        "      const fileList = document.getElementById('fileList');"
        "      if (data.files && data.files.length > 0) {"
        "        let html = '';"
        "        data.files.forEach(file => {"
        "          html += '<div class=\"file-item\">';"
        "          html += '<div class=\"file-info\">';"
        "          html += '<div class=\"file-name\">' + file.name + '</div>';"
        "          html += '<div class=\"file-meta\">' + file.size + ' • ' + file.duration + 's</div>';"
        "          html += '</div>';"
        "          html += '<div class=\"file-actions\">';"
        "          html += '<a href=\"/download?file=\" + encodeURIComponent(file.name) + '\" class=\"btn btn-small btn-success\">⬇️ Download</a>';"
        "          html += '<button onclick=\"deleteFile('\" + file.name + \"')\" class=\"btn btn-small btn-danger\">🗑️ Delete</button>';"
        "          html += '</div>';"
        "          html += '</div>';"
        "        });"
        "        fileList.innerHTML = html;"
        "      } else {"
        "        fileList.innerHTML = '<p>No recorded files found.</p>';"
        "      }"
        "    });"
        "}"
        "function deleteFile(filename) {"
        "  if (confirm('Are you sure you want to delete ' + filename + '?')) {"
        "    fetch('/api/delete', {"
        "      method: 'POST',"
        "      headers: {'Content-Type': 'application/json'},"
        "      body: JSON.stringify({filename: filename})"
        "    })"
        "    .then(response => response.json())"
        "    .then(data => {"
        "      if (data.success) {"
        "        alert('File deleted successfully!');"
        "        refreshFiles();"
        "      } else {"
        "        alert('Failed to delete file: ' + data.error);"
        "      }"
        "    });"
        "  }"
        "}"
        "// Load files on page load"
        "document.addEventListener('DOMContentLoaded', refreshFiles);"
        "</script>"
        "%s",
        HTML_HEADER, bytes_written, duration_ms, HTML_FOOTER);
    
    httpd_resp_send(req, html_content, strlen(html_content));
    return ESP_OK;
}

// API status handler
static esp_err_t api_status_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    
    uint32_t bytes_written, duration_ms;
    recorder_get_stats(&bytes_written, &duration_ms);
    
    cJSON *response = cJSON_CreateObject();
    cJSON_AddNumberToObject(response, "bytes_written", bytes_written);
    cJSON_AddNumberToObject(response, "duration_ms", duration_ms);
    
    char *json_string = cJSON_Print(response);
    httpd_resp_send(req, json_string, strlen(json_string));
    
    free(json_string);
    cJSON_Delete(response);
    return ESP_OK;
}

// API record start handler
static esp_err_t api_record_start_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    
    esp_err_t ret = recorder_start();
    
    cJSON *response = cJSON_CreateObject();
    if (ret == ESP_OK) {
        cJSON_AddBoolToObject(response, "success", true);
        cJSON_AddStringToObject(response, "message", "Recording started");
    } else {
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddStringToObject(response, "error", esp_err_to_name(ret));
    }
    
    char *json_string = cJSON_Print(response);
    httpd_resp_send(req, json_string, strlen(json_string));
    
    free(json_string);
    cJSON_Delete(response);
    return ESP_OK;
}

// API record stop handler
static esp_err_t api_record_stop_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    
    esp_err_t ret = recorder_stop();
    
    cJSON *response = cJSON_CreateObject();
    if (ret == ESP_OK) {
        cJSON_AddBoolToObject(response, "success", true);
        cJSON_AddStringToObject(response, "message", "Recording stopped");
    } else {
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddStringToObject(response, "error", esp_err_to_name(ret));
    }
    
    char *json_string = cJSON_Print(response);
    httpd_resp_send(req, json_string, strlen(json_string));
    
    free(json_string);
    cJSON_Delete(response);
    return ESP_OK;
}

// API files handler
static esp_err_t api_files_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    
    cJSON *response = cJSON_CreateObject();
    cJSON *files_array = cJSON_CreateArray();
    
    DIR *dir = opendir("/sdcard/rec");
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strstr(entry->d_name, ".wav") != NULL) {
                char filepath[512];
                snprintf(filepath, sizeof(filepath), "/sdcard/rec/%s", entry->d_name);
                
                struct stat st;
                if (stat(filepath, &st) == 0) {
                    cJSON *file_obj = cJSON_CreateObject();
                    cJSON_AddStringToObject(file_obj, "name", entry->d_name);
                    
                    char size_str[32];
                    format_file_size(st.st_size, size_str, sizeof(size_str));
                    cJSON_AddStringToObject(file_obj, "size", size_str);
                    
                    uint32_t duration = get_wav_duration(filepath);
                    cJSON_AddNumberToObject(file_obj, "duration", duration);
                    
                    cJSON_AddItemToArray(files_array, file_obj);
                }
            }
        }
        closedir(dir);
    }
    
    cJSON_AddItemToObject(response, "files", files_array);
    
    char *json_string = cJSON_Print(response);
    httpd_resp_send(req, json_string, strlen(json_string));
    
    free(json_string);
    cJSON_Delete(response);
    return ESP_OK;
}

// Download handler
static esp_err_t download_handler(httpd_req_t *req) {
    char query[128];
    char filename[128];
    
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing filename parameter");
        return ESP_FAIL;
    }
    
    if (httpd_query_key_value(query, "file", filename, sizeof(filename)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid filename parameter");
        return ESP_FAIL;
    }
    
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "/sdcard/rec/%s", filename);
    
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
        return ESP_FAIL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Set response headers
    httpd_resp_set_type(req, "audio/wav");
    httpd_resp_set_hdr(req, "Content-Disposition", "attachment");
    
    char content_length[32];
    snprintf(content_length, sizeof(content_length), "%ld", file_size);
    httpd_resp_set_hdr(req, "Content-Length", content_length);
    
    // Send file in chunks
    char buffer[1024];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (httpd_resp_send_chunk(req, buffer, bytes_read) != ESP_OK) {
            fclose(file);
            return ESP_FAIL;
        }
    }
    
    fclose(file);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

// Delete file handler
static esp_err_t delete_file_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    
    char content[256];
    int received = httpd_req_recv(req, content, sizeof(content) - 1);
    if (received <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No content received");
        return ESP_FAIL;
    }
    content[received] = '\0';
    
    cJSON *json = cJSON_Parse(content);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    cJSON *filename_json = cJSON_GetObjectItem(json, "filename");
    if (!filename_json || !cJSON_IsString(filename_json)) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing filename");
        return ESP_FAIL;
    }
    
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "/sdcard/rec/%s", filename_json->valuestring);
    
    esp_err_t ret = unlink(filepath);
    
    cJSON *response = cJSON_CreateObject();
    if (ret == ESP_OK) {
        cJSON_AddBoolToObject(response, "success", true);
        cJSON_AddStringToObject(response, "message", "File deleted successfully");
    } else {
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddStringToObject(response, "error", esp_err_to_name(ret));
    }
    
    char *json_string = cJSON_Print(response);
    httpd_resp_send(req, json_string, strlen(json_string));
    
    free(json_string);
    cJSON_Delete(response);
    cJSON_Delete(json);
    return ESP_OK;
}
