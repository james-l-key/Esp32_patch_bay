/**
 * @file gui.c
 * @brief GUI implementation for the ESP32 Patch Bay
 *
 * This file implements the graphical user interface for the patch bay system,
 * displaying the current effects chain, preset information, and system status
 * messages using LVGL.
 */

#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h> // For variadic functions
#include <esp_log.h>
#include <esp_task_wdt.h>
#include "gui.h"

static const char *TAG = "GUI";
static lv_obj_t *chain_label;         /**< LVGL label for displaying the effects chain */
static lv_obj_t *status_label;        /**< LVGL label for displaying status messages */
static bool display_available = true; /**< Flag indicating if display is working */

#define CHAIN_BUFFER_SIZE 96 // Increased buffer size for prefixes and longer chains
#define STATUS_BUFFER_SIZE 64

/**
 * @brief Initialize the GUI subsystem with watchdog protection
 *
 * Sets up the LVGL UI components and prepares the display for showing
 * the patch bay interface. Includes protection against watchdog timeouts.
 */
void gui_init(void)
{
    ESP_LOGI(TAG, "Starting GUI initialization with deferred rendering");

    lv_obj_t *scr = lv_scr_act();
    if (!scr)
    {
        ESP_LOGE(TAG, "Failed to get active screen");
        display_available = false;
        return;
    }

    ESP_LOGI(TAG, "Screen acquired, disabling auto-refresh during object creation");

    // Disable automatic refresh during object creation to prevent I2C timeouts
    lv_disp_t *disp = lv_disp_get_default();
    if (disp)
    {
        // Temporarily disable screen refresh to prevent I2C operations during object creation
        lv_disp_enable_invalidation(disp, false);
        ESP_LOGI(TAG, "Screen invalidation disabled for object creation");
    }

    // Create objects without triggering immediate screen updates
    ESP_LOGI(TAG, "Creating chain label");
    chain_label = lv_label_create(scr);
    if (!chain_label)
    {
        ESP_LOGE(TAG, "Failed to create chain label");
        display_available = false;
        // Re-enable invalidation before returning
        if (disp)
            lv_disp_enable_invalidation(disp, true);
        return;
    }

    ESP_LOGI(TAG, "Creating status label");
    status_label = lv_label_create(scr);
    if (!status_label)
    {
        ESP_LOGE(TAG, "Failed to create status label");
        // Re-enable invalidation before returning
        if (disp)
            lv_disp_enable_invalidation(disp, true);
        return;
    }

    ESP_LOGI(TAG, "Setting label properties (still no screen updates)");

    // Set all properties while invalidation is disabled
    lv_label_set_text(chain_label, "Patch Bay");
    lv_obj_align(chain_label, LV_ALIGN_TOP_MID, 0, 10);
    lv_label_set_long_mode(chain_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(chain_label, 120);

    lv_label_set_text(status_label, "Ready");
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_label_set_long_mode(status_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(status_label, 126);

    ESP_LOGI(TAG, "All objects created, re-enabling screen invalidation"); // Re-enable invalidation - but DON'T manually invalidate to avoid I2C timeout
    if (disp)
    {
        lv_disp_enable_invalidation(disp, true);
        ESP_LOGI(TAG, "Screen invalidation re-enabled");

        // Don't trigger any manual refresh - let LVGL handle updates on its timer
        // The objects will be refreshed automatically on the next LVGL timer tick
        ESP_LOGI(TAG, "Objects will be refreshed automatically on next LVGL timer cycle");
    }

    ESP_LOGI(TAG, "GUI initialized successfully with lazy refresh approach");
}

/**
 * @brief Initialize a fallback GUI without display
 *
 * This version doesn't rely on an actual display and provides dummy objects
 * that won't crash when no display is available.
 */
void gui_init_fallback(void)
{
    ESP_LOGI(TAG, "Initializing fallback GUI (no display)");

    // Set flag to indicate no display
    display_available = false;

    // Set labels to NULL to indicate they're not available
    chain_label = NULL;
    status_label = NULL;

    ESP_LOGW(TAG, "Running in headless mode - no GUI available");
}

/**
 * @brief Update the chain display in the GUI with watchdog protection
 *
 * Formats and displays the current effects chain configuration with
 * an indication of whether it's a custom chain or a loaded preset.
 * Includes protection against watchdog timeouts.
 *
 * @param patch Array containing the current patch configuration
 * @param len Length of the patch array
 * @param loaded_slot_index Index of the loaded preset (-1 for live/custom, 0-7 for presets)
 */
void gui_update_chain(const uint8_t *patch, uint8_t len, int8_t loaded_slot_index)
{
    // Skip if display is not available
    if (!display_available || !chain_label)
    {
        ESP_LOGD(TAG, "Chain update skipped (no display)");
        return;
    }

    // Reset watchdog if accessible
    TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
    if (esp_task_wdt_status(current_task) == ESP_OK)
    {
        esp_task_wdt_reset();
    }

    char buf[CHAIN_BUFFER_SIZE];
    char temp_chain_buf[CHAIN_BUFFER_SIZE - 20] = {0}; // Buffer for the chain part

    // Create a simpler chain description to prevent long operations
    if (len == 0)
    {
        strcat(temp_chain_buf, "Bypass");
    }
    else if (len > 4)
    {
        // Simplify very long chains to prevent timeout
        snprintf(temp_chain_buf, sizeof(temp_chain_buf), "%d->%d->...->%d",
                 patch[0], patch[1], patch[len - 1]);
    }
    else
    {
        for (int i = 0; i < len; i++)
        {
            char tmp_num[4]; // For pedal number
            snprintf(tmp_num, sizeof(tmp_num), "%d", patch[i]);
            strcat(temp_chain_buf, tmp_num);
            if (i < len - 1)
            {
                strcat(temp_chain_buf, "->");
            }
        }
    }

    if (loaded_slot_index != -1)
    { // Preset 0-7
        snprintf(buf, sizeof(buf), "[P%d] %s", loaded_slot_index + 1, temp_chain_buf);
    }
    else
    { // Live/custom config
        snprintf(buf, sizeof(buf), "Live: %s", temp_chain_buf);
    }

    // Temporarily disable invalidation during text update to prevent I2C timeout
    lv_disp_t *disp = lv_disp_get_default();
    bool invalidation_was_enabled = true;
    if (disp)
    {
        invalidation_was_enabled = lv_disp_is_invalidation_enabled(disp);
        if (invalidation_was_enabled)
        {
            lv_disp_enable_invalidation(disp, false);
        }
    }

    // Apply the text update without triggering immediate invalidation
    lv_label_set_text(chain_label, buf);
    lv_label_set_long_mode(chain_label, LV_LABEL_LONG_CLIP);

    // Re-enable invalidation if it was previously enabled
    if (disp && invalidation_was_enabled)
    {
        lv_disp_enable_invalidation(disp, true);
    }

    ESP_LOGD(TAG, "Chain updated: %s", buf);
}

/**
 * @brief Set or update the status message in the GUI with watchdog protection
 *
 * Formats and displays a status message in the designated area of the display.
 * Supports variable arguments for formatting the message.
 * Includes protection against watchdog timeouts.
 *
 * @param status_fmt Format string for the status message
 * @param ... Variable arguments for formatting the status message
 */
void gui_set_status(const char *status_fmt, ...)
{
    // Skip if display is not available
    if (!display_available || !status_label)
    {
        ESP_LOGD(TAG, "Status update skipped (no display)");
        return;
    }

    // Reset watchdog if accessible
    TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
    if (esp_task_wdt_status(current_task) == ESP_OK)
    {
        esp_task_wdt_reset();
    }

    // Keep messages short for faster processing
    char buf[STATUS_BUFFER_SIZE];

    // Limit status message length to prevent long processing
    const char *truncated_fmt = status_fmt;
    if (strlen(status_fmt) > 40)
    {
        ESP_LOGW(TAG, "Status message too long, truncating");
        static const char fallback_msg[] = "Status updated...";
        truncated_fmt = fallback_msg;
    }

    va_list args;
    va_start(args, status_fmt);
    vsnprintf(buf, sizeof(buf), truncated_fmt, args);
    va_end(args);

    // Temporarily disable invalidation during text update to prevent I2C timeout
    lv_disp_t *disp = lv_disp_get_default();
    bool invalidation_was_enabled = true;
    if (disp)
    {
        invalidation_was_enabled = lv_disp_is_invalidation_enabled(disp);
        if (invalidation_was_enabled)
        {
            lv_disp_enable_invalidation(disp, false);
        }
    }

    // Apply the text update without triggering immediate invalidation
    lv_label_set_text(status_label, buf);
    lv_label_set_long_mode(status_label, LV_LABEL_LONG_CLIP);

    // Re-enable invalidation if it was previously enabled
    if (disp && invalidation_was_enabled)
    {
        lv_disp_enable_invalidation(disp, true);
    }
}

/**
 * @brief Safely trigger a manual display refresh
 * 
 * This function provides a controlled way to update the display that's safe
 * against I2C timeouts by using partial refresh and watchdog protection.
 */
void gui_force_refresh(void)
{
    if (!display_available)
    {
        ESP_LOGD(TAG, "Force refresh skipped (no display)");
        return;
    }

    ESP_LOGD(TAG, "Triggering controlled display refresh with watchdog protection");
    
    // Reset watchdog before potentially slow operation
    esp_task_wdt_reset();
    
    // Use a gentler invalidation approach - only invalidate specific objects
    if (chain_label) {
        lv_obj_invalidate(chain_label);
    }
    if (status_label) {
        lv_obj_invalidate(status_label);
    }
    
    // Don't force immediate refresh - let LVGL handle it on its timer
    ESP_LOGD(TAG, "Objects invalidated for next LVGL refresh cycle");
}