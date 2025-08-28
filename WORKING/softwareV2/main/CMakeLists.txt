idf_component_register(SRCS "main.c" "ui.c" "sd_storage.c" "audio_capture.c" "raw_audio_storage.c"
                       INCLUDE_DIRS "."
                       REQUIRES driver fatfs sdmmc esp_adc esp_timer)
