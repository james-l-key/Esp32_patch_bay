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

static const char *TAG = "PatchBay";

void i2c_init()
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = CONFIG_I2C_SDA_PIN,
        .scl_io_num = CONFIG_I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
}

void nvs_init()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        nvs_flash_init();
    }
}

void app_main()
{
    // Initialize hardware
    nvs_init();
    i2c_init();
    matrix_init();
    buttons_init();

    // Initialize LVGL
    lv_init();

    // Configure display
    lvgl_port_display_cfg_t disp_cfg = {
        .disp_drv = NULL,
        .hor_res = 128,
        .ver_res = 64,
        .buff_size = 128 * 64 / 8, // Monochrome, 1-bit per pixel
        .buff_type = LVGL_PORT_DISP_BUF_STATIC,
    };

    // Initialize display driver
    lv_disp_drv_t *disp_drv = lv_disp_drv_init();
#ifdef CONFIG_SSD1306
    ssd1306_init(disp_drv, I2C_NUM_0, 0x3C, CONFIG_I2C_SDA_PIN, CONFIG_I2C_SCL_PIN);
#else // CONFIG_SH1106
    sh1106_init(disp_drv, I2C_NUM_0, 0x3C, CONFIG_I2C_SDA_PIN, CONFIG_I2C_SCL_PIN);
#endif
    disp_cfg.disp_drv = disp_drv;

    // Add display to LVGL
    lv_disp_t *disp = lvgl_port_add_disp(&disp_cfg);
    if (!disp)
    {
        ESP_LOGE(TAG, "LVGL display init failed");
        return;
    }

    // Initialize GUI
    gui_init();

    // Load patch and update matrix
    buttons_load_patch();
    matrix_update();

    // Start button task
    xTaskCreate(buttons_task, "buttons_task", 4096, NULL, 5, NULL);
}