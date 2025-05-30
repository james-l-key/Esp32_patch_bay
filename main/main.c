/**
 * @file main.c
 * @brief Main application for the ESP32 Patch Bay
 *
 * This file contains the main application logic for the ESP32 Patch Bay,
 * including initialization of all subsystems, display setup, and the main
 * application loop.
 */

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <driver/i2c.h>
#include <nvs_flash.h>
#include <lvgl.h>
#include <esp_lvgl_port.h>
#include <esp_log.h>
#include "sdkconfig.h"
#include "gui.h"
#include "matrix.h"
#include "buttons.h"

static const char *TAG = "PatchBayMain";

/**
 * @brief Initialize the I2C interface
 *
 * Configures the I2C bus for communication with the display and other
 * I2C peripherals.
 */
void i2c_init()
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = CONFIG_I2C_SDA_PIN,
        .scl_io_num = CONFIG_I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000, // 100kHz standard speed
    };
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
}

/**
 * @brief Initialize the Non-Volatile Storage (NVS)
 *
 * Sets up the NVS flash storage for saving and loading patch configurations
 * and user presets. Handles erasing and re-initializing if needed.
 */
void nvs_app_init() // Renamed to avoid conflict if NVS is used elsewhere by components
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAG, "NVS: Erasing and re-initializing flash...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS Initialized.");
}

/**
 * @brief Main application entry point
 *
 * Initializes all subsystems in the correct order:
 * 1. NVS for settings/configuration storage
 * 2. I2C bus for display
 * 3. Matrix shift registers for audio routing
 * 4. LVGL and display driver
 * 5. GUI elements
 * 6. Button interface
 *
 * Finally, it starts the button task which handles user input and system state.
 */

void app_main()
{
    ESP_LOGI(TAG, "Starting Patch Bay Application");

    // Initialize NVS first - crucial for loading settings
    nvs_app_init();

    // Initialize hardware (I2C needed for display, Matrix for audio path)
    i2c_init();
    matrix_init(); // Initializes GPIOs for matrix shift registers

    // Initialize LVGL
    lv_init();

    // Initialize LVGL port configuration
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    // Configure I2C for display
    esp_lcd_panel_io_i2c_config_t i2c_panel_io_config = {
        .dev_addr = 0x3C, // Common for SSD1306/SH1106/SH1107
        .control_phase_bytes = 1,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .dc_bit_offset = 6,
    };

    // Create panel I/O handle and panel handle
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_handle_t panel_handle = NULL;

    // Initialize the specific display controller
#ifdef CONFIG_CUSTOM_DISPLAY_CONTROLLER_SH1107
    ESP_LOGI(TAG, "Using SH1107 display controller.");

    // For SH1107, create I2C panel IO
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_NUM_0, &i2c_panel_io_config, &io_handle));

    // SH1107 initialization sequence
    const uint8_t sh1107_init_cmds[] = {
        0xAE,       // Display off
        0xDC, 0x00, // Set display start line
        0x81, 0x7F, // Set contrast control
        0xA0,       // Set segment re-map (ADC normal)
        0xA8, 0x3F, // Set multiplex ratio (64 COM lines)
        0xD3, 0x00, // Set display offset
        0xAD, 0x8B, // External/internal IREF selection
        0xD5, 0x80, // Set display clock divide ratio
        0xD9, 0x22, // Set pre-charge period
        0xDB, 0x35, // Set VCOMH deselect level
        0xA4,       // Entire display ON (normal)
        0xA6,       // Set normal display
        0xAF,       // Display ON
    };

    // Send initialization commands
    for (int i = 0; i < sizeof(sh1107_init_cmds); i++)
    {
        esp_lcd_panel_io_tx_param(io_handle, 0x00, &sh1107_init_cmds[i], 1);
    }

