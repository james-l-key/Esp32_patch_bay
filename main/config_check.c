#include "sdkconfig.h" // Essential for accessing Kconfig options

#ifdef CONFIG_IDF_TARGET_ESP32S3
// This code will only compile for ESP32-S3 target
#define MY_TARGET_ESP32S3 1
#else
#define MY_TARGET_ESP32S3 0
#endif

// In your application code:
void check_build_target()
{
    if (MY_TARGET_ESP32S3)
    {
        ESP_LOGI("PROTECTION_CHECK", "Build target is ESP32-S3.");
    }
    else
    {
        ESP_LOGW("PROTECTION_CHECK", "Build target is NOT ESP32-S3. GPIO conflicts may occur on other targets.");
    }
}

#include "sdkconfig.h"
#include "esp_log.h" // For ESP_LOGI, ESP_LOGW

static const char *TAG = "GPIO_PROTECTION";

void check_spi_config_and_warn_gpio_conflicts()
{
    if (MY_TARGET_ESP32S3)
    {
        ESP_LOGI(TAG, "Running GPIO protection checks for ESP32-S3...");

// Check Flash Type
#ifdef CONFIG_ESPTOOLPY_OCT_FLASH
        ESP_LOGI(TAG, "Octal SPI Flash is configured.");
        // Warn about potential GPIO conflicts for Octal Flash
        ESP_LOGW(TAG, "Octal Flash uses GPIOs 35, 36, and 37. Avoid using these pins for other purposes.");
#else
        ESP_LOGI(TAG, "Quad SPI Flash is configured.");
        // Quad Flash generally uses fewer pins for the main flash interface (usually not 35-37).
        // You might still want to warn about specific pins if you know your dev kit uses them for other functions.
#endif

// Check PSRAM Type (if enabled)
#ifdef CONFIG_SPIRAM_ENABLE
        ESP_LOGI(TAG, "PSRAM is enabled.");
#if CONFIG_SPIRAM_MODE == ESP_SPIRAM_MODE_QUAD
        ESP_LOGI(TAG, "Quad SPI PSRAM is configured.");
        // Quad PSRAM might use specific pins, but less likely to conflict with 35-37 than Octal.
#elif CONFIG_SPIRAM_MODE == ESP_SPIRAM_MODE_OCT
        ESP_LOGI(TAG, "Octal SPI PSRAM is configured.");
        // Octal PSRAM *definitely* uses GPIOs 33-37.
        ESP_LOGW(TAG, "Octal PSRAM uses GPIOs 33, 34, 35, 36, and 37. These pins are likely unavailable for other uses.");
#else
        ESP_LOGW(TAG, "Unknown PSRAM mode configured. Review CONFIG_SPIRAM_MODE.");
#endif
#else
        ESP_LOGI(TAG, "PSRAM is disabled.");
#endif

        // Add more specific checks for your dev kits here
        // For example, if you know a specific dev kit model has a common GPIO conflict:
        // #if defined(CONFIG_MY_DEV_KIT_MODEL_A) && defined(CONFIG_ESPTOOLPY_OCT_FLASH)
        //     ESP_LOGE(TAG, "ERROR: Dev Kit Model A with Octal Flash has a known conflict on GPIOXX.");
        // #endif
    }
    else
    {
        ESP_LOGI(TAG, "Not an ESP32-S3 target. Skipping specific S3 SPI configuration checks.");
    }
}