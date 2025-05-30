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
#include "lv_conf.h"
#include "font/lv_font.h"
#include "gui.h"

static lv_obj_t *chain_label;  /**< LVGL label for displaying the effects chain */
static lv_obj_t *status_label; /**< LVGL label for displaying status messages */

#define CHAIN_BUFFER_SIZE 96 // Increased buffer size for prefixes and longer chains
#define STATUS_BUFFER_SIZE 64

/**
 * @brief Initialize the GUI subsystem
 *
 * Sets up the LVGL UI components and prepares the display for showing
 * the patch bay interface.
 */
void gui_init(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

    // Chain label (top)
    chain_label = lv_label_create(scr);
    lv_label_set_text(chain_label, "Patch Bay");
    lv_obj_set_style_text_color(chain_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(chain_label, &lv_font_montserrat_14, 0);
    lv_obj_align(chain_label, LV_ALIGN_TOP_MID, 0, 10);
    lv_label_set_long_mode(chain_label, LV_LABEL_LONG_SCROLL_CIRCULAR); // Allow scrolling if text is too long
    lv_obj_set_width(chain_label, 120);                                 // Set width to enable scrolling, adjust as needed

    // Status label (bottom)
    status_label = lv_label_create(scr);
    lv_label_set_text(status_label, "");
    lv_obj_set_style_text_color(status_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, 0);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_label_set_long_mode(status_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(status_label, 126);
}

/**
 * @brief Update the chain display in the GUI
 *
 * Formats and displays the current effects chain configuration with
 * an indication of whether it's a custom chain or a loaded preset.
 *
 * @param patch Array containing the current patch configuration
 * @param len Length of the patch array
 * @param loaded_slot_index Index of the loaded preset (-1 for live/custom, 0-7 for presets)
 */
void gui_update_chain(const uint8_t *patch, uint8_t len, int8_t loaded_slot_index)
{
    char buf[CHAIN_BUFFER_SIZE];
    char temp_chain_buf[CHAIN_BUFFER_SIZE - 20] = {0}; // Buffer for the chain part

    if (len == 0)
    {
        strcat(temp_chain_buf, "Bypass");
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
    {   // Preset 0-7
        snprintf(buf, sizeof(buf), "[P%d] %s", loaded_slot_index + 1, temp_chain_buf);
    }
    else
    {   // Live/custom config
        snprintf(buf, sizeof(buf), "Live: %s", temp_chain_buf);
    }
    lv_label_set_text(chain_label, buf);
}

/**
 * @brief Set or update the status message in the GUI
 *
 * Formats and displays a status message in the designated area of the display.
 * Supports variable arguments for formatting the message.
 *
 * @param status_fmt Format string for the status message
 * @param ... Variable arguments for formatting the status message
 */
void gui_set_status(const char *status_fmt, ...)
{
    char buf[STATUS_BUFFER_SIZE];
    va_list args;
    va_start(args, status_fmt);
    vsnprintf(buf, sizeof(buf), status_fmt, args);
    va_end(args);
    lv_label_set_text(status_label, buf);
}