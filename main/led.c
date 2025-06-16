
/**
 * @file led.c
 * @brief Implementation of LED control functions for ESP32 patch bay
 *
 * This file implements control of LEDs via 74HC595 shift registers,
 * including individual LED control, multiple LED control with bitmasks,
 * and brightness control using PWM on the output enable pin.
 */
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include "led.h" // Include our header file

/**
 * @file led.c
 * @brief Implementation of LED control functions for ESP32 patch bay
 *
 * This file implements control of LEDs via 74HC595 shift registers,
 * including individual LED control, multiple LED control with bitmasks,
 * and brightness control using PWM on the output enable pin.
 */

// Define GPIO pins for 74HC595 control (adjust as needed)
#define SER_PIN GPIO_NUM_5   // Serial data input
#define SRCLK_PIN GPIO_NUM_6 // Shift clock
#define RCLK_PIN GPIO_NUM_7  // Latch clock
#define OE_PIN GPIO_NUM_8    // Output enable (active-low)
#define SRCLR_PIN GPIO_NUM_9 // Shift register clear (active-low)

// LED mapping to shift register outputs (0-based index)
#define LED_PEDAL_1 0 // U801 QA
#define LED_PEDAL_3 1 // U801 QB
#define LED_PEDAL_4 2 // U801 QC
#define LED_STATUS 3  // U801 QD
#define LED_PEDAL_5 4 // U802 QA
#define LED_PEDAL_6 5 // U802 QB
#define LED_PEDAL_7 6 // U802 QC
#define LED_PEDAL_8 7 // U802 QD

// PWM parameters for dimming
#define PWM_PERIOD_MS 10                 // 10ms period (100Hz PWM frequency)
    static uint8_t pwm_duty_cycle = 100; // 0-100%, default full brightness
static volatile bool pwm_running = false;
static TaskHandle_t pwm_task_handle = NULL;

static const char *TAG = "LED_CONTROL";

// Current state of LEDs (0 = on, 1 = off for active-low)
static uint8_t led_state = 0xFF; // All LEDs off initially (all bits 1)

// Initialize GPIOs and shift registers
/**
 * Initialize GPIOs and shift registers
 *
 * Configures all necessary GPIO pins and initializes the shift registers
 * to a known state with all LEDs turned off.
 */
void led_init(void)
{
    // Configure GPIO pins
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << SER_PIN) | (1ULL << SRCLK_PIN) | (1ULL << RCLK_PIN) |
                        (1ULL << OE_PIN) | (1ULL << SRCLR_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&io_conf);

    // Initialize shift register
    gpio_set_level(SRCLR_PIN, 1); // Clear disabled
    gpio_set_level(OE_PIN, 0);    // Outputs enabled
    gpio_set_level(SER_PIN, 0);
    gpio_set_level(SRCLK_PIN, 0);
    gpio_set_level(RCLK_PIN, 0);

    // Clear shift registers
    gpio_set_level(SRCLR_PIN, 0); // Assert clear
    vTaskDelay(1 / portTICK_PERIOD_MS);
    gpio_set_level(SRCLR_PIN, 1); // Release clear

    // Update shift registers with initial state (all off)
    led_update();
}

// Update shift registers with current LED state
/**
 * Update shift registers with current LED state
 *
 * Shifts out the current LED state to the 74HC595 shift registers.
 * This function shifts the bits MSB first for U802, then U801.
 * All updates to LED states should call this function to apply changes.
 */
void led_update(void)
{
    // Shift out 8 bits (MSB first for U802, then U801)
    for (int i = 7; i >= 0; i--)
    {
        gpio_set_level(SER_PIN, (led_state >> i) & 1);
        gpio_set_level(SRCLK_PIN, 1); // Clock pulse
        vTaskDelay(1 / portTICK_PERIOD_MS);
        gpio_set_level(SRCLK_PIN, 0);
    }
    // Latch data to outputs
    gpio_set_level(RCLK_PIN, 1);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    gpio_set_level(RCLK_PIN, 0);
}

// Enable/disable a single LED
/**
 * Enable/disable a single LED
 *
 * Sets the state of a single LED identified by its index.
 * Note that LEDs are active-low in this design (0 = on, 1 = off).
 *
 * @param led_index The LED to control (use LED_* constants from led.h)
 * @param enable true to turn the LED on, false to turn it off
 */
