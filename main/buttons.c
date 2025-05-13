#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <nvs_flash.h>
#include <esp_log.h>
#include "sdkconfig.h"
#include "buttons.h"
#include "matrix.h"
#include "gui.h"

#define NUM_PEDALS 8
static const gpio_num_t PEDAL_PINS[NUM_PEDALS] = {
    CONFIG_PEDAL_BUTTON_1_PIN,
    CONFIG_PEDAL_BUTTON_2_PIN,
    CONFIG_PEDAL_BUTTON_3_PIN,
    CONFIG_PEDAL_BUTTON_4_PIN,
    CONFIG_PEDAL_BUTTON_5_PIN,
    CONFIG_PEDAL_BUTTON_6_PIN,
    CONFIG_PEDAL_BUTTON_7_PIN,
    CONFIG_PEDAL_BUTTON_8_PIN};

static const char *TAG = "Buttons";
static uint8_t patch[NUM_PEDALS] = {0};
static uint8_t patch_index = 0;
static bool programming = false;

void buttons_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CONFIG_PROGRAM_BUTTON_PIN) |
                        (1ULL << CONFIG_END_BUTTON_PIN) |
                        (1ULL << CONFIG_PEDAL_BUTTON_1_PIN) |
                        (1ULL << CONFIG_PEDAL_BUTTON_2_PIN) |
                        (1ULL << CONFIG_PEDAL_BUTTON_3_PIN) |
                        (1ULL << CONFIG_PEDAL_BUTTON_4_PIN) |
                        (1ULL << CONFIG_PEDAL_BUTTON_5_PIN) |
                        (1ULL << CONFIG_PEDAL_BUTTON_6_PIN) |
                        (1ULL << CONFIG_PEDAL_BUTTON_7_PIN) |
                        (1ULL << CONFIG_PEDAL_BUTTON_8_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&io_conf);

#ifdef CONFIG_ENABLE_LEDS
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << CONFIG_LED_1_PIN) |
                        (1ULL << CONFIG_LED_2_PIN) |
                        (1ULL << CONFIG_LED_3_PIN) |
                        (1ULL << CONFIG_LED_4_PIN) |
                        (1ULL << CONFIG_LED_5_PIN) |
                        (1ULL << CONFIG_LED_6_PIN) |
                        (1ULL << CONFIG_LED_7_PIN) |
                        (1ULL << CONFIG_LED_8_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&led_conf);
#endif
}

void buttons_load_patch(void)
{
    nvs_handle_t handle;
    nvs_open("storage", NVS_READONLY, &handle);
    size_t size = sizeof(patch);
    esp_err_t ret = nvs_get_blob(handle, "patch", patch, &size);
    nvs_close(handle);
    if (ret == ESP_OK)
    {
        patch_index = 0;
        while (patch_index < NUM_PEDALS && patch[patch_index] != 0)
            patch_index++;
        gui_update_chain(patch, patch_index);
    }
}

uint8_t buttons_get_patch(uint8_t *out_patch)
{
    for (int i = 0; i < patch_index; i++)
        out_patch[i] = patch[i];
    return patch_index;
}

void buttons_task(void *pvParameters)
{
    while (1)
    {
        if (!gpio_get_level(CONFIG_PROGRAM_BUTTON_PIN))
        {
            programming = true;
            patch_index = 0;
            memset(patch, 0, sizeof(patch));
            gui_set_status("Program Mode");
            gui_update_chain(patch, 0);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        if (programming)
        {
            for (int i = 0; i < NUM_PEDALS; i++)
            {
                if (!gpio_get_level(PEDAL_PINS[i]))
                {
                    patch[patch_index++] = i + 1;
                    gui_update_chain(patch, patch_index);
#ifdef CONFIG_ENABLE_LEDS
                    gpio_set_level(CONFIG_LED_1_PIN + i, 1); // Turn on LED for pedal
#endif
                    vTaskDelay(pdMS_TO_TICKS(200));
                }
            }
        }
        if (!gpio_get_level(CONFIG_END_BUTTON_PIN) && programming)
        {
            programming = false;
            nvs_handle_t handle;
            nvs_open("storage", NVS_READWRITE, &handle);
            nvs_set_blob(handle, "patch", patch, sizeof(patch));
            nvs_commit(handle);
            nvs_close(handle);
            matrix_update();
            gui_set_status(patch_index ? "Saved!" : "Bypass");
            gui_update_chain(patch, patch_index);
#ifdef CONFIG_ENABLE_LEDS
            for (int i = 0; i < NUM_PEDALS; i++)
            {
                gpio_set_level(CONFIG_LED_1_PIN + i, 0); // Turn off LEDs
            }
#endif
            vTaskDelay(pdMS_TO_TICKS(1000));
            gui_set_status("");
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}