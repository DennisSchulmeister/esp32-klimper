/*
 * Klimper: ESP32 I²S Synthesizer Test
 * © 2024 Dennis Schulmeister-Zimolong <dennis@wpvs.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

#pragma once

#include <stdbool.h>                        // bool, true, false
#include "dsp/adsr.h"
#include "dsp/oscil.h"
#include "dsp/wavetable.h"

/**
 * Internal state of a tone-generating voice.
 */
typedef struct {
    bool  active;                           // Whether the note is being heard
    int   note;                             // MIDI note number
    float velocity;                         // Velocity
    float direction;                        // Panorama change delta

    dsp_oscil_t* osc1;                      // Carrier: Wavetable oscillator
    dsp_adsr_t*  env1;                      // Carrier: Amplitude envelope generator
    dsp_oscil_t* lfo1;                      // LFO hard-wired to panorama

    dsp_oscil_t* osc2;                      // Modulator: Wavetable oscillator
    dsp_adsr_t*  env2;                      // Modulator: Amplitude envelope generator

    float fm_index_2_1;                     // OSC2->OSC1: FM Index
    float fm_ratio_2_1;                     // OSC2->OSC1: FM Ratio
} synth_voice_t;

/**
 * Configuration parameters for the FM synthesis
 */
typedef struct {
        int    n_ratios;
        float* ratios;
        float  index_min;
        float  index_max;
} synth_fm_params;

/**
 * A very simple, polyphonic wavetable synthesizer. Nothing to write home about. :-)
 */
typedef struct {
    volatile struct {
        float volume;                       // Overall volume

        dsp_adsr_values_t env1;             // Carrier: Amplitude envelope generator parameters
        dsp_adsr_values_t env2;             // Modulator: Amplitude envelope generator parameters
        synth_fm_params   fm;               // Frequency modulation parameters
    } params;

    struct {
        int   polyphony;                    // Number of voices
        float gain_staging;                 // Gain factor to avoid clipping

        synth_voice_t* voices;              // Voices that actually create sound
    } state;
} synth_t;

/**
 * Configuration parameters for the synthesizer.
 */
typedef struct {
    float volume;                           // Overall volume

    dsp_wavetable_t*  wavetable;            // Oscillator wavetable
    dsp_adsr_values_t env1;                 // Carrier: Amplitude envelope generator parameters
    dsp_adsr_values_t env2;                 // Modulator: Amplitude envelope generator parameters
    synth_fm_params   fm;                   // Frequency modulation parameters
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
 * Set parameters of the carrier amplitude envelope generator.
 *
 * @param synth Synthesizer instance
 * @param env1 Attack, decay, sustain, release
 */
void synth_set_env1_values(synth_t* synth, dsp_adsr_values_t env1);

/**
 * Set parameters of the modulator amplitude envelope generator.
 *
 * @param synth Synthesizer instance
 * @param env1 Attack, decay, sustain, release
 */
void synth_set_env2_values(synth_t* synth, dsp_adsr_values_t env2);

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
