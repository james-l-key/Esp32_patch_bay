/**
 * @file buttons.c
 * @brief Implementation of button handling and patch bay system state management
 *
 * This file implements the button interface, debouncing, and system state machine
 * for the ESP32 Patch Bay. It handles user input for creating, editing, saving,
 * and recalling effect chain presets, as well as managing the active audio
 * signal routing configuration.
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_log.h>
#include <string.h> // For memset, memcpy, memcmp
#include <stdio.h>  // For snprintf

#include "sdkconfig.h"
#include "buttons.h"
#include "matrix.h"
#include "gui.h"

#define NVS_NAMESPACE "patch_bay"       /**< NVS namespace for storing patch data */
#define NVS_KEY_LIVE_CONFIG "live_cfg"  /**< NVS key for the live configuration */
#define NVS_KEY_PRESET_PREFIX "preset_" /**< NVS key prefix for preset configurations */

// --- Button Configuration (Ensure these are in sdkconfig.h) ---
// Example: #define CONFIG_EDIT_SAVE_BUTTON_PIN 25
// Example: #define CONFIG_PRESET_BUTTON_PIN 26
// Example: #define CONFIG_PEDAL_BUTTON_1_PIN 12
// ...etc. for PEDAL_BUTTON_2_PIN to PEDAL_BUTTON_8_PIN

/** @brief Tag for logging */
static const char *TAG = "Buttons";

// --- Global State Variables ---
/** @brief Current system mode (live, programming, recall, save) */
static patch_bay_system_mode_t current_system_mode = MODE_LIVE;
/** @brief Current active patch configuration */
static uint8_t live_patch_data[NUM_PEDALS_MAX] = {0};
/** @brief Length of the current active patch */
static uint8_t live_patch_len = 0;
/** @brief Index of the preset the current patch was loaded from (-1 if custom) */
static int8_t loaded_from_preset_slot = -1; // 0-7 if live_patch_data matches a preset, -1 otherwise

// --- Button Hardware Definitions ---
/** @brief GPIO pins for pedal buttons */
static const gpio_num_t PEDAL_BUTTON_PINS[NUM_PEDALS_MAX] = {
    CONFIG_PEDAL_BUTTON_1_PIN, CONFIG_PEDAL_BUTTON_2_PIN, CONFIG_PEDAL_BUTTON_3_PIN, CONFIG_PEDAL_BUTTON_4_PIN,
    CONFIG_PEDAL_BUTTON_5_PIN, CONFIG_PEDAL_BUTTON_6_PIN, CONFIG_PEDAL_BUTTON_7_PIN, CONFIG_PEDAL_BUTTON_8_PIN};

#ifdef CONFIG_ENABLE_LEDS
// We'll define a shift register-based LED implementation
// The LEDs are controlled by a shift register with a single data pin
static const gpio_num_t LED_SR_DATA_PIN = CONFIG_LED_SR_DATA_PIN;
static const gpio_num_t LED_SR_CLOCK_PIN = CONFIG_SR_CLOCK_PIN; // Reusing main shift register clock
static const gpio_num_t LED_SR_LATCH_PIN = CONFIG_SR_LATCH_PIN; // Reusing main shift register latch
#endif

// --- Button State Tracking for Press Types ---
/**
 * @brief Structure for tracking button state and press types
 */
typedef struct
{
    gpio_num_t pin;           /**< GPIO pin number of the button */
    bool current_state;       /**< Current debounced state of the button */
    bool last_state;          /**< Previous raw state for debounce handling */
    uint32_t press_time_ms;   /**< Timestamp when button was pressed */
    uint32_t release_time_ms; /**< Timestamp when button was released */
    bool short_press_event;   /**< Flag indicating a short press was detected */
    bool long_press_event;    /**< Flag indicating a long press was detected */
    bool ongoing_long_press;  /**< Flag indicating an ongoing long press */
} button_state_t;

/** @brief State tracking for edit/save button */
static button_state_t edit_save_btn_state;
/** @brief State tracking for preset button */
static button_state_t preset_btn_state;
/** @brief State tracking for pedal buttons */
static button_state_t pedal_btn_states[NUM_PEDALS_MAX];

