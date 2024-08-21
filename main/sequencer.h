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

#include <stddef.h>                         // size_t
#include <stdbool.h>                        // bool, true, false
#include "synth.h"

#define SEQUENCER_POLYPHONY (8)             // Worst-case: Four 16th-notes, 1/4 long = 4 notes

/**
 * Beat durations: Quarter, eighth, sixteenth notes
 */
typedef enum {
    SEQUENCER_DURATION_QUARTER,
    SEQUENCER_DURATION_EIGHTH,
    SEQUENCER_DURATION_SIXTEENTH,
    SEQUENCER_DURATION_MAX,                 // Dummy value to mark the end
} sequencer_duration_t;

/**
 * Played note as seen by the sequencer. This tracks the remaining playing time
 * of a triggered note until it will be stopped again.
 */
typedef struct {
    int note;                               // MIDI note number
    int samples_remaining;                  // Number of samples until the note is stopped
} sequencer_note_t;

/**
 * A simple sequencer that triggers random notes from a given scale
 * on the synthesizer.
 */
typedef struct {
    volatile struct {
        synth_t* synth;                     // The controlled synthesizer (not freed)
        float    bpm;                       // Tempo in beats per minute
        bool     running;                   // Play state (playing or stopped)
    } params;

    struct {
        int*   midi_notes;                  // Array with MIDI notes to choose from
        size_t length;                      // Array length
    } notes_available;

    struct {
        int durations[3];                   // Sample durations for quarter/eighth/sixteenth notes
        int pause_remaining;                // Number of samples until the next note is triggered

        sequencer_note_t notes_playing[SEQUENCER_POLYPHONY];    // Currently played notes
    } state;                                // Internal sequencer state
} sequencer_t;

/**
 * Configuration parameters for the sequencer.
 */
typedef struct {
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
 * Set play state to start or stop the sequencer.
 *
 * @param sequencer Sequencer instance
 * @param running Play state
 */
void sequencer_set_running(sequencer_t* sequencer, bool running);

/**
 * Run the sequencer to create new events based on the time passed since the last call.
 * This "plays" the synthesizer and updates its parameters. Passed time is calculated
 * based on the given number of samples rendered since the last call.
 *
 * @param sequencer Sequencer instance
 * @param n_samples_pass Number of samples rendered since the last call
 */
void sequencer_process(sequencer_t* sequencer, size_t n_samples_passed);