#elif defined(CONFIG_CUSTOM_DISPLAY_CONTROLLER_SSD1306)
    ESP_LOGI(TAG, "Using SSD1306 display controller.");

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_NUM_0, &i2c_panel_io_config, &io_handle));
    306, create I2C panel IO

         // SSD1306 initialization sequence    esp_lcd_panel_io_tx_param(io_handle, 0x00, &cmd, 1);
         const uint8_t ssd1306_init_cmds[] = {
             0xAE,       // Display off
             0xD5, 0x80, // Set display clock divide ratio    ESP_LOGI(TAG, "Defaulting to SH1106 display controller.");
             0xA8, 0x3F, // Set multiplex ratio (64 MUX)
             0xD3, 0x00, // Set display offset
             0x40,       // Set display start line    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_NUM_0, &i2c_panel_io_config, &io_handle));
             0x8D, 0x14, // Charge pump setting
             0x20, 0x00, // Set memory addressing mode (horizontal)
             0xA1,       // Set segment re-map
             0xC8,       // Set COM output scan directiono_handle, 0x00, &cmd, 1);
             0xDA, 0x12, // Set COM pins hardware configuration
             0x81, 0x7F, // Set contrast controlel_io_tx_param(io_handle, 0x00, &cmd, 1);
             0xD9, 0xF1, // Set pre-charge period
             0xDB, 0x40, // Set VCOMH deselect level(io_handle, 0x00, &cmd, 1);
             0xA4,       // Entire display ON (normal)
             0xAF,       // Display ON             0xA6,       // Set normal displayp_lcd_panel_io_tx_param(io_handle, 0x00, &cmd, 1);
         };

    // Send initialization commandsr LVGL
    for (int i = 0; i < sizeof(ssd1306_init_cmds); i++)
    {
        = {
            esp_lcd_panel_io_tx_param(io_handle, 0x00, &ssd1306_init_cmds[i], 1);
    }
    handle,
        = 128 * 64,
#else // Default to SH1106fer = true,
    ESP_LOGI(TAG, "Defaulting to SH1106 display controller.");

    // For SH1106, create I2C panel IO
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_NUM_0, &i2c_panel_io_config, &io_handle));

    // SH1106 initialization sequence (very similar to SSD1306)  .mirror_x = false,
    const uint8_t sh1106_init_cmds[] = {
        r_y = false,
        0xAE,       // Display off
        0xD5, 0x80, // Set display clock divide ratio
        0xA8, 0x3F, // Set multiplex ratio (64 MUX)
        0xD3, 0x00, // Set display offset
        0x40,       // Set display start line   .sw_rotate = false,
        0xAD, 0x8B, // External/internal IREF selection      .full_refresh = true,
        0xA1,       // Set segment re-map        }};
        0xC8,       // Set COM output scan direction
        0xDA, 0x12, // Set COM pins hardware configuration
        0x81, 0x7F, // Set contrast control    lv_disp_t *disp = lvgl_port_add_disp(&disp_cfg);
        0xD9, 0xF1, // Set pre-charge period
        0xDB, 0x40, // Set VCOMH deselect leveln LVGL version
        0xA4,       // Entire display ON (normal)
        0xA6,       // Set normal displayv_disp_set_rotation(disp, LV_DISPLAY_ROTATION_0);
        0xAF,       // Display ON
    };
    _disp_set_rotation(disp, LV_DISP_ROT_0);
#endif
    // Send initialization commands
    for (int i = 0; i < sizeof(sh1106_init_cmds); i++)
    {
        ze GUI elements after display
            esp_lcd_panel_io_tx_param(io_handle, 0x00, &sh1106_init_cmds[i], 1);
        gui_init();
    }