#define DEBOUNCE_TIME_MS 50         /**< Button debounce time in milliseconds */
#define LONG_PRESS_DURATION_MS 1500 /**< Duration in milliseconds to detect a long press */

// --- NVS Helper Functions ---
/**
 * @brief Save a patch configuration to NVS
 *
 * @param key NVS key to save the patch under
 * @param data Pointer to the patch data array
 * @param len Length of the patch data
 * @return esp_err_t ESP_OK on success, or an error code
 */
static esp_err_t _save_patch_to_nvs(const char *key, const uint8_t *data, uint8_t len)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return err;
    }

    // Store length + data
    uint8_t nvs_buffer[NUM_PEDALS_MAX + 1];
    nvs_buffer[0] = len;
    memcpy(&nvs_buffer[1], data, len);
    // Zero out remaining part of the buffer to ensure consistent blob size if needed
    if (len < NUM_PEDALS_MAX)
    {
        memset(&nvs_buffer[1 + len], 0, NUM_PEDALS_MAX - len);
    }

    err = nvs_set_blob(nvs_handle, key, nvs_buffer, NUM_PEDALS_MAX + 1);
    if (err == ESP_OK)
    {
        err = nvs_commit(nvs_handle);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "NVS commit failed for key %s! Error: %s", key, esp_err_to_name(err));
        }
    }
    else
    {
        ESP_LOGE(TAG, "NVS set_blob failed for key %s! Error: %s", key, esp_err_to_name(err));
    }
    nvs_close(nvs_handle);
    return err;
}

/**
 * @brief Load a patch configuration from NVS
 *
 * @param key NVS key to load the patch from
 * @param data_buf Buffer to receive the patch data
 * @param len_buf Pointer to receive the length of the patch
 * @return esp_err_t ESP_OK on success, or an error code
 */
static esp_err_t _load_patch_from_nvs(const char *key, uint8_t *data_buf, uint8_t *len_buf)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS R/O handle for key %s", esp_err_to_name(err), key);
        *len_buf = 0; // Ensure length is zero on error
        memset(data_buf, 0, NUM_PEDALS_MAX);
        return err;
    }

    uint8_t nvs_buffer[NUM_PEDALS_MAX + 1];
    size_t required_size = sizeof(nvs_buffer);

    err = nvs_get_blob(nvs_handle, key, nvs_buffer, &required_size);
    if (err == ESP_OK)
    {
        if (required_size == (NUM_PEDALS_MAX + 1))
        {
            *len_buf = nvs_buffer[0];
            if (*len_buf > NUM_PEDALS_MAX)
                *len_buf = NUM_PEDALS_MAX; // Sanity check
            memcpy(data_buf, &nvs_buffer[1], *len_buf);
            if (*len_buf < NUM_PEDALS_MAX)
            { // Zero out rest of buffer if loaded patch is shorter
                memset(data_buf + *len_buf, 0, NUM_PEDALS_MAX - *len_buf);
            }
        }
        else
        {
            ESP_LOGE(TAG, "NVS blob size mismatch for key %s. Expected %d, got %d", key, NUM_PEDALS_MAX + 1, required_size);
            err = ESP_FAIL; // Treat as error
            *len_buf = 0;
            memset(data_buf, 0, NUM_PEDALS_MAX);
        }
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGI(TAG, "NVS key %s not found, initializing to empty.", key);
        *len_buf = 0;
        memset(data_buf, 0, NUM_PEDALS_MAX);
        // Don't return error for not_found, treat as empty patch
        err = ESP_OK;
    }
    else
    {
        ESP_LOGE(TAG, "NVS get_blob failed for key %s! Error: %s", key, esp_err_to_name(err));
        *len_buf = 0;
        memset(data_buf, 0, NUM_PEDALS_MAX);
    }
    nvs_close(nvs_handle);
    return err;
}

