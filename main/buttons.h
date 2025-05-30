/**
 * @file buttons.h
 * @brief Buttons interface for the ESP32 Patch Bay
 * 
 * This file provides the interface for handling button inputs and managing the
 * patch bay system modes, presets, and live configuration.
 *
 * @note The patch bay supports up to 8 pedals and 8 presets
 */

#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>
#include <stdbool.h>

#define NUM_PEDALS_MAX 8 // Max number of pedals physical interface supports
#define NUM_PRESETS 8    // Number of storable user presets

/**
 * @brief System operation modes for the patch bay
 */
typedef enum
{
    MODE_LIVE,               /**< Normal operation, current live chain is active */
    MODE_PROGRAM_CHAIN,      /**< Programming the live chain */
    MODE_RECALL_SLOT_SELECT, /**< PRESET_BUTTON short-pressed, waiting for pedal button (1-8) to load */
    MODE_SAVE_SLOT_SELECT    /**< PRESET_BUTTON long-pressed, waiting for pedal button (1-8) to save */
} patch_bay_system_mode_t;

/**
 * @brief Initialize the buttons subsystem
 * 
 * Configures GPIO pins for buttons, sets up internal state, and loads the last
 * saved configuration from NVS.
 */
void buttons_init(void);

/**
 * @brief Main task for handling button presses and system state
 * 
 * This task continuously monitors button inputs, handles debouncing, detects
 * short and long presses, and manages the system state machine. It controls
 * the effects chain configuration based on user input.
 * 
 * @param pvParameters FreeRTOS task parameters (unused)
 */
void buttons_task(void *pvParameters);

/**
 * @brief Provides the current patch configuration to the matrix driver
 * 
 * @param[out] patch_buffer Buffer to receive the current patch data
 * @param[out] length_buffer Pointer to receive the length of the current patch
 */
void buttons_get_current_patch_for_matrix(uint8_t *patch_buffer, uint8_t *length_buffer);

#endif