/**
 * @file matrix.h
 * @brief Audio signal routing matrix for the ESP32 Patch Bay
 * 
 * This file provides the interface for the audio signal routing matrix which controls
 * the actual audio path through the pedal effects chain using shift registers.
 */

#ifndef MATRIX_H
#define MATRIX_H

/**
 * @brief Initialize the matrix hardware
 * 
 * Sets up the GPIO pins used for controlling the shift registers that
 * implement the audio signal routing matrix.
 */
void matrix_init(void);

/**
 * @brief Update the routing matrix based on current patch configuration
 * 
 * Retrieves the current patch configuration from the buttons subsystem and
 * updates the shift registers to route the audio signal accordingly.
 */
void matrix_update(void);

#endif