/**
 * @brief Compares the current live patch with a stored preset
 *
 * Checks if the current live patch configuration matches a specific stored preset.
 * Used to determine if the current configuration was loaded from a preset.
 *
 * @param preset_slot_index Index of the preset to compare against (0-7)
 * @return true if the live patch matches the preset, false otherwise
 */
static bool _is_live_patch_same_as_preset(uint8_t preset_slot_index)
{
    uint8_t preset_data[NUM_PEDALS_MAX];
    uint8_t preset_len;
    char key[20];
    snprintf(key, sizeof(key), "%s%d", NVS_KEY_PRESET_PREFIX, preset_slot_index);
    if (_load_patch_from_nvs(key, preset_data, &preset_len) == ESP_OK)
    {
        if (preset_len == live_patch_len && memcmp(preset_data, live_patch_data, live_patch_len) == 0)
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief Updates the status of which preset the current patch was loaded from
 *
 * Checks if the current live patch matches any of the stored presets and
 * updates the loaded_from_preset_slot variable accordingly.
 */
static void _update_loaded_from_preset_slot_status()
{
    loaded_from_preset_slot = -1; // Assume not from preset
    for (int i = 0; i < NUM_PRESETS; i++)
    {
        if (_is_live_patch_same_as_preset(i))
        {
            loaded_from_preset_slot = i;
            break;
        }
    }
}

// --- LED Control Functions ---
#ifdef CONFIG_ENABLE_LEDS

/**
 * @brief Updates the LED status using a shift register
 *
 * Shifts out the current LED status to the LED shift register
 *
 * @param led_status An 8-bit value representing the on/off state of each LED
 */
static void update_led_shift_register(uint8_t led_status)
{
    // Set latch low to begin data transfer
    gpio_set_level(LED_SR_LATCH_PIN, 0);

    // Shift out the data
    for (int i = 7; i >= 0; i--)
    {
        gpio_set_level(LED_SR_DATA_PIN, (led_status >> i) & 0x01);
        gpio_set_level(LED_SR_CLOCK_PIN, 1);
        gpio_set_level(LED_SR_CLOCK_PIN, 0);
    }

    // Set latch high to latch the data
    gpio_set_level(LED_SR_LATCH_PIN, 1);
}

/**
 * @brief Set the state of a pedal LED
 *
 * Controls the LED associated with a specific pedal using the shift register
 *
 * @param pedal_index Index of the pedal (0-7)
 * @param on true to turn the LED on, false to turn it off
 */
static void _set_pedal_led(uint8_t pedal_index, bool on)
{
    static uint8_t led_status = 0; // Keep track of LED states
    
    if (pedal_index < NUM_PEDALS_MAX)
    {
        if (on) {
            led_status |= (1 << pedal_index);  // Set the bit
        } else {
            led_status &= ~(1 << pedal_index); // Clear the bit
        }
        update_led_shift_register(led_status);
    }
}

static void _update_active_chain_leds()
{
    for (int i = 0; i < NUM_PEDALS_MAX; i++)
    { // Turn all off first
        _set_pedal_led(i, false);
    }
    for (int i = 0; i < live_patch_len; i++)
    {
        if (live_patch_data[i] > 0 && live_patch_data[i] <= NUM_PEDALS_MAX)
        {
            _set_pedal_led(live_patch_data[i] - 1, true);
        }
    }
}

static void _flash_all_pedal_leds(int count, int duration_ms_on, int duration_ms_off)
{
    ESP_LOGI(TAG, "LEDs: Flashing %d times.", count);
    for (int c = 0; c < count; c++)
    {
        for (int i = 0; i < NUM_PEDALS_MAX; i++)
            _set_pedal_led(i, true);
        vTaskDelay(pdMS_TO_TICKS(duration_ms_on));
        for (int i = 0; i < NUM_PEDALS_MAX; i++)
            _set_pedal_led(i, false);
        if (c < count - 1)
            vTaskDelay(pdMS_TO_TICKS(duration_ms_off));
    }
    _update_active_chain_leds(); // Restore LEDs to current chain state
}

static void _blink_all_pedal_leds_start(bool start_blinking)
{
    // This would ideally be handled by a separate LED task or timer
    // For now, just a placeholder concept
    static bool blinking_active = false; // Used to track blinking state
    if (blinking_active != start_blinking)
    {
        blinking_active = start_blinking;
        // Additional logic if needed
    }
    ESP_LOGI(TAG, "LEDs: Blinking %s.", start_blinking ? "started" : "stopped");
    if (!start_blinking)
    {
        _update_active_chain_leds();
    }
    else
    {
        // In a real implementation, a timer would toggle LEDs
        // For simplicity here, just turn them on if starting, off if stopping
        for (int i = 0; i < NUM_PEDALS_MAX; i++)
            _set_pedal_led(i, start_blinking);
    }
}
#else
// No-op versions if LEDs are disabled
static void _set_pedal_led(uint8_t pedal_index, bool on) {}
static void _update_active_chain_leds() {}
static void _flash_all_pedal_leds(int count, int duration_ms_on, int duration_ms_off)
{
    ESP_LOGI(TAG, "LEDs disabled, flash requested.");
}
static void _blink_all_pedal_leds_start(bool start_blinking)
{
    ESP_LOGI(TAG, "LEDs disabled, blink requested.");
}
#endif

// --- Button Processing Function ---
/**
 * @brief Process a button's state to detect presses and releases
 *
 * Handles debouncing and detects short and long presses for a button.
 * Updates the button's state and sets event flags appropriately.
 *
 * @param btn Pointer to the button_state_t structure for the button
 */
static void _process_button(button_state_t *btn)
{
    bool raw_state = !gpio_get_level(btn->pin); // Active low
    uint32_t current_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

    btn->short_press_event = false;
    btn->long_press_event = false; // This indicates a completed long press

    if (raw_state != btn->last_state)
    { // State change
        btn->last_state = raw_state;
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_TIME_MS)); // Simple debounce
        raw_state = !gpio_get_level(btn->pin);       // Read again after debounce
        if (raw_state != btn->current_state)
        { // Debounced state change
            btn->current_state = raw_state;
            if (btn->current_state)
            { // Pressed
                btn->press_time_ms = current_time_ms;
                btn->ongoing_long_press = false;
            }
            else
            { // Released
                btn->release_time_ms = current_time_ms;
                if (btn->ongoing_long_press)
                {
                    btn->long_press_event = true; // Long press completed on release
                    btn->ongoing_long_press = false;
                }
                else
                {
                    btn->short_press_event = true;
                }
            }
        }
    }

    // Check for ongoing long press (if still pressed)
    if (btn->current_state && !btn->ongoing_long_press)
    {
        if ((current_time_ms - btn->press_time_ms) >= LONG_PRESS_DURATION_MS)
        {
            // This is a "long press fire" event, usually action is taken on release or specific need
            // For this design, we trigger save mode selection on long press *detection*
            // and then action (pedal button press) confirms
            if (btn->pin == CONFIG_PRESET_BUTTON_PIN)
            {                                   // Only preset button uses this ongoing detection for mode change
                btn->ongoing_long_press = true; // Mark that a long press has been achieved
                // The mode change will happen in the main task loop based on this flag
            }
        }
    }
}