void led_set(uint8_t led_index, bool enable)
{
    if (led_index >= 8)
    {
        ESP_LOGE(TAG, "Invalid LED index: %d", led_index);
        return;
    }
    // Active-low: 0 = on, 1 = off
    if (enable)
    {
        led_state &= ~(1 << led_index); // Clear bit to turn on
    }
    else
    {
        led_state |= (1 << led_index); // Set bit to turn off
    }
    led_update();
}

/**
 * Enable/disable multiple LEDs using a bitmask
 *
 * Sets the state of multiple LEDs at once using a bitmask.
 * This is more efficient than calling led_set() multiple times.
 * Note that LEDs are active-low in this design (0 = on, 1 = off).
 *
 * @param led_mask Bitmask of LEDs to control (set bit for each LED)
 * @param enable true to turn the LEDs on, false to turn them off
 */
void led_set_multiple(uint8_t led_mask, bool enable)
{
    if (enable)
    {
        led_state &= ~led_mask; // Clear bits to turn on
    }
    else
    {
        led_state |= led_mask; // Set bits to turn off
    }
    led_update();
}

// PWM task for dimming all LEDs
/**
 * PWM task for dimming all LEDs
 *
 * FreeRTOS task that implements software PWM by toggling the OE pin
 * of the shift registers at a frequency determined by PWM_PERIOD_MS.
 * The duty cycle controls the ratio of on/off time, affecting brightness.
 *
 * @param pvParameters Task parameters (unused)
 */
void pwm_task(void *pvParameters)
{
    while (pwm_running)
    {
        if (pwm_duty_cycle > 0)
        {
            gpio_set_level(OE_PIN, 0); // Enable outputs
            vTaskDelay((pwm_duty_cycle * PWM_PERIOD_MS / 100) / portTICK_PERIOD_MS);
        }
        if (pwm_duty_cycle < 100)
        {
            gpio_set_level(OE_PIN, 1); // Disable outputs
            vTaskDelay(((100 - pwm_duty_cycle) * PWM_PERIOD_MS / 100) / portTICK_PERIOD_MS);
        }
    }
    gpio_set_level(OE_PIN, 0); // Ensure outputs are enabled when stopping
    vTaskDelete(NULL);
}

/**
 * Set dimming level (0-100%) for all LEDs
 *
 * Controls LED brightness using PWM on the output enable pin.
 * - For 100% brightness, the PWM task is stopped and OE is kept low
 * - For 0% brightness, the PWM task is stopped and OE is kept high
 * - For values between 0-100%, a PWM task is created/managed
 *
 * @param duty_cycle Brightness level (0-100%)
 *                   0 = off, 100 = full brightness
 */
void led_set_brightness(uint8_t duty_cycle)
{
    if (duty_cycle > 100)
    {
        ESP_LOGE(TAG, "Invalid duty cycle: %d", duty_cycle);
        return;
    }
    pwm_duty_cycle = duty_cycle;

    // Start or stop PWM task based on duty cycle
    if (duty_cycle == 100 && pwm_running)
    {
        pwm_running = false; // Stop PWM for full brightness
        vTaskDelete(pwm_task_handle);
        gpio_set_level(OE_PIN, 0); // Enable outputs
    }
    else if (duty_cycle == 0 && pwm_running)
    {
        pwm_running = false; // Stop PWM for fully off
        vTaskDelete(pwm_task_handle);
        gpio_set_level(OE_PIN, 1); // Disable outputs
    }
    else if (!pwm_running && duty_cycle > 0 && duty_cycle < 100)
    {
        pwm_running = true;
        xTaskCreate(pwm_task, "pwm_task", 2048, NULL, 5, &pwm_task_handle);
    }
}

/**
 * Example usage of the LED control functions (commented out)
 *
 * This example demonstrates how to:
 * - Initialize the LED system
 * - Turn individual LEDs on and off
 * - Control multiple LEDs with a bitmask
 * - Adjust LED brightness
 */
/* void app_main(void)
{
    led_init();

    // Example: Turn on Pedal_1 and Status LEDs
    led_set(LED_PEDAL_1, true);
    led_set(LED_STATUS, true);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // Example: Turn off Pedal_1
    led_set(LED_PEDAL_1, false);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // Example: Turn on Pedal_3, Pedal_4, and Pedal_5 using bitmask
    led_set_multiple((1 << LED_PEDAL_3) | (1 << LED_PEDAL_4) | (1 << LED_PEDAL_5), true);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // Example: Dim all LEDs to 50%
    led_set_brightness(50);
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    // Example: Restore full brightness
    led_set_brightness(100);
} */