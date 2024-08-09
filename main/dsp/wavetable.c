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

float dsp_wavetable_sin(float x) { return sin(x); }
float dsp_wavetable_cos(float x) { return cos(x); }

/**
 * Create a new wavetable and fill it with values.
 */
dsp_wavetable_t* dsp_wavetable_new(size_t length, size_t guards, dsp_wavetable_func_ptr func) {
    dsp_wavetable_t* wavetable = malloc(sizeof(dsp_wavetable_t));

    wavetable->length = length;
    wavetable->samples = malloc((length + guards) * sizeof(float));

    float incr = TWO_PI / length;

    for (int i = 0; i < length; i++) {
        wavetable->samples[i] = func(i * incr);
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
