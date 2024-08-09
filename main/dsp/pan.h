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

/**
 * Apply equal-power pan law to the given sample. The panning value ranges from
 * minus one to one from left to right.
 * 
 * @param sample Input sample
 * @param pan Pan value [-1 … 1]
 * @param left [out] Left output sample
 * @param right [out] Right output sample
 */
inline void dsp_pan_stereo(float sample, float pan, float* left, float* right);


#include <stdbool.h>                        // bool, true, false
#include <math.h>                           // cos(), sin(), sqrt()
#include "wavetable.h"                      // wavetable_t

static bool dsp_pan_init = true;
static float dsp_pan_const;
static dsp_wavetable_t* dsp_pan_wt_cos;
static dsp_wavetable_t* dsp_pan_wt_sin;

/**
 * Static initialization of the wave tables and calculated constants.
 */
inline void _init_wavetables() {
    if (!dsp_pan_init) return;
    dsp_pan_init = false;

    dsp_pan_const = sqrt(2) * 0.5;

    dsp_pan_wt_cos = dsp_wavetable_new(DSP_WAVETABLE_DEFAULT_LENGTH, 1, &dsp_wavetable_cos);
    dsp_pan_wt_sin = dsp_wavetable_new(DSP_WAVETABLE_DEFAULT_LENGTH, 1, &dsp_wavetable_sin);
}

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
inline void dsp_pan_stereo(float sample, float pan, float* left, float* right) {
    _init_wavetables();

    float cos_value = dsp_wavetable_read2(dsp_pan_wt_cos, pan);
    float sin_value = dsp_wavetable_read2(dsp_pan_wt_sin, pan);

    *left  = dsp_pan_const * (cos_value + sin_value);
    *right = dsp_pan_const * (cos_value - sin_value);
}