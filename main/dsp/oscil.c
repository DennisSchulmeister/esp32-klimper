/*
 * Klimper: ESP32 I²S Synthesizer Test / µDSP Library
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
void dsp_oscil_reinit(dsp_oscil_t* oscil, float frequency, bool reset_index) {
    oscil->frequency = frequency;
    oscil->increment = frequency * oscil->wavetable->length / CONFIG_AUDIO_SAMPLE_RATE;
    if (reset_index) oscil->index = 0;
}

/**
 * Free memory of a given oscillator.
 */
void dsp_oscil_free(dsp_oscil_t* oscil) {
    free(oscil);
}
