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
#include <stdlib.h>                         // size_t

#define DSP_WAVETABLE_DEFAULT_LENGTH (512)

/**
 * A wavetable with sample values
 */
typedef struct {
    size_t length;
    float* samples;
} dsp_wavetable_t;

/**
 * Pointer to a function to calculate the wavetable values.
 * Values a calculated for the interval [0 … TWO_PI].
 */
typedef float (*dsp_wavetable_func_ptr)(float);

/**
 * Create a new wavetable and fill it with values. Optionally add the given number
 * of guard points at the end, extending the wavetable length accordingly.
 * 
 * @param length Size of the new wavetable
 * @param guards Number of additional guard points
 * @param func Function to calculate the wavetable
 * @returns Pointer to the new wavetable
 */
dsp_wavetable_t* dsp_wavetable_new(size_t length, size_t guards, dsp_wavetable_func_ptr func);

/**
 * Free memory of a given wavetable.
 * 
 * @param wavetable Wavetable instance
 */
void dsp_wavetable_free(dsp_wavetable_t* wavetable);

/**
 * Read wavetable with linear interpolation. Note that the wavetable must have at
 * least one guard point for this to work. Otherwise a garbage value will be read
 * at the end of the table.
 * 
 * @param wavetable Wavetable instance
 * @param index Floating point index
 * @returns Linearly interpolated value
 */
inline float dsp_wavetable_read2(dsp_wavetable_t* wavetable, float index);