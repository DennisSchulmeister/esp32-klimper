/*
 * ESP32 I²S Synthesizer Test
 * © 2024 Dennis Schulmeister-Zimolong <dennis@wpvs.de>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

#include "synth.h"

#include <esp_log.h>                        // ESP_LOGx
#include <stdlib.h>                         // calloc(), malloc(), free()
#include <math.h>                           // cos()
#include "dsp/utils.h"                      // mtof()

static const char* TAG = "synth";           // Logging tag

/**
 * Create new synthesizer instance.
 */
synth_t* synth_new(synth_config_t* config) {
    ESP_LOGD(TAG, "Creating new synthesizer instance");

    synth_t* synth = calloc(1, sizeof(synth_t));

    synth->params.volume    = config->volume;
    synth_set_aenv_values(synth, config->aenv);

    synth->state.sample_rate = config->sample_rate;
    synth->state.polyphony   = config->polyphony;
    synth->state.voices      = calloc(config->polyphony, sizeof(synth_voice_t));

    for (int i = 0; i < config->polyphony; i++) {
        synth->state.voices[i].oscil = dsp_oscil_new(config->wavetable);
        synth->state.voices[i].aenv  = dsp_adsr_new();
        dsp_adsr_set_values(synth->state.voices[i].aenv, config->sample_rate, &config->aenv);
    }

    ESP_LOGD(TAG, "Created synthesizer instance %p", synth);
    
    return synth;
}

/**
 * Free synthesizer instance.
 */
void synth_free(synth_t* synth) {
    ESP_LOGD(TAG, "Freeing synthesizer instance %p", synth);

    for (int i = 0; i < synth->state.polyphony; i++) {
        free(synth->state.voices[i].oscil);
        free(synth->state.voices[i].aenv);
    }

    free(synth->state.voices);
    free(synth);

}

/**
 * Set overall volume of the synthesizer.
 */
void synth_set_volume(synth_t* synth, float volume)  {
    ESP_LOGD(TAG, "Set volume to %f", volume);

    synth->params.volume = volume;
}

/**
 * Set parameters of the amplitude envelope generator.
 */
void synth_set_aenv_values(synth_t* synth, dsp_adsr_values_t aenv) {
    synth->params.aenv = aenv;

    for (int i = 0; i < synth->state.polyphony; i++) {
        dsp_adsr_set_values(synth->state.voices[i].aenv, synth->state.sample_rate, &aenv);
    }
}

/**
 * Play a new note or re-trigger playing with with same number.
 */
void synth_note_on(synth_t* synth, int note, float velocity) {
}

/**
 * Stop a currently playing note.
 */
void synth_note_off(synth_t* synth, int note) {
}

/**
 * Render next audio block.
 */
void synth_process(synth_t* synth, float* audio_buffer, size_t length) {
}