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
#include <driver/i2c_master.h>
#include <nvs_flash.h>
#include <lvgl.h>
#include <esp_lvgl_port.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_log.h>
#include <stdlib.h>
#include <esp_task_wdt.h> // For watchdog functions

#if CONFIG_EXAMPLE_LCD_CONTROLLER_SH1107
#include "esp_lcd_sh1107.h"
#else
#include "esp_lcd_panel_vendor.h"
#endif

#include "sdkconfig.h"
#include "config_check.h"
#include "gui.h"
#include "matrix.h"
#include "buttons.h"
#include "led.h"

//set pwm_duty_cycle to 100% by default
extern uint8_t pwm_duty_cycle = 100; // 0-100%, default full brightness

static const char *TAG = "PatchBayMain";

#define I2C_BUS_PORT 0

// LCD configuration - updated to match working example
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ (400 * 1000)
#define EXAMPLE_PIN_NUM_RST -1
#define EXAMPLE_I2C_HW_ADDR 0x3D // Default OLED I2C address, most displays use 0x3C or 0x3D

// The pixel number in horizontal and vertical
#if CONFIG_EXAMPLE_LCD_CONTROLLER_SSD1306
#define EXAMPLE_LCD_H_RES 128
#define EXAMPLE_LCD_V_RES CONFIG_EXAMPLE_SSD1306_HEIGHT
#elif CONFIG_EXAMPLE_LCD_CONTROLLER_SH1107
#define EXAMPLE_LCD_H_RES 64
#define EXAMPLE_LCD_V_RES 128
#else
// Default to SSD1306 128x64
#define EXAMPLE_LCD_H_RES 128
#define EXAMPLE_LCD_V_RES 64
#endif

// Bit number used to represent command and parameter
#define EXAMPLE_LCD_CMD_BITS 8
#define EXAMPLE_LCD_PARAM_BITS 8

// Forward declarations for display initialization
extern void example_lvgl_demo_ui(lv_disp_t *disp);
static void init_display_and_lvgl(void);

/**
 * @brief Initialize the Non-Volatile Storage (NVS)
 *
 * Sets up the NVS flash storage for saving and loading patch configurations
 * and user presets. Handles erasing and re-initializing if needed.
 */

// Global I2C bus handle - similar to working example
static i2c_master_bus_handle_t i2c_bus = NULL;

void i2c_init(void)
{
    ESP_LOGI(TAG, "Initialize I2C bus");

    // Use modern i2c_master API like in working example
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .i2c_port = I2C_BUS_PORT,
        .sda_io_num = CONFIG_I2C_SDA_PIN,
        .scl_io_num = CONFIG_I2C_SCL_PIN,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));

    ESP_LOGI(TAG, "I2C bus initialized successfully");
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
 * @brief Initialize display and LVGL - adapted from working example
 */
static void init_display_and_lvgl(void)
{
    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = EXAMPLE_I2C_HW_ADDR, // Use the defined OLED I2C address
        .scl_speed_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
        .control_phase_bytes = 1,               // According to SSD1306 datasheet
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,   // According to SSD1306 datasheet
        .lcd_param_bits = EXAMPLE_LCD_CMD_BITS, // According to SSD1306 datasheet
#if CONFIG_EXAMPLE_LCD_CONTROLLER_SSD1306
        .dc_bit_offset = 6, // According to SSD1306 datasheet
#elif CONFIG_EXAMPLE_LCD_CONTROLLER_SH1107
        .dc_bit_offset = 0, // According to SH1107 datasheet
        .flags =
            {
                .disable_control_phase = 1,
            }
#endif
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus, &io_config, &io_handle));

    ESP_LOGI(TAG, "Install LCD panel driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,
        .reset_gpio_num = EXAMPLE_PIN_NUM_RST,
    };
#if CONFIG_EXAMPLE_LCD_CONTROLLER_SSD1306
    esp_lcd_panel_ssd1306_config_t ssd1306_config = {
        .height = EXAMPLE_LCD_V_RES,
    };
    panel_config.vendor_config = &ssd1306_config;
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));
#elif CONFIG_EXAMPLE_LCD_CONTROLLER_SH1107
    ESP_ERROR_CHECK(esp_lcd_new_panel_sh1107(io_handle, &panel_config, &panel_handle));
#else
    // Default to SSD1306 if no specific display is configured
    ESP_LOGI(TAG, "No specific display configured, defaulting to SSD1306");
    esp_lcd_panel_ssd1306_config_t ssd1306_config = {
        .height = EXAMPLE_LCD_V_RES,
    };
    panel_config.vendor_config = &ssd1306_config;
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));
#endif

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

#if CONFIG_EXAMPLE_LCD_CONTROLLER_SH1107
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
#endif

    ESP_LOGI(TAG, "Initialize LVGL");
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvgl_cfg);

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES,
        .double_buffer = true,
        .hres = EXAMPLE_LCD_H_RES,
        .vres = EXAMPLE_LCD_V_RES,
        .monochrome = true,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        }};
    lv_disp_t *disp = lvgl_port_add_disp(&disp_cfg);

    /* Rotation of the screen */
    lv_disp_set_rotation(disp, LV_DISP_ROTATION_0);

    ESP_LOGI(TAG, "Display LVGL initialization complete");

    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (lvgl_port_lock(0))
    {
        // Initialize GUI instead of demo
        gui_init();
        // Release the mutex
        lvgl_port_unlock();
    }
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
    led_init(); // Initialize LEDs
    ESP_LOGI(TAG, "Running GPIO protection checks.");
    run_gpio_protection_checks(true);

    // Initialize NVS first - crucial for loading settings
    nvs_app_init();

    // Initialize hardware (I2C needed for display, Matrix for audio path)
    i2c_init();
    matrix_init(); // Initializes GPIOs for matrix shift registers

    // Initialize display and LVGL - using the working example method
    init_display_and_lvgl();

    // Initialize buttons (this will load NVS and update GUI/Matrix initially)
    buttons_init();

    ESP_LOGI(TAG, "Creating buttons_task.");
    xTaskCreate(buttons_task, "buttons_task", 4096 * 2, NULL, 5, NULL); // Increased stack for safety

    ESP_LOGI(TAG, "Initialization Complete. Patch Bay Running.");
}