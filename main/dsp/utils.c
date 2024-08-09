/*
 * ESP32 I²S Synthesizer Test / µDSP Library
 * © 2024 Dennis Schulmeister-Zimolong <dennis@wpvs.de>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

#include "utils.h"

#include <math.h>                           // pow()
#include <stdbool.h>                        // bool, true, false

/**
 * Convert MIDI note number to frequency in Hz. Algorithm adapted from
 * Richard Dobson, "Programming in C", in "The Audio Programming Book",
 * Ed. Richard Boulanger and Victor Lazzarini, 2011, p.69.
 */
float mtof(float midinote) {
    // NOTE: Static variables in C are initialized at compile-time, in C++ at runtime!
    // The `init` variable emulates the C++ behaviour that allows function calls to
    // initialize static variables when the function first runs.
    static float ratio, c0;
    static bool init = true;

    if (init) {
        ratio = pow(2, 1/12.0);
        c0    = 220.0 * pow(ratio, 3) * pow(0.5, 5);
        init  = false;
    }

    return c0 * pow(ratio, midinote);
}