/**
 * @brief Initialize the buttons subsystem
 *
 * Configures GPIO pins for buttons, sets up internal state, and loads the last
 * saved configuration from NVS. Also initializes LED outputs if enabled.
 */
void buttons_init(void)
{
    // Configure Edit/Save Button and Preset Button
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CONFIG_PROGRAM_BUTTON_PIN) | (1ULL << CONFIG_PRESET_BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // Configure Pedal Buttons
    uint64_t pedal_pin_mask = 0;
    for (int i = 0; i < NUM_PEDALS_MAX; i++)
    {
        pedal_pin_mask |= (1ULL << PEDAL_BUTTON_PINS[i]);
    }
    io_conf.pin_bit_mask = pedal_pin_mask;
    gpio_config(&io_conf);

#ifdef CONFIG_ENABLE_LEDS
    // Configure LED shift register pins
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_SR_DATA_PIN) |
                         (1ULL << LED_SR_CLOCK_PIN) |
                         (1ULL << LED_SR_LATCH_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);
    
    // Initialize all LEDs to off
    update_led_shift_register(0);
#endif

    // Initialize button states
    edit_save_btn_state.pin = CONFIG_PROGRAM_BUTTON_PIN;
    preset_btn_state.pin = CONFIG_PRESET_BUTTON_PIN;
    for (int i = 0; i < NUM_PEDALS_MAX; i++)
    {
        pedal_btn_states[i].pin = PEDAL_BUTTON_PINS[i];
    }

    // Load live_config on startup
    esp_err_t err = _load_patch_from_nvs(NVS_KEY_LIVE_CONFIG, live_patch_data, &live_patch_len);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    { // NOT_FOUND is handled as empty, other errors are more serious
        gui_set_status("NVS Load Err!");
        // Initialize to a known safe state (bypass)
        live_patch_len = 0;
        memset(live_patch_data, 0, NUM_PEDALS_MAX);
    }
    _update_loaded_from_preset_slot_status(); // Check if it matches any preset
    _update_active_chain_leds();
    matrix_update(); // Update matrix with loaded/default config
    gui_update_chain(live_patch_data, live_patch_len, loaded_from_preset_slot);
    gui_set_status(loaded_from_preset_slot != -1 ? "P%d Loaded" : "Live Config", loaded_from_preset_slot != -1 ? loaded_from_preset_slot + 1 : 0);
    
    // Now that we have better I2C settings, we can try a controlled refresh
    gui_force_refresh();
    vTaskDelay(pdMS_TO_TICKS(1500)); // Show initial status
    gui_set_status("");

    current_system_mode = MODE_LIVE;
}

