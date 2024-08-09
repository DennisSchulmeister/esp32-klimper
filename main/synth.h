/*
 * ESP32 I²S Synthesizer Test
 * © 2024 Dennis Schulmeister-Zimolong <dennis@wpvs.de>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

#pragma once

#include <stddef.h>                         // size_t

struct synth_pimpl;

/**
 * A very simple, polyphonic wavetable synthesizer. Nothing to write home about. :-)
 */
typedef struct {
    struct synth_pimpl* pimpl;              // Private implementation
} synth_t;

/**
 * Configuration parameters for the synthesizer.
 */
typedef struct {
    int sample_rate;                        // Sample rate in Hz
    int polyphony;                          // Maximum number of simultaneous voices
} synth_config_t;

/**
 * Create a new synthesizer instance.
 * 
 * @param config Configuration parameters (can be freed afterwards)
 * @returns Pointer to the synthesizer instance
 */
synth_t* synth_new(synth_config_t* config);

/**
 * Delete a synthesizer instance as a replacement for `free()`. This makes sure that
 * the internal data structures will be completely freed.
 * 
 * @param synth Synthesizer instance
 */
void synth_free(synth_t* synth);

/**
 * Set overall volume of the synthesizer.
 * 
 * @param synth Synthesizer instance
 * @param volume Volume level [0…1]
 */
void synth_set_volume(synth_t* synth, float volume);

/**
 * Play a new note or re-trigger an already playing note of the same number.
 * 
 * @param synth Synthesizer instance
 * @param note MIDI note number
 * @param velocity Note velocity (0…1)
 */
void synth_note_on(synth_t* synth, int note, float velocity);

/**
 * Stop a currently playing note.
 * 
 * @param synth Synthesizer instance
 * @param note MIDI note number
 */
void synth_note_off(synth_t* synth, int note);

/**
 * Render a new block of audio with the currently set parameters and notes.
 * The buffer is expected to be two-channel left/right interleaved.
 * 
 * @param synth Synthesizer instance
 * @param audio_buffer Audio buffer to render into
 * @param length Length of the audio buffer
 */
void synth_process(synth_t* synth, float* audio_buffer, size_t length);