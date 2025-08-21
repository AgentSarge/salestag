#include "sdcard.h"
#include "esp_log.h"
#include "driver/sdspi_host.h"
#include "driver/spi_master.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

static const char *TAG_SD = "sdcard";
static bool s_mounted = false;
static sdmmc_card_t *s_card = NULL;

esp_err_t sdcard_mount(const char *mount_point, bool format_if_mount_failed) {
    if (s_mounted) {
        return ESP_OK;
    }
    ESP_LOGI(TAG_SD, "Mounting SD card at %s", mount_point);

    const esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = format_if_mount_failed,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    // Configure SPI bus (use pins per hardware: MOSI=IO35, MISO=IO37, SCLK=IO36, CS=IO39)
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = 35,
        .miso_io_num = 37,
        .sclk_io_num = 36,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = 39; // IO39 CS
    slot_config.host_id = host.slot;

    esp_err_t ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &s_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_SD, "Failed to mount SD card: %s", esp_err_to_name(ret));
        spi_bus_free(SPI2_HOST);
        return ret;
    }

    s_mounted = true;
    uint64_t cap_mb = (uint64_t)s_card->csd.capacity / (1024ULL * 1024ULL);
    ESP_LOGI(TAG_SD, "SD mounted. Capacity: %llu MB", (unsigned long long)cap_mb);
    return ESP_OK;
}

void sdcard_unmount(void) {
    if (!s_mounted) return;
    esp_vfs_fat_sdcard_unmount("/sdcard", s_card);
    spi_bus_free(SPI2_HOST);
    s_card = NULL;
    s_mounted = false;
    ESP_LOGI(TAG_SD, "SD unmounted");
}

bool sdcard_is_mounted(void) { return s_mounted; }