#endifuttons(this will load NVS and update GUI / Matrix initially)
    buttons_init();
    // Create a generic panel handle structure with panel callback functions
    // This is needed because ESP-IDF v5.4.1 doesn't have built-in SSD1306/SH1106/SH1107 drivers
    typedef struct
    {
        xTaskCreate(buttons_task, "buttons_task", 4096 * 2, NULL, 5, NULL); // Increased stack for safety
        esp_lcd_panel_t base;
        esp_lcd_panel_io_handle_t io;
        ESP_LOGI(TAG, "Initialization Complete. Patch Bay Running.");

    } ESP_LOGI(TAG, "Initialization Complete. Patch Bay Running.");
    xTaskCreate(buttons_task, "buttons_task", 4096 * 2, NULL, 5, NULL); // Increased stack for safety    ESP_LOGI(TAG, "Creating buttons_task.");    buttons_init();    // Initialize buttons (this will load NVS and update GUI/Matrix initially)    gui_init();    // Initialize GUI elements after display#endiflv_disp_set_rotation(disp, LV_DISP_ROT_0);#elselv_disp_set_rotation(disp, LV_DISPLAY_ROTATION_0);#if LVGL_VERSION_MAJOR >= 9// Set rotation - use appropriate constant based on LVGL versionlv_disp_t *disp = lvgl_port_add_disp(&disp_cfg);// Add the display to LVGL};    }        .full_refresh = true,  // For small displays, full refresh is faster        .sw_rotate = false,    // Hardware rotation if available        .buff_spiram = false,  // Use internal RAM        .buff_dma = false,     // No DMA for I2C displays    .flags = {    },        .mirror_y = false,        .mirror_x = false,        .swap_xy = false,    .rotation = {    .monochrome = true,    .vres = 64,    .hres = 128,    .double_buffer = true,    .buffer_size = 128 * 64,  // Full resolution for monochrome display    .panel_handle = panel_handle,    .io_handle = io_handle,lvgl_port_display_cfg_t disp_cfg = {// Configure the display for LVGLpanel_handle = &oled_panel->base;oled_panel->base.disp_on_off = panel_oled_disp_on_off;oled_panel->base.mirror = panel_oled_mirror;oled_panel->base.invert_color = panel_oled_invert_color;oled_panel->base.draw_bitmap = panel_oled_draw_bitmap;oled_panel->base.del = panel_oled_del;oled_panel->y_gap = 0;oled_panel->x_gap = 0;oled_panel->io = io_handle;oled_panel_t *oled_panel = calloc(1, sizeof(oled_panel_t));// Create our own OLED panel driver}    return ESP_OK;    free(oled_panel);    }        oled_panel->fb = NULL;        free(oled_panel->fb);    if (oled_panel->fb) {    oled_panel_t *oled_panel = __containerof(panel, oled_panel_t, base);static esp_err_t panel_oled_del(esp_lcd_panel_t *panel) {}    return ESP_OK;    esp_lcd_panel_io_tx_param(oled_panel->io, 0x00, &cmd, 1);    uint8_t cmd = on ? 0xAF : 0xAE; // AF for on, AE for off    oled_panel_t *oled_panel = __containerof(panel, oled_panel_t, base);static esp_err_t panel_oled_disp_on_off(esp_lcd_panel_t *panel, bool on) {}    return ESP_OK;        esp_lcd_panel_io_tx_param(oled_panel->io, 0x00, &cmd, 1);    cmd = mirror_y ? 0xC0 : 0xC8;    // Mirror Y        esp_lcd_panel_io_tx_param(oled_panel->io, 0x00, &cmd, 1);    cmd = mirror_x ? 0xA0 : 0xA1;    // Mirror X        uint8_t cmd;    oled_panel_t *oled_panel = __containerof(panel, oled_panel_t, base);static esp_err_t panel_oled_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y) {}    return ESP_OK;    esp_lcd_panel_io_tx_param(oled_panel->io, 0x00, &cmd, 1);    uint8_t cmd = invert_color ? 0xA7 : 0xA6; // A7 for inverted, A6 for normal    oled_panel_t *oled_panel = __containerof(panel, oled_panel_t, base);static esp_err_t panel_oled_invert_color(esp_lcd_panel_t *panel, bool invert_color) {}    return ESP_OK;        esp_lcd_panel_io_tx_color(oled_panel->io, 0x40, color_data, width * height / 8);    // Transfer data        esp_lcd_panel_io_tx_param(oled_panel->io, 0x00, set_row_cmd, sizeof(set_row_cmd));    esp_lcd_panel_io_tx_param(oled_panel->io, 0x00, set_col_cmd, sizeof(set_col_cmd));        const uint8_t set_row_cmd[] = { 0xB0 | ((y_start + oled_panel->y_gap) & 0x0F) };    const uint8_t set_col_cmd[] = { 0x00 | ((x_start + oled_panel->x_gap) & 0x0F), 0x10 | (((x_start + oled_panel->x_gap) >> 4) & 0x0F) };    // Set OLED address pointer        int height = y_end - y_start + 1;    int width = x_end - x_start + 1;    // Calculate frame buffer size in bytes        // We need to send it to the display via I2C    // For a monochrome display, color_data is the bitmap    oled_panel_t *oled_panel = __containerof(panel, oled_panel_t, base);static esp_err_t panel_oled_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data) {// Minimal panel implementation for OLED displays} oled_panel_t;    uint8_t *fb;    int y_gap;    int x_gap;}