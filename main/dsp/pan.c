/*
 * ESP32 I²S Synthesizer Test / µDSP Library
 * © 2024 Dennis Schulmeister-Zimolong <dennis@wpvs.de>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

#include "pan.h"

#include <stdbool.h>                        // bool, true, false
#include <math.h>                           // cos(), sin(), sqrt()
#include "wavetable.h"                      // wavetable_t

dsp_wavetable_t* wt_cos;
dsp_wavetable_t* wt_sin;
float pan_const;

/**
 * Static initialization of the wave tables and calculated constants.
 */
inline void _init_wavetables() {
    static bool init = true;
    if (!init) return;

    wt_cos = dsp_wavetable_new(DSP_WAVETABLE_DEFAULT_LENGTH, 1, &cos);
    wt_sin = dsp_wavetable_new(DSP_WAVETABLE_DEFAULT_LENGTH, 1, &sin);

    pan_const = sqrt(2) * 0.5;
}

/**
 * Apply equal-power pan law to the given sample. Algorithm from "The Audio Programming Book",
 * p.234ff and changed to use pre-calculated wave tables.
 */
extern inline void dsp_pan_stereo(float sample, float pan, float* left, float* right) {
    _init_wavetables();

    float cos_value = dsp_wavetable_read2(wt_cos, pan);
    float sin_value = dsp_wavetable_read2(wt_sin, pan);

    *left  = pan_const * (cos_value + sin_value);
    *right = pan_const * (cos_value - sin_value);
}