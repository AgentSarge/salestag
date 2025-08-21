#pragma once

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Mount the SD card at the given mount point using SDSPI host.
 * Returns ESP_OK on success. Safe to call multiple times.
 */
esp_err_t sdcard_mount(const char *mount_point, bool format_if_mount_failed);

/** Unmount and release resources. Safe if not mounted. */
void sdcard_unmount(void);

/** Returns true if currently mounted. */
bool sdcard_is_mounted(void);

#ifdef __cplusplus
}
#endif
