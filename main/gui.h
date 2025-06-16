/**
 * @file gui.h
 * @brief GUI interface for the ESP32 Patch Bay
 * 
 * This file provides the interface for managing the graphical user interface
 * of the patch bay system, displaying the current effects chain, preset information,
 * and system status messages.
 */

#ifndef GUI_H
#define GUI_H

#include <stdint.h>

#define NUM_PRESETS 8 /**< Number of user-configurable presets */

/**
 * @brief Initialize the GUI subsystem
 * 
 * Sets up the LVGL UI components and prepares the display for showing
 * the patch bay interface.
 */
void gui_init(void);

/**
 * @brief Initialize a fallback GUI when display fails
 * 
 * This creates a minimal GUI setup that won't crash when the display
 * initialization fails, allowing the rest of the system to function.
 */
void gui_init_fallback(void);

/**
 * @brief Safely trigger a manual display refresh
 * 
 * Forces a controlled display update to ensure GUI content is visible
 * without causing I2C timeouts.
 */
void gui_force_refresh(void);

/**
 * @brief Update the chain display in the GUI
 * 
 * @param patch Array containing the current patch configuration
 * @param len Length of the patch array
 * @param loaded_slot_index Index of the loaded preset (-1 for live/custom, 0-7 for presets)
 */
void gui_update_chain(const uint8_t *patch, uint8_t len, int8_t loaded_slot_index);

/**
 * @brief Set or update the status message in the GUI
 * 
 * @param status_fmt Format string for the status message
 * @param ... Variable arguments for formatting the status message
 */
void gui_set_status(const char *status_fmt, ...); // Variadic for easier formatting

#endif