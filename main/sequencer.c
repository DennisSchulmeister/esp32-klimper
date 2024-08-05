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

static const char* TAG = "sequencer";       // Logging tag

/**
 * Private properties of the sequencer (pimpl idiom).
 */
typedef struct sequencer_pimpl {
    int      sample_rate;                   // Sample rate in Hz
    synth_t* synth;                         // The controlled synthesizer (not freed by `sequencer_free()`)
} sequencer_pimpl_t;

/**
 * Create a new sequencer instance.
 */
sequencer_t* sequencer_new(sequencer_config_t* config) {
    ESP_LOGD(TAG, "Creating new sequencer instance");

    sequencer_t* sequencer = calloc(1, sizeof(sequencer_t));

    sequencer->pimpl = calloc(1, sizeof(sequencer_pimpl_t));
    sequencer->pimpl->sample_rate = config->sample_rate;
    sequencer->pimpl->synth       = config->synth;

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
 * Run the sequencer for another processing block.
 */
void sequencer_process(sequencer_t* sequencer, size_t n_samples_passed) {
}