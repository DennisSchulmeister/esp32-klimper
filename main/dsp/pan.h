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