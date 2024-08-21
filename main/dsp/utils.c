/*
 * Klimper: ESP32 I²S Synthesizer Test / µDSP Library
 * © 2024 Dennis Schulmeister-Zimolong <dennis@wpvs.de>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

#include "utils.h"

#include <math.h>                                   // pow()
#include <stdbool.h>                                // bool, true, false

static float equal_tuning_ratio = 1.059463094f;     // pow(2, 1/12.0)
static float equal_tuning_c0    = 8.175798916f;     // 220.0 * pow(ratio, 3) * pow(0.5, 5);

/**
 * Convert MIDI note number to frequency in Hz. Algorithm adapted from
 * Richard Dobson, "Programming in C", in "The Audio Programming Book",
 * Ed. Richard Boulanger and Victor Lazzarini, 2011, p.69. Ratio and
 * C0 reference note calculated in advance to save processing cycles.
 */
float mtof(float midinote) {
    return equal_tuning_c0 * pow(equal_tuning_ratio, midinote);
}
