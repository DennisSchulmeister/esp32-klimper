/*
 * ESP32 I²S Synthesizer Test
 * © 2024 Dennis Schulmeister-Zimolong <dennis@wpvs.de>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

#include "sequencer.h"

#include <esp_log.h>
#include <stdlib.h>
#include <string.h>

static const char* TAG = "sequencer";                   // Logging tag

#define N_NOTES_PLAYING (8)                             // Worst-case: Four 16th-notes, 1/4 long = 4 notes

/**
 * Beat durations: Quarter, eighth, sixteenth notes
 */
typedef enum sequencer_duration {
    QUARTER,
    EIGHTH,
    SIXTEENTH,
    MAX_DURATION,                                       // Dummy value to mark the end
} sequencer_duration_t;

/**
 * Played note as seen by the sequencer. This tracks the remaining playing time
 * of a triggered note until it will be stopped again.
 */
typedef struct sequencer_note {
    int note;                                           // MIDI note number
    int samples_remaining;                              // Number of samples until the note is stopped
} sequencer_note_t;

/**
 * Private properties of the sequencer (pimpl idiom).
 */
typedef struct sequencer_pimpl {
    synth_t*         synth;                             // The controlled synthesizer (not freed by `sequencer_free()`)
    int              sample_rate;                       // Sample rate in Hz
    int*             notes_available;                   // Array with available MIDI notes
    size_t           n_notes_available;                 // Number of available MIDI notes
    int              bpm;                               // Tempo in beats per minute
    bool             running;                           // Play state (playing or stopped)
    int              durations[3];                      // Sample durations for quarter/eighth/sixteenth notes
    int              pause_remaining;                   // Number of samples until the next note is triggered
    sequencer_note_t notes_playing[N_NOTES_PLAYING];    // Currently played notes
} sequencer_pimpl_t;

/**
 * Create a new sequencer instance.
 */
sequencer_t* sequencer_new(sequencer_config_t* config) {
    ESP_LOGD(TAG, "Creating new sequencer instance");

    sequencer_t* sequencer = calloc(1, sizeof(sequencer_t));

    sequencer->pimpl = calloc(1, sizeof(sequencer_pimpl_t));
    sequencer->pimpl->synth             = config->synth;
    sequencer->pimpl->sample_rate       = config->sample_rate;
    sequencer->pimpl->n_notes_available = config->n_notes;

    sequencer->pimpl->notes_available = malloc(config->n_notes * sizeof(int));
    memcpy(sequencer->pimpl->notes_available, config->notes, config->n_notes * sizeof(int));

    ESP_LOGD(TAG, "Created sequencer instance %p", sequencer);    

    return sequencer;
}

/**
 * Delete a sequencer instance.
 */
void sequencer_free(sequencer_t* sequencer) {
    ESP_LOGD(TAG, "Freeing sequencer instance %p", sequencer);

    free(sequencer->pimpl);
    free(sequencer);
}

/**
 * Change the musical tempo of the sequencer.
 */
void sequencer_set_bpm(sequencer_t* sequencer, int bpm) {
    ESP_LOGD(TAG, "Setting musical tempo to %i bpm.", bpm);

    sequencer->pimpl->bpm = bpm;
    sequencer->pimpl->durations[QUARTER]   = sequencer->pimpl->sample_rate * 60 / bpm;
    sequencer->pimpl->durations[EIGHTH]    = sequencer->pimpl->durations[QUARTER] / 2;
    sequencer->pimpl->durations[SIXTEENTH] = sequencer->pimpl->durations[EIGHTH]  / 2;

    ESP_LOGD(TAG, "Duration of a 1/4  note: %i samples", sequencer->pimpl->durations[QUARTER]);
    ESP_LOGD(TAG, "Duration of a 1/8  note: %i samples", sequencer->pimpl->durations[EIGHTH]);
    ESP_LOGD(TAG, "Duration of a 1/16 note: %i samples", sequencer->pimpl->durations[SIXTEENTH]);
}

/**
 * Get the musical tempo of the sequencer.
 */
int sequencer_get_bpm(sequencer_t* sequencer) {
    return sequencer->pimpl->bpm;
}

/**
 * Set play state to start or stop the sequencer.
 */
void sequencer_set_running(sequencer_t* sequencer, bool running) {
    ESP_LOGD(TAG, "Set play state to %s", running ? "playing" : "stopped");

    sequencer->pimpl->running = running;
    sequencer->pimpl->pause_remaining = 0;
}

/**
 * Get play state of the sequencer.
 */
bool sequencer_get_running(sequencer_t* sequencer) {
    return sequencer->pimpl->running;
}

/**
 * Run the sequencer for another processing block.
 */
void sequencer_process(sequencer_t* sequencer, size_t n_samples_passed) {
    // Trigger note-off events
    for (int i = 0; i < N_NOTES_PLAYING; i++) {
        if (sequencer->pimpl->notes_playing[i].samples_remaining <= 0) continue;
        sequencer->pimpl->notes_playing[i].samples_remaining -= n_samples_passed;

        if (sequencer->pimpl->notes_playing[i].samples_remaining <= 0) {
            ESP_LOGD(TAG, "Triggering note-off for note %i", sequencer->pimpl->notes_playing[i].note);
            synth_note_off(sequencer->pimpl->synth, sequencer->pimpl->notes_playing[i].note);
        }
    }

    // Pause sequencer between notes
    if (!sequencer->pimpl->running) return;

    sequencer->pimpl->pause_remaining -= n_samples_passed;
    if (sequencer->pimpl->pause_remaining > 0) return;      // Signed int to avoid edge case at first call

    sequencer_duration_t pause_duration = rand() % MAX_DURATION;
    sequencer->pimpl->pause_remaining = sequencer->pimpl->durations[pause_duration];

    ESP_LOGD(TAG, "Pause is over. New pause duration: %i samples", sequencer->pimpl->pause_remaining);

    // Play new note
    for (int i = 0; i < N_NOTES_PLAYING; i++) {
        if (sequencer->pimpl->notes_playing[i].samples_remaining > 0) continue;

        int note_index = rand() % sequencer->pimpl->n_notes_available;
        sequencer_duration_t note_duration = rand() % MAX_DURATION;
        float velocity = (rand() % 256) / 255.0;

        sequencer->pimpl->notes_playing[i].note = sequencer->pimpl->notes_available[note_index];
        sequencer->pimpl->notes_playing[i].samples_remaining = sequencer->pimpl->durations[note_duration];

        ESP_LOGD(TAG, "Triggering note-on for note %i with velocity %f and duration %i samples",
                sequencer->pimpl->notes_playing[i].note, velocity, sequencer->pimpl->notes_playing[i].samples_remaining);
        
        synth_note_on(sequencer->pimpl->synth, sequencer->pimpl->notes_playing[i].note, velocity);
        break;
    }
}