/**
 * @brief Main task for handling button presses and system state
 *
 * This task continuously monitors button inputs, handles debouncing, detects
 * short and long presses, and manages the system state machine. It controls
 * the effects chain configuration based on user input and manages preset saving and loading.
 *
 * @param pvParameters FreeRTOS task parameters (unused)
 */
void buttons_task(void *pvParameters)
{
    char key_name_buffer[20]; // For NVS key construction

    while (1)
    {
        // Process all buttons to update their event flags
        _process_button(&edit_save_btn_state);
        _process_button(&preset_btn_state);
        for (int i = 0; i < NUM_PEDALS_MAX; i++)
        {
            _process_button(&pedal_btn_states[i]);
        }

        // --- Main State Machine ---
        switch (current_system_mode)
        {
        case MODE_LIVE:
            if (edit_save_btn_state.short_press_event)
            {
                current_system_mode = MODE_PROGRAM_CHAIN;
                loaded_from_preset_slot = -1;
                live_patch_len = 0;
                memset(live_patch_data, 0, sizeof(live_patch_data));
                gui_update_chain(live_patch_data, live_patch_len, loaded_from_preset_slot);
                gui_set_status("Program Chain");
                _flash_all_pedal_leds(1, 50, 0);
                _update_active_chain_leds(); // Should show no LEDs as chain is empty
            }
            else if (preset_btn_state.short_press_event)
            {
                current_system_mode = MODE_RECALL_SLOT_SELECT;
                gui_set_status("Recall: Select Slot");
                _blink_all_pedal_leds_start(true);
            }
            else if (preset_btn_state.ongoing_long_press)
            {                                                // Detected long press initiation
                preset_btn_state.ongoing_long_press = false; // Consume this event for mode change
                current_system_mode = MODE_SAVE_SLOT_SELECT;
                gui_set_status("Save To: Select Slot");
                _blink_all_pedal_leds_start(true); // Use blinking for save select too
            }
            break;

        case MODE_PROGRAM_CHAIN:
            if (edit_save_btn_state.short_press_event)
            { // Finalize programming
                // live_patch_data is already updated by pedal presses
                matrix_update();
                _save_patch_to_nvs(NVS_KEY_LIVE_CONFIG, live_patch_data, live_patch_len);
                loaded_from_preset_slot = -1; // It's a custom live config now
                current_system_mode = MODE_LIVE;
                gui_update_chain(live_patch_data, live_patch_len, loaded_from_preset_slot);
                gui_set_status("Chain Set & Saved Live");
                _flash_all_pedal_leds(2, 50, 50);
                _update_active_chain_leds();
                vTaskDelay(pdMS_TO_TICKS(1500));
                gui_set_status(""); // Clear status or show live chain info
            }
            else if (preset_btn_state.short_press_event)
            {                                                                                // Cancel programming
                _load_patch_from_nvs(NVS_KEY_LIVE_CONFIG, live_patch_data, &live_patch_len); // Revert
                _update_loaded_from_preset_slot_status();
                matrix_update();
                current_system_mode = MODE_LIVE;
                gui_update_chain(live_patch_data, live_patch_len, loaded_from_preset_slot);
                gui_set_status("Program Canceled");
                _update_active_chain_leds();
                vTaskDelay(pdMS_TO_TICKS(1500));
                gui_set_status("");
            }
            else
            {
                for (int i = 0; i < NUM_PEDALS_MAX; i++)
                {
                    if (pedal_btn_states[i].short_press_event)
                    {
                        if (live_patch_len < NUM_PEDALS_MAX)
                        {
                            // Check if pedal already in chain to prevent duplicates (optional)
                            bool found = false;
                            for (int j = 0; j < live_patch_len; ++j)
                                if (live_patch_data[j] == (i + 1))
                                    found = true;

                            if (!found)
                            {
                                live_patch_data[live_patch_len++] = i + 1;             // Pedal numbers are 1-based
                                gui_update_chain(live_patch_data, live_patch_len, -1); // Show as custom
                                _set_pedal_led(i, true);
                            }
                            else
                            { // Toggle off if already in chain
                                // This part requires more complex chain editing logic (remove, reorder)
                                // For now, let's just prevent adding if already there, or allow re-adding
                                // Simple toggle:
                                // uint8_t temp_patch[NUM_PEDALS_MAX];
                                // uint8_t temp_len = 0;
                                // for(int j=0; j<live_patch_len; ++j) {
                                //    if(live_patch_data[j] != (i+1)) temp_patch[temp_len++] = live_patch_data[j];
                                // }
                                // memcpy(live_patch_data, temp_patch, temp_len);
                                // live_patch_len = temp_len;
                                // _set_pedal_led(i, false);
                                // gui_update_chain(live_patch_data, live_patch_len, -1);
                                gui_set_status("Pedal %d in chain", i + 1);
                                vTaskDelay(pdMS_TO_TICKS(500));
                                gui_set_status("Program Chain");
                            }
                        }
                        else
                        {
                            gui_set_status("Chain Full!");
                            vTaskDelay(pdMS_TO_TICKS(1000));
                            gui_set_status("Program Chain");
                        }
                    }
                }
            }
            break;

        case MODE_RECALL_SLOT_SELECT:
            _blink_all_pedal_leds_start(true); // Keep blinking
            if (preset_btn_state.short_press_event || edit_save_btn_state.short_press_event)
            { // Cancel
                current_system_mode = MODE_LIVE;
                _blink_all_pedal_leds_start(false);
                gui_update_chain(live_patch_data, live_patch_len, loaded_from_preset_slot);
                gui_set_status("Recall Canceled");
                vTaskDelay(pdMS_TO_TICKS(1500));
                gui_set_status("");
            }
            else
            {
                for (int i = 0; i < NUM_PRESETS; i++)
                { // NUM_PRESETS is 8
                    if (pedal_btn_states[i].short_press_event)
                    {
                        snprintf(key_name_buffer, sizeof(key_name_buffer), "%s%d", NVS_KEY_PRESET_PREFIX, i);
                        if (_load_patch_from_nvs(key_name_buffer, live_patch_data, &live_patch_len) == ESP_OK)
                        {
                            loaded_from_preset_slot = i;
                            matrix_update();
                            _save_patch_to_nvs(NVS_KEY_LIVE_CONFIG, live_patch_data, live_patch_len); // Update live config
                            gui_set_status("P%d Loaded & Set Live", i + 1);
                        }
                        else
                        {
                            gui_set_status("Slot P%d Load Err", i + 1);
                            // live_patch_data and len might be modified by failed load, reload live_config
                            _load_patch_from_nvs(NVS_KEY_LIVE_CONFIG, live_patch_data, &live_patch_len);
                            _update_loaded_from_preset_slot_status();
                        }
                        _blink_all_pedal_leds_start(false);
                        _flash_all_pedal_leds(2, 50, 50);
                        current_system_mode = MODE_LIVE;
                        gui_update_chain(live_patch_data, live_patch_len, loaded_from_preset_slot);
                        vTaskDelay(pdMS_TO_TICKS(1500));
                        gui_set_status("");
                        break; // Exit loop once a pedal is pressed
                    }
                }
            }
            break;

        case MODE_SAVE_SLOT_SELECT:
            _blink_all_pedal_leds_start(true); // Keep blinking
            if (preset_btn_state.short_press_event || edit_save_btn_state.short_press_event)
            { // Cancel
                current_system_mode = MODE_LIVE;
                _blink_all_pedal_leds_start(false);
                gui_update_chain(live_patch_data, live_patch_len, loaded_from_preset_slot);
                gui_set_status("Save Canceled");
                vTaskDelay(pdMS_TO_TICKS(1500));
                gui_set_status("");
            }
            else
            {
                for (int i = 0; i < NUM_PRESETS; i++)
                {
                    if (pedal_btn_states[i].short_press_event)
                    {
                        snprintf(key_name_buffer, sizeof(key_name_buffer), "%s%d", NVS_KEY_PRESET_PREFIX, i);
                        if (_save_patch_to_nvs(key_name_buffer, live_patch_data, live_patch_len) == ESP_OK)
                        {
                            loaded_from_preset_slot = i;                                              // Live data now matches this preset
                            _save_patch_to_nvs(NVS_KEY_LIVE_CONFIG, live_patch_data, live_patch_len); // Also update live config
                            gui_set_status("Saved to P%d", i + 1);
                        }
                        else
                        {
                            gui_set_status("Save P%d Err", i + 1);
                        }
                        _blink_all_pedal_leds_start(false);
                        _flash_all_pedal_leds(2, 50, 50);
                        current_system_mode = MODE_LIVE;
                        gui_update_chain(live_patch_data, live_patch_len, loaded_from_preset_slot);
                        vTaskDelay(pdMS_TO_TICKS(1500));
                        gui_set_status("");
                        break; // Exit loop
                    }
                }
            }
            break;
        }

        // Clear event flags after processing
        edit_save_btn_state.short_press_event = false;
        edit_save_btn_state.long_press_event = false;
        preset_btn_state.short_press_event = false;
        preset_btn_state.long_press_event = false;
        // preset_btn_state.ongoing_long_press is handled carefully for mode entry
        for (int i = 0; i < NUM_PEDALS_MAX; i++)
        {
            pedal_btn_states[i].short_press_event = false;
            pedal_btn_states[i].long_press_event = false;
        }

        vTaskDelay(pdMS_TO_TICKS(20)); // Main task loop delay
    }
}

// Function for matrix.c to get the current patch
/**
 * @brief Provides the current patch configuration to the matrix driver
 *
 * Copies the current active patch configuration into the provided buffer
 * and sets the length of the configuration. Used by the matrix module to
 * update the hardware signal routing.
 *
 * @param patch_buffer Buffer to receive the current patch data
 * @param length_buffer Pointer to receive the length of the current patch
 */
void buttons_get_current_patch_for_matrix(uint8_t *patch_buffer, uint8_t *length_buffer)
{
    memcpy(patch_buffer, live_patch_data, live_patch_len);
    *length_buffer = live_patch_len;
    // Ensure rest of buffer is zero if live_patch_len < NUM_PEDALS_MAX
    if (live_patch_len < NUM_PEDALS_MAX)
    {
        memset(patch_buffer + live_patch_len, 0, NUM_PEDALS_MAX - live_patch_len);
    }
}