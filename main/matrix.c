/**
 * @file matrix.c
 * @brief Implementation of the audio signal routing matrix
 * 
 * This file implements the control of the audio signal routing matrix
 * using shift registers. It manages the actual physical audio path
 * configuration based on the current effects chain.
 */

#include <driver/gpio.h>
#include "sdkconfig.h"
#include "matrix.h"
#include "buttons.h" // buttons_get_patch will be replaced by direct use of live_patch_data

/**
 * @brief Shifts data out to the shift registers
 * 
 * @param data Pointer to the data bytes to shift out
 * @param len Number of bytes to shift out
 */
static void shift_out(uint8_t *data, size_t len)
{
    gpio_set_level(CONFIG_SR_LATCH_PIN, 0);
    for (int i = 0; i < len; i++)
    {
        for (int j = 7; j >= 0; j--)
        {
            gpio_set_level(CONFIG_SR_DATA_PIN, (data[i] >> j) & 1);
            gpio_set_level(CONFIG_SR_CLOCK_PIN, 1);
            gpio_set_level(CONFIG_SR_CLOCK_PIN, 0);
        }
    }
    gpio_set_level(CONFIG_SR_LATCH_PIN, 1);
}

/**
 * @brief Initialize the matrix hardware
 * 
 * Sets up the GPIO pins used for controlling the shift registers that
 * implement the audio signal routing matrix.
 */
void matrix_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CONFIG_SR_DATA_PIN) |
                        (1ULL << CONFIG_SR_CLOCK_PIN) |
                        (1ULL << CONFIG_SR_LATCH_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Good practice to define both
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
}

/**
 * @brief Update the routing matrix based on current patch configuration
 * 
 * Retrieves the current patch configuration from the buttons subsystem and
 * updates the shift registers to route the audio signal accordingly.
 * This function will be called by buttons_task when the live_patch_data changes.
 */
void matrix_update(void)
{
    uint8_t current_chain[NUM_PEDALS_MAX]; // NUM_PEDALS_MAX defined in buttons.h
    uint8_t chain_len;

    buttons_get_current_patch_for_matrix(current_chain, &chain_len);

    uint8_t sr_data[5] = {0}; // Placeholder size, adjust to your hardware
    if (chain_len == 0)
    {
        // Bypass: Guitar -> Amp
        // Example: Set DG408 to route input 0 to output 0
        sr_data[0] = 0x01; // Placeholder for bypass
        // Add actual bit manipulation for your shift registers and analog switches
    }
    else
    {
        // Route: Guitar -> patch[0] -> patch[1] -> ... -> Amp
        // Placeholder: Set DG408 bits for input i to output patch[i]
        // This requires detailed knowledge of your analog matrix and shift register setup.
        // For example, if sr_data[0] controls pedal 1 input, sr_data[1] pedal 1 output to pedal 2 input etc.
        for (int i = 0; i < chain_len; i++)
        {
            // Example: sr_data[current_chain[i]-1 / 2] |= (1 << ( (current_chain[i]-1 % 2) * 4 ) ); // Highly schematic
        }
        // This section is CRITICAL and needs to be filled based on your hardware.
    }
    shift_out(sr_data, sizeof(sr_data));
}