/*
 * ESP32 I²S Synthesizer Test / µDSP Library
 * © 2024 Dennis Schulmeister-Zimolong <dennis@wpvs.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

#include "wavetable.h"
#include "utils.h"                          // TWO_PI

// Pre-defined wavetable functions and global singleton wavetables
float dsp_wavetable_sin(float x) { return sin(x); }
float dsp_wavetable_cos(float x) { return cos(x); }

dsp_wavetable_t* default_wavetables[DSP_WAVETABLE_N] = {};

dsp_wavetable_func_ptr default_wavetable_funcs[DSP_WAVETABLE_N] = {
    // Array index must match the corresponding enum value!
    dsp_wavetable_sin,
    dsp_wavetable_cos,
};

/**
 * Create a new wavetable and fill it with values (with default options).
 */
dsp_wavetable_t* dsp_wavetable_new(dsp_wavetable_func_ptr func) {
    return dsp_wavetable_new_custom(CONFIG_DSP_WAVETABLE_LENGTH, 1, func);
}

/**
 * Create a new wavetable and fill it with values (with cusdom options).
 */
dsp_wavetable_t* dsp_wavetable_new_custom(size_t length, size_t guards, dsp_wavetable_func_ptr func) {
    dsp_wavetable_t* wavetable = malloc(sizeof(dsp_wavetable_t));

    wavetable->length = length;
    wavetable->samples = malloc((length + guards) * sizeof(float));

    float incr = TWO_PI / length;

    for (unsigned int i = 0; i < length; i++) {
        wavetable->samples[i] = func(i * incr);
    }

    for (unsigned int i = 0; i < guards; i++) {
        wavetable->samples[length + i] = wavetable->samples[i];
    }

    return wavetable;
}

/**
 * Free memory of a given wavetable.
 */
void dsp_wavetable_free(dsp_wavetable_t* wavetable) {
    free(wavetable->samples);
    free(wavetable);
}

/**
 * Get a pointer to one of the default wavetables. Create if missing.
 */
dsp_wavetable_t* dsp_wavetable_get(dsp_wavetable_default_t which) {
    if (default_wavetables[which] == NULL) {
        default_wavetables[which] = dsp_wavetable_new(default_wavetable_funcs[which]);
    }

    return default_wavetables[which];
}

