/*
 * Klimper: ESP32 I²S Synthesizer Test / µDSP Library
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
 * @param frequency New frequency in Hz
 * @param reset_index Reset index to zero
 */
void dsp_oscil_reinit(dsp_oscil_t* oscil, float frequency, bool reset_index);

/**
 * Free memory of a given oscillator.
 * @param oscil Oscillator instance
 */
void dsp_oscil_free(dsp_oscil_t* oscil);

/**
 * Calculate next sample. Algorithm from "The Audio Programming Book", p302ff.
 *
 * @param oscil Oscillator instance
 * @param modulator Sample output from modulator oscillator (use 0.0f for no FM)
 * @returns Next sample
 */
static inline float dsp_oscil_tick(dsp_oscil_t* oscil, float modulator) {
    float sample = dsp_wavetable_read2(oscil->wavetable, oscil->index);

    // // No FM
    // oscil->index += oscil->increment;

    // Linear FM (needs a low-pass filter to avoid aliasing)
    oscil->index += oscil->increment + (modulator * oscil->wavetable->length * 0.01f);

    // // Exponential FM (needs low-pass filter and lookup table for powf()! Formula really correct??)
    // oscil->index += oscil->increment * powf(2.0f, modulator);

    while (oscil->index >= oscil->wavetable->length) oscil->index -= oscil->wavetable->length;
    while (oscil->index < 0) oscil->index += oscil->wavetable->length;

    return sample;
}
