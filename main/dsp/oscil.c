/*
 * ESP32 I²S Synthesizer Test / µDSP Library
 * © 2024 Dennis Schulmeister-Zimolong <dennis@wpvs.de>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

#include "oscil.h"

#include <stdlib.h>                         // calloc(), malloc(), free()

/**
 * Create new oscillator instance.
 */
dsp_oscil_t* dsp_oscil_new(dsp_wavetable_t* wavetable) {
    dsp_oscil_t* oscil = calloc(1, sizeof(dsp_oscil_t));
    oscil->wavetable = wavetable;
    return oscil;
}

/**
 * Reinitialize oscillator with the given frequency.
 */
void dsp_oscil_reinit(dsp_oscil_t* oscil, int sample_rate, float frequency, bool reset_index) {
    oscil->frequency = frequency;
    oscil->increment = frequency * oscil->wavetable->length / sample_rate;
    if (reset_index) oscil->index = 0;
}

/**
 * Free memory of a given oscillator.
 */
void dsp_oscil_free(dsp_oscil_t* oscil) {
    free(oscil);
}

/**
 * Calculate next sample. Algorithm from "The Audio Programming Book", p302ff.
 */
extern inline float dsp_oscil_tick(dsp_oscil_t* oscil) {
    float sample = dsp_wavetable_read2(oscil->wavetable, oscil->index);

    oscil->index += oscil->increment;
    while (oscil->index >= oscil->wavetable->length) oscil->index -= oscil->wavetable->length;
    while (oscil->index < 0) oscil->index += oscil->wavetable->length;

    return sample;
}