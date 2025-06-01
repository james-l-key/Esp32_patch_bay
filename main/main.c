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
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_log.h>
#include <stdlib.h>
#include "sdkconfig.h"
#include "config_check.h"
#include "gui.h"
#include "matrix.h"
#include "buttons.h"

static const char *TAG = "PatchBayMain";

// Define our own panel interface structure for the OLED operations
typedef struct
{
    esp_err_t (*del)(void *panel);
    esp_err_t (*reset)(void *panel);
    esp_err_t (*init)(void *panel);
    esp_err_t (*draw_bitmap)(void *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data);
    esp_err_t (*invert_color)(void *panel, bool invert_color);
    esp_err_t (*mirror)(void *panel, bool mirror_x, bool mirror_y);
    esp_err_t (*swap_xy)(void *panel, bool swap_xy);
    esp_err_t (*set_gap)(void *panel, int x_gap, int y_gap);
    esp_err_t (*disp_on_off)(void *panel, bool on_off);
} lcd_panel_ops_t;

// Forward declaration for our custom OLED panel structure
typedef struct
{
    lcd_panel_ops_t ops;          // Panel operations interface
    esp_lcd_panel_io_handle_t io; // I2C communication handle
    int x_gap;                    // X offset
    int y_gap;                    // Y offset
    uint8_t *fb;                  // Frame buffer (if needed)
} oled_panel_t;

// Function prototypes
static esp_err_t panel_oled_draw_bitmap(void *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data);
static esp_err_t panel_oled_invert_color(void *panel, bool invert_color);
static esp_err_t panel_oled_mirror(void *panel, bool mirror_x, bool mirror_y);
static esp_err_t panel_oled_disp_on_off(void *panel, bool on);
static esp_err_t panel_oled_del(void *panel);

/**
 * @brief Initialize the I2C interface
 *
 * Configures the I2C bus for communication with the display and other
 * I2C peripherals.
 */
void i2c_init(void)
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
void nvs_app_init(void)
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
 * @brief OLED panel driver structure
 *
 * This structure provides a custom implementation for OLED displays since
 * ESP-IDF v5.4.1 doesn't have built-in SSD1306/SH1106/SH1107 drivers.
 */
// Structure already defined above

/**
 * @brief Draw bitmap to OLED display
 */
static esp_err_t panel_oled_draw_bitmap(void *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data)
{
    oled_panel_t *oled_panel = (oled_panel_t *)panel;

    // Calculate frame buffer size in bytes
    int width = x_end - x_start + 1;
    int height = y_end - y_start + 1;

    // Set OLED address pointer
    const uint8_t set_col_cmd[] = {0x00 | ((x_start + oled_panel->x_gap) & 0x0F), 0x10 | (((x_start + oled_panel->x_gap) >> 4) & 0x0F)};
    const uint8_t set_row_cmd[] = {0xB0 | ((y_start + oled_panel->y_gap) & 0x0F)};

    esp_lcd_panel_io_tx_param(oled_panel->io, 0x00, set_col_cmd, sizeof(set_col_cmd));
    esp_lcd_panel_io_tx_param(oled_panel->io, 0x00, set_row_cmd, sizeof(set_row_cmd));

    // Transfer data
    esp_lcd_panel_io_tx_color(oled_panel->io, 0x40, color_data, width * height / 8);

    return ESP_OK;
}

/**
 * @brief Invert OLED display colors
 */
static esp_err_t panel_oled_invert_color(void *panel, bool invert_color)
{
    oled_panel_t *oled_panel = (oled_panel_t *)panel;
    uint8_t cmd = invert_color ? 0xA7 : 0xA6; // A7 for inverted, A6 for normal
    esp_lcd_panel_io_tx_param(oled_panel->io, 0x00, &cmd, 1);
    return ESP_OK;
}

/**
 * @brief Mirror OLED display
 */
static esp_err_t panel_oled_mirror(void *panel, bool mirror_x, bool mirror_y)
{
    oled_panel_t *oled_panel = (oled_panel_t *)panel;
    uint8_t cmd;

    // Mirror X
    cmd = mirror_x ? 0xA0 : 0xA1;
    esp_lcd_panel_io_tx_param(oled_panel->io, 0x00, &cmd, 1);

    // Mirror Y
    cmd = mirror_y ? 0xC0 : 0xC8;
    esp_lcd_panel_io_tx_param(oled_panel->io, 0x00, &cmd, 1);

    return ESP_OK;
}

/**
 * @brief Turn OLED display on or off
 */
static esp_err_t panel_oled_disp_on_off(void *panel, bool on)
{
    oled_panel_t *oled_panel = (oled_panel_t *)panel;
    uint8_t cmd = on ? 0xAF : 0xAE; // AF for on, AE for off
    esp_lcd_panel_io_tx_param(oled_panel->io, 0x00, &cmd, 1);
    return ESP_OK;
}

/**
 * @brief Delete OLED panel and free resources
 */
