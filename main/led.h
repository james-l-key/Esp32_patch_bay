#ifndef LED_H
#define LED_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @file led.h
 * @brief LED control functions for ESP32 patch bay
 *
 * This header file defines the interface for controlling LEDs via 74HC595 shift registers
 * in the ESP32 patch bay project. It allows for turning individual LEDs on/off,
 * controlling multiple LEDs at once, and adjusting brightness through PWM.
 */

/**
 * @brief LED identifiers mapped to shift register outputs
 */
#define LED_PEDAL_1 0 /**< U801 QA */
#define LED_PEDAL_3 1 /**< U801 QB */
#define LED_PEDAL_4 2 /**< U801 QC */
#define LED_STATUS 3  /**< U801 QD */
#define LED_PEDAL_5 4 /**< U802 QA */
#define LED_PEDAL_6 5 /**< U802 QB */
#define LED_PEDAL_7 6 /**< U802 QC */
#define LED_PEDAL_8 7 /**< U802 QD */

/**
 * @brief Initialize LEDs and shift register GPIOs
 *
 * Configures the GPIO pins for controlling 74HC595 shift registers
 * and initializes them to a known state with all LEDs off.
 */
void led_init(void);

/**
 * @brief Update shift registers with current LED state
 *
 * Shifts out the current LED state to the 74HC595 shift registers.
 * This function is called automatically by the other LED control functions.
 */
void led_update(void);

/**
 * @brief Turn a single LED on or off
 *
 * @param led_index The LED to control (use LED_* constants)
 * @param enable true to turn the LED on, false to turn it off
 */
void led_set(uint8_t led_index, bool enable);

/**
 * @brief Control multiple LEDs at once using a bitmask
 *
 * @param led_mask Bitmask of LEDs to control (set bit for each LED)
 * @param enable true to turn the LEDs on, false to turn them off
 */
void led_set_multiple(uint8_t led_mask, bool enable);

/**
 * @brief Set brightness level for all LEDs
 *
 * Controls LED brightness using PWM on the output enable pin.
 * 
 * @param duty_cycle Brightness level (0-100%)
 *                   0 = off, 100 = full brightness
 */
void led_set_brightness(uint8_t duty_cycle);

#endif /* LED_H */
