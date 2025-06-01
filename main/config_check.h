/**
 * @file config_check.h
 * @brief GPIO protection and configuration validation functions for ESP32-S3
 * @author ESP32 Patch Bay Project
 *
 * This header provides functions to verify build targets and check for potential
 * GPIO conflicts based on SPI flash and PSRAM configurations. These checks help
 * prevent hardware conflicts when using specific GPIO pins that might be reserved
 * by the ESP32-S3 SPI flash or PSRAM interfaces.
 */

#ifndef CONFIG_CHECK_H
#define CONFIG_CHECK_H

#include "sdkconfig.h"
#include "esp_log.h"
#include <stdbool.h>

/**
 * @brief Check if the current build target is ESP32-S3
 * 
 * Verifies whether the code is being compiled for an ESP32-S3 target
 * and logs appropriate messages based on the result.
 * 
 * @return true if the target is ESP32-S3, false otherwise
 */
bool check_build_target(void);

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
bool check_spi_config_and_warn_gpio_conflicts(void);

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
bool run_gpio_protection_checks(bool abort_on_critical_conflict);

#endif /* CONFIG_CHECK_H */