static esp_err_t panel_oled_del(void *panel)
{
    oled_panel_t *oled_panel = (oled_panel_t *)panel;
    if (oled_panel->fb)
    {
        free(oled_panel->fb);
        oled_panel->fb = NULL;
    }
    free(oled_panel);
    return ESP_OK;
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
void app_main(void)
{
    ESP_LOGI(TAG, "Starting Patch Bay Application");

    run_gpio_protection_checks(true);

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

    // Create our own OLED panel driver
    oled_panel_t *oled_panel = calloc(1, sizeof(oled_panel_t));
    if (!oled_panel)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for OLED panel");
        return;
    }

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

    // For SSD1306, create I2C panel IO
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_NUM_0, &i2c_panel_io_config, &io_handle));

    // SSD1306 initialization sequence
    const uint8_t ssd1306_init_cmds[] = {
        0xAE,       // Display off
        0xD5, 0x80, // Set display clock divide ratio
        0xA8, 0x3F, // Set multiplex ratio (64 MUX)
        0xD3, 0x00, // Set display offset
        0x40,       // Set display start line
        0x8D, 0x14, // Charge pump setting
        0x20, 0x00, // Set memory addressing mode (horizontal)
        0xA1,       // Set segment re-map
        0xC8,       // Set COM output scan direction
        0xDA, 0x12, // Set COM pins hardware configuration
        0x81, 0x7F, // Set contrast control
        0xD9, 0xF1, // Set pre-charge period
        0xDB, 0x40, // Set VCOMH deselect level
        0xA4,       // Entire display ON (normal)
        0xA6,       // Set normal display
        0xAF,       // Display ON
    };

    // Send initialization commands
    for (int i = 0; i < sizeof(ssd1306_init_cmds); i++)
    {
        esp_lcd_panel_io_tx_param(io_handle, 0x00, &ssd1306_init_cmds[i], 1);
    }

#else  // Default to SH1106
    ESP_LOGI(TAG, "Defaulting to SH1106 display controller.");

    // For SH1106, create I2C panel IO
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_NUM_0, &i2c_panel_io_config, &io_handle));

    // SH1106 initialization sequence (very similar to SSD1306)
    const uint8_t sh1106_init_cmds[] = {
        0xAE,       // Display off
        0xD5, 0x80, // Set display clock divide ratio
        0xA8, 0x3F, // Set multiplex ratio (64 MUX)
        0xD3, 0x00, // Set display offset
        0x40,       // Set display start line
        0xAD, 0x8B, // External/internal IREF selection
        0xA1,       // Set segment re-map
        0xC8,       // Set COM output scan direction
        0xDA, 0x12, // Set COM pins hardware configuration
        0x81, 0x7F, // Set contrast control
        0xD9, 0xF1, // Set pre-charge period
        0xDB, 0x40, // Set VCOMH deselect level
        0xA4,       // Entire display ON (normal)
        0xA6,       // Set normal display
        0xAF,       // Display ON
    };

    // Send initialization commands
    for (int i = 0; i < sizeof(sh1106_init_cmds); i++)
    {
        esp_lcd_panel_io_tx_param(io_handle, 0x00, &sh1106_init_cmds[i], 1);
    }
#endif // Setup panel callbacks
    oled_panel->io = io_handle;
    oled_panel->x_gap = 0;
    oled_panel->y_gap = 0;

    // Setup panel operations
    lcd_panel_ops_t *panel_ops = &oled_panel->ops;
    panel_ops->del = panel_oled_del;
    panel_ops->draw_bitmap = panel_oled_draw_bitmap;
    panel_ops->invert_color = panel_oled_invert_color;
    panel_ops->mirror = panel_oled_mirror;
    panel_ops->disp_on_off = panel_oled_disp_on_off;
    panel_handle = (esp_lcd_panel_handle_t)oled_panel;

    // Configure the display for LVGL
    lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = 128 * 64, // Full resolution for monochrome display
        .double_buffer = true,
        .hres = 128,
        .vres = 64,
        .monochrome = true,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = false,    // No DMA for I2C displays
            .buff_spiram = false, // Use internal RAM
            .sw_rotate = false,   // Hardware rotation if available
            .full_refresh = true, // For small displays, full refresh is faster
        },
    };

    // Add the display to LVGL
    lv_disp_t *disp = lvgl_port_add_disp(&disp_cfg);

    // Set rotation - use appropriate constant based on LVGL version
#if LVGL_VERSION_MAJOR >= 9
    lv_disp_set_rotation(disp, LV_DISPLAY_ROTATION_0);
#else
    lv_disp_set_rotation(disp, LV_DISP_ROT_0);
#endif

    // Initialize GUI elements after display
    gui_init();

    // Initialize buttons (this will load NVS and update GUI/Matrix initially)
    buttons_init();

    ESP_LOGI(TAG, "Creating buttons_task.");
    xTaskCreate(buttons_task, "buttons_task", 4096 * 2, NULL, 5, NULL); // Increased stack for safety

    ESP_LOGI(TAG, "Initialization Complete. Patch Bay Running.");
}