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

#include <stddef.h>
#include <stdbool.h>
#include "synth.h"

struct sequencer_pimpl;

/**
 * A simple sequencer that triggers random notes from a given scale
 * on the synthesizer.
 */
typedef struct sequencer {
    struct sequencer_pimpl* pimpl;          // Private implementation
} sequencer_t;

/**
 * Configuration parameters for the sequencer.
 */
typedef struct sequencer_config {
    int      sample_rate;                   // Sample rate in Hz
    synth_t* synth;                         // The controlled synthesizer (not freed by `sequencer_free()`)
    int*     notes;                         // Array with available MIDI notes (will be copied)
    size_t   n_notes;                       // Number of available MIDI notes
} sequencer_config_t;

/**
 * Create a new sequencer instance.
 * 
 * @param config Configuration parameters (can be freed afterwards)
 * @returns Pointer to the sequencer instance
 */
sequencer_t* sequencer_new(sequencer_config_t* config);

/**
 * Delete a sequencer instance as a replacement for `free()`. This makes sure that
 * the internal data structures will be completely freed.
 * 
 * @param sequencer Sequencer instance
 */
void sequencer_free(sequencer_t* sequencer);

/**
 * Change the musical tempo of the sequencer.
 * 
 * @param sequencer Sequencer instance
 * @param bpm New beats-per-minute
 */
void sequencer_set_bpm(sequencer_t* sequencer, int bpm);

/**
 * Get the musical tempo of the sequencer.
 * 
 * @param sequencer Sequencer instance
 * @returns Current beats-per-minute
 */
int sequencer_get_bpm(sequencer_t* sequencer);

/**
 * Set play state to start or stop the sequencer.
 * 
 * @param sequencer Sequencer instance
 * @param running Play state
 */
void sequencer_set_running(sequencer_t* sequencer, bool running);

/**
 * Get play state of the sequencer.
 */
bool sequencer_get_running(sequencer_t* sequencer);

/**
 * Run the sequencer to create new events based on the time passed since the last call.
 * This "plays" the synthesizer and updates its parameters. Passed time is calculated
 * based on the given number of samples rendered since the last call.
 * 
 * @param sequencer Sequencer instance
 * @param n_samples_pass Number of samples rendered since the last call
 */
void sequencer_process(sequencer_t* sequencer, size_t n_samples_passed);