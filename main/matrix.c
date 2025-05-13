#include <driver/gpio.h>
#include "sdkconfig.h"
#include "matrix.h"
#include "buttons.h"

static void shift_out(uint8_t *data, size_t len) {
    gpio_set_level(CONFIG_SR_LATCH_PIN, 0);
    for (int i = 0; i < len; i++) {
        for (int j = 7; j >= 0; j--) {
            gpio_set_level(CONFIG_SR_DATA_PIN, (data[i] >> j) & 1);
            gpio_set_level(CONFIG_SR_CLOCK_PIN, 1);
            gpio_set_level(CONFIG_SR_CLOCK_PIN, 0);
        }
    }
    gpio_set_level(CONFIG_SR_LATCH_PIN, 1);
}

void matrix_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CONFIG_SR_DATA_PIN) |
                        (1ULL << CONFIG_SR_CLOCK_PIN) |
                        (1ULL << CONFIG_SR_LATCH_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&io_conf);
}

void matrix_update(void) {
    uint8_t patch[8];
    uint8_t len = buttons_get_patch(patch);
    uint8_t sr_data[5] = {0};
    if (len == 0) {
        // Bypass: Guitar -> Amp
        // Example: Set DG408 to route input 0 to output 0
        sr_data[0] = 0x01; // Placeholder
    } else {
        // Route: Guitar -> patch[0] -> patch[1] -> ... -> Amp
        for (int i = 0; i < len; i++) {
            // Placeholder: Set DG408 bits for input i to output patch[i]
        }
    }
    shift_out(sr_data, 5);
}