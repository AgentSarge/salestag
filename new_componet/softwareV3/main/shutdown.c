#include <stdio.h>
#include "esp_sleep.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void) {
    printf("=== ESP32 SHUTDOWN ===\n");
    printf("Going to deep sleep... Device will be effectively OFF\n");
    printf("To wake up: Press RESET button or power cycle\n");
    printf("Shutting down in 3 seconds...\n");
    
    // Wait a moment for the messages to print
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    printf("💤 GOODNIGHT! Device is now OFF\n");
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Go into deep sleep (effectively OFF until manual reset)
    esp_deep_sleep_start();
}

