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
#include "wavetable.h"                      // wavetable_t

extern bool             dsp_pan_initialized;
extern dsp_wavetable_t* dsp_pan_wt_cos;
extern dsp_wavetable_t* dsp_pan_wt_sin;

/**
 * Static initialization of the wave tables and calculated constants for
 * the panorama algorithm. Must be called at least once before using the
 * `dsp_pan_stereo()` function the first time.
 */
void dsp_pan_init();

/**
 * Apply equal-power pan law to the given sample. The panning value ranges from
 * minus one to one from left to right. Algorithm from "The Audio Programming Book",
 * p.234ff, simplified and changed to use pre-calculated wave tables.
 * 
 * @param sample Input sample
 * @param pan Pan value [-1 … 1]
 * @param left [out] Left output sample
 * @param right [out] Right output sample
 */
static inline void dsp_pan_stereo(float sample, float pan, float* left, float* right) {
    float index = (pan + 1.0f) * 0.125f * DSP_WAVETABLE_DEFAULT_LENGTH;

    *left  = sample * dsp_wavetable_read2(dsp_pan_wt_cos, index);
    *right = sample * dsp_wavetable_read2(dsp_pan_wt_sin, index);
}