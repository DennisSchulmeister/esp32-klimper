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

#ifndef PI
    #define PI (3.14159265)
#endif

#ifndef TWO_PI
    #define TWO_PI (6.2831853)
#endif

/**
 * Convert MIDI note number to frequency in Hz in the equal tempered scale
 * (using a frequency ratio of 2 ^ 1/12 per semitone). The MIDI note is given
 * as a float value here to allow pitch-bend.
 * 
 * @param midinote MIDI note number (60 = middle C)
 * @returns Frequency in Hz
 */
float mtof(float midinote);