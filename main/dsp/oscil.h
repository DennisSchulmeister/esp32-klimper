/*
 * ESP32 I²S Synthesizer Test / µDSP Library
 * © 2024 Dennis Schulmeister-Zimolong <dennis@wpvs.de>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

#pragma once

#include <stdbool.h>                        // bool, true, false
#include "wavetable.h"                      // dsp_wavetable_t

/**
 * Simple wavetable oscillator with linear interpolation. Note that this requires
 * a wavetable with at least one guard point.
 */
typedef struct {
    dsp_wavetable_t* wavetable;             // Wavetable (not freed)
    float            frequency;             // Frequency in Hz
    float            increment;             // Index increment
    float            index;                 // Current index
} dsp_oscil_t;

/**
 * Create a new oscillator instance and initialize it with zero values.
 * 
 * @param wavetable Wavetable (must have one guard point)
 * @returns Pointer to the new oscillator
 */
dsp_oscil_t* dsp_oscil_new(dsp_wavetable_t* wavetable);

/**
 * Reinitialize oscillator with the given frequency. Optionally the index can be
 * skipped in order to realize frequency sweeps and pitch bends.
 * 
 * @param oscil Oscillator instance
 * @param sample_rate Sample rate in Hz
 * @param frequency New frequency in Hz
 * @param reset_index Reset index to zero
 */
void dsp_oscil_reinit(dsp_oscil_t* oscil, int sample_rate, float frequency, bool reset_index);

/**
 * Free memory of a given oscillator.
 * @param oscil Oscillator instance
 */
void dsp_oscil_free(dsp_oscil_t* oscil);

/**
 * Calculate next sample.
 * @param oscil Oscillator instance
 */
inline float dsp_oscil_tick(dsp_oscil_t* oscil);