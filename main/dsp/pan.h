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
#include <math.h>                           // cos(), sin(), sqrt()
#include "wavetable.h"                      // wavetable_t

extern bool             dsp_pan_initialized;
extern float            dsp_pan_const;
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
 * p.234ff and changed to use pre-calculated wave tables.
 * 
 * @param sample Input sample
 * @param pan Pan value [-1 … 1]
 * @param left [out] Left output sample
 * @param right [out] Right output sample
 */
static inline void dsp_pan_stereo(float sample, float pan, float* left, float* right) {
    float cos_value = dsp_wavetable_read2(dsp_pan_wt_cos, pan);
    float sin_value = dsp_wavetable_read2(dsp_pan_wt_sin, pan);

    *left  = dsp_pan_const * (cos_value + sin_value);
    *right = dsp_pan_const * (cos_value - sin_value);
}