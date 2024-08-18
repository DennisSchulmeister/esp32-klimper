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

#include <esp_log.h>                                    // ESP_LOGx
#include <string.h>                                     // memcpy()

static const char* TAG = "sequencer";                   // Logging tag

/**
 * Create a new sequencer instance.
 */
sequencer_t* sequencer_new(sequencer_config_t* config) {
    ESP_LOGD(TAG, "Creating new sequencer instance");

    sequencer_t* sequencer = calloc(1, sizeof(sequencer_t));

    sequencer->params.synth = config->synth;

    sequencer->notes_available.midi_notes = malloc(config->n_notes * sizeof(int));
    sequencer->notes_available.length     = config->n_notes;
    memcpy(sequencer->notes_available.midi_notes, config->notes, config->n_notes * sizeof(int));

    ESP_LOGI(TAG, "Created sequencer instance %p", sequencer);

    return sequencer;
}

/**
 * Delete a sequencer instance.
 */
void sequencer_free(sequencer_t* sequencer) {
    ESP_LOGD(TAG, "Freeing sequencer instance %p", sequencer);

    free(sequencer->notes_available.midi_notes);
    free(sequencer);
}

/**
 * Change the musical tempo of the sequencer.
 */
void sequencer_set_bpm(sequencer_t* sequencer, int bpm) {
    ESP_LOGD(TAG, "Setting musical tempo to %i bpm.", bpm);

    sequencer->params.bpm = bpm;
    sequencer->state.durations[SEQUENCER_DURATION_QUARTER]   = CONFIG_AUDIO_SAMPLE_RATE * 60 / bpm;
    sequencer->state.durations[SEQUENCER_DURATION_EIGHTH]    = sequencer->state.durations[SEQUENCER_DURATION_QUARTER] / 2;
    sequencer->state.durations[SEQUENCER_DURATION_SIXTEENTH] = sequencer->state.durations[SEQUENCER_DURATION_EIGHTH]  / 2;

    ESP_LOGD(TAG, "Duration of a 1/4  note: %i samples", sequencer->state.durations[SEQUENCER_DURATION_QUARTER]);
    ESP_LOGD(TAG, "Duration of a 1/8  note: %i samples", sequencer->state.durations[SEQUENCER_DURATION_EIGHTH]);
    ESP_LOGD(TAG, "Duration of a 1/16 note: %i samples", sequencer->state.durations[SEQUENCER_DURATION_SIXTEENTH]);
}

/**
 * Set play state to start or stop the sequencer.
 */
void sequencer_set_running(sequencer_t* sequencer, bool running) {
    ESP_LOGD(TAG, "Set play state to %s", running ? "playing" : "stopped");

    sequencer->params.running = running;
    sequencer->state.pause_remaining = 0;
}

/**
 * Run the sequencer for another processing block.
 */
void sequencer_process(sequencer_t* sequencer, size_t n_samples_passed) {
    // Trigger note-off events
    for (int i = 0; i < SEQUENCER_POLYPHONY; i++) {
        if (sequencer->state.notes_playing[i].samples_remaining <= 0) continue;
        sequencer->state.notes_playing[i].samples_remaining -= n_samples_passed;

        if (sequencer->state.notes_playing[i].samples_remaining <= 0) {
            ESP_LOGD(TAG, "Triggering note-off for note %i", sequencer->state.notes_playing[i].note);
            synth_note_off(sequencer->params.synth, sequencer->state.notes_playing[i].note);
        }
    }

    // Pause sequencer between notes
    if (!sequencer->params.running) return;

    sequencer->state.pause_remaining -= n_samples_passed;
    if (sequencer->state.pause_remaining > 0) return;      // Signed int to avoid edge case at first call

    sequencer_duration_t pause_duration = rand() % SEQUENCER_DURATION_MAX;
    sequencer->state.pause_remaining = sequencer->state.durations[pause_duration];

    ESP_LOGD(TAG, "Pause is over. New pause duration: %i samples", sequencer->state.pause_remaining);

    // Play new note
    for (int i = 0; i < SEQUENCER_POLYPHONY; i++) {
        if (sequencer->state.notes_playing[i].samples_remaining > 0) continue;

        int note_index = rand() % sequencer->notes_available.length;
        sequencer_duration_t note_duration = rand() % SEQUENCER_DURATION_MAX;
        float velocity = rand() % 256 / 255.0f;

        sequencer->state.notes_playing[i].note = sequencer->notes_available.midi_notes[note_index];
        sequencer->state.notes_playing[i].samples_remaining = sequencer->state.durations[note_duration];

        ESP_LOGD(TAG, "Triggering note-on for note %i with velocity %f and duration %i samples",
                sequencer->state.notes_playing[i].note, velocity, sequencer->state.notes_playing[i].samples_remaining);

        synth_note_on(sequencer->params.synth, sequencer->state.notes_playing[i].note, velocity);
        break;
    }
}
