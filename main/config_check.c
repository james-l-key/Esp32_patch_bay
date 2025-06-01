/**
 * @file config_check.c
 * @brief Implementation of GPIO protection and configuration validation functions
 * @author ESP32 Patch Bay Project
 *
 * This file implements checks for ESP32-S3 build targets and identifies potential
 * GPIO conflicts that may arise from different SPI flash and PSRAM configurations.
 * It helps prevent hardware issues when using GPIO pins that might be reserved
 * by system components.
 */

#include "config_check.h"
#include <stdlib.h> // For abort()

#ifdef CONFIG_IDF_TARGET_ESP32S3
// This code will only compile for ESP32-S3 target
#define MY_TARGET_ESP32S3 1
#else
#define MY_TARGET_ESP32S3 0
#endif

static const char *TAG = "GPIO_PROTECTION";

/**
 * @brief Check if the current build target is ESP32-S3
 *
 * Verifies whether the code is being compiled for an ESP32-S3 target
 * and logs appropriate messages based on the result.
 *
 * @return true if the target is ESP32-S3, false otherwise
 */
bool check_build_target(void)
{
    if (MY_TARGET_ESP32S3)
    {
        ESP_LOGI(TAG, "Build target is ESP32-S3.");
        return true;
    }
    else
    {
        ESP_LOGW(TAG, "Build target is NOT ESP32-S3. GPIO conflicts may occur on other targets.");
        return false;
    }
}

/**
 * @brief Check SPI configuration and warn about potential GPIO conflicts
 *
 * For ESP32-S3 targets, this function logs information about the current
 * SPI flash and PSRAM configurations and issues warnings about specific GPIO pins
 * that might be reserved by these interfaces, particularly GPIOs 33-37 for
 * Octal SPI PSRAM and GPIOs 35-37 for Octal SPI Flash.
 *
 * @return true if any critical GPIO conflicts were detected, false otherwise
 */
bool check_spi_config_and_warn_gpio_conflicts(void)
{
    bool critical_conflict_detected = false;

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
#endif

        // Check PSRAM Type (if enabled)
#ifdef CONFIG_SPIRAM_ENABLE
        ESP_LOGI(TAG, "PSRAM is enabled.");
#if CONFIG_SPIRAM_MODE == ESP_SPIRAM_MODE_QUAD
        ESP_LOGI(TAG, "Quad SPI PSRAM is configured.");
#elif CONFIG_SPIRAM_MODE == ESP_SPIRAM_MODE_OCT
        ESP_LOGI(TAG, "Octal SPI PSRAM is configured.");
        // Octal PSRAM *definitely* uses GPIOs 33-37.
        ESP_LOGW(TAG, "Octal PSRAM uses GPIOs 33, 34, 35, 36, and 37. These pins are likely unavailable for other uses.");

        // Check for specific pin configurations that would conflict with Octal PSRAM
        // This is where you would add checks for pins used in your project
        // For example, check if buttons or matrix pins are configured to use reserved GPIOs

        /* Example:
        if (CONFIG_SOME_BUTTON_PIN >= 33 && CONFIG_SOME_BUTTON_PIN <= 37) {
            ESP_LOGE(TAG, "CRITICAL: Button configured to use GPIO %d which conflicts with Octal PSRAM!",
                     CONFIG_SOME_BUTTON_PIN);
            critical_conflict_detected = true;
        }
        */
#else
        ESP_LOGW(TAG, "Unknown PSRAM mode configured. Review CONFIG_SPIRAM_MODE.");
#endif
#else
        ESP_LOGI(TAG, "PSRAM is disabled.");
#endif

        // Add specific dev kit checks here
        // #if defined(CONFIG_ESP32S3_DEV_KIT)
        //     // Check for known issues with this specific dev kit
        // #endif
    }
    else
    {
        ESP_LOGI(TAG, "Not an ESP32-S3 target. Skipping specific S3 SPI configuration checks.");
    }

    return critical_conflict_detected;
}

/**
 * @brief Run all GPIO protection checks
 *
 * This function calls both check_build_target() and
 * check_spi_config_and_warn_gpio_conflicts() to perform a complete
 * verification of the build configuration and identify potential GPIO conflicts.
 *
 * @param abort_on_critical_conflict If true, the program will abort if critical conflicts are detected
 * @return true if any critical conflicts were detected, false otherwise
 */
bool run_gpio_protection_checks(bool abort_on_critical_conflict)
{
    bool is_s3 = check_build_target();
    bool has_critical_conflicts = false;

    if (is_s3)
    {
        has_critical_conflicts = check_spi_config_and_warn_gpio_conflicts();

        if (has_critical_conflicts)
        {
            ESP_LOGE(TAG, "Critical GPIO conflicts detected! Review your pin configuration.");

            if (abort_on_critical_conflict)
            {
                ESP_LOGE(TAG, "Aborting due to critical GPIO conflicts.");
                abort(); // This will halt the program
            }
        }
        else
        {
            ESP_LOGI(TAG, "No critical GPIO conflicts detected.");
        }
    }

    return has_critical_conflicts;
}