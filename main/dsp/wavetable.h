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
#include <math.h>                           // sin(), cos(), …
#include <stdlib.h>                         // size_t

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

float dsp_wavetable_sin(float x);
float dsp_wavetable_cos(float x);

/**
 * Built-in default wavetables that can be accessed globaly with `dsp_wavetable_get()`.
 */
typedef enum {
    DSP_WAVETABLE_SIN = 0,
    DSP_WAVETABLE_COS,
    DSP_WAVETABLE_N,                        // Dummy entry for the enum length
} dsp_wavetable_default_t;

/**
 * Create a new wavetable with default lenght (config option) and one guard point.
 * Fill the wavetable with values computed from the given wavetable function, which
 * gets a float with the wavetable index to calculate the corresponding float value.
 *
 * @param func Function to calculate the wavetable
 */
dsp_wavetable_t* dsp_wavetable_new(dsp_wavetable_func_ptr func);

/**
 * Create a new wavetable and fill it with values. Optionally add the given number
 * of guard points at the end, extending the wavetable length accordingly. The
 * length of the wavetable will be `CONFIG_DSP_WAVETABLE_LENGTH` bytes.
 *
 * @param length Size of the new wavetable
 * @param guards Number of additional guard points
 * @param func Function to calculate the wavetable
 * @returns Pointer to the new wavetable
 */
dsp_wavetable_t* dsp_wavetable_new_custom(size_t length, size_t guards, dsp_wavetable_func_ptr func);

/**
 * Free memory of a given wavetable.
 *
 * @param wavetable Wavetable instance
 */
void dsp_wavetable_free(dsp_wavetable_t* wavetable);

/**
 * Get a pointer to one of the default wavetables. The wavetable will be created the first time
 * this function is called and must never be freed. It is essentially a global singleton so that
 * we don't need to have multiple copies of the same wavetable in memory.
 *
 * @param which Type of the requested wavetable
 * @returns Pointer to the global wavetable (don't free!)
 */
dsp_wavetable_t* dsp_wavetable_get(dsp_wavetable_default_t which);

/**
 * Read wavetable with linear interpolation. Note that the wavetable must have at
 * least one guard point for this to work. Otherwise a garbage value will be read
 * at the end of the table.
 *
 * Algorithm from "The Audio Programming Book", p302ff.
 *
 * @param wavetable Wavetable instance
 * @param index Floating point index
 * @returns Linearly interpolated value
 */
static inline float dsp_wavetable_read2(dsp_wavetable_t* wavetable, float index) {
    int   iindex = (int) index;
    float value  = wavetable->samples[iindex];
    float slope  = wavetable->samples[iindex + 1] - value;

    return value + (slope * (index - iindex));
}
