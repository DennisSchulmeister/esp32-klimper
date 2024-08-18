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
#include <esp_attr.h>                       // IRAM_ATTR
#include <float.h>                          // FLT_MAX
#include <stdlib.h>                         // calloc(), malloc(), free()
#include <string.h>                         // memcpy()
#include <math.h>                           // cos()
#include "dsp/pan.h"                        // dsp_pan()
#include "dsp/utils.h"                      // mtof()

static const char* TAG = "synth";           // Logging tag

/**
 * Create new synthesizer instance.
 */
synth_t* synth_new(synth_config_t* config) {
    ESP_LOGD(TAG, "Creating new synthesizer instance");

    synth_t* synth = calloc(1, sizeof(synth_t));

    synth->params.volume    = config->volume;
    synth->params.fm        = config->fm;

    synth->params.fm.ratios = malloc(config->fm.n_ratios * sizeof(float));
    memcpy(synth->params.fm.ratios, config->fm.ratios, config->fm.n_ratios * sizeof(float));

    synth->state.gain_staging = 1.0f / CONFIG_SYNTH_POLYPHONY;
    synth->state.voices       = calloc(CONFIG_SYNTH_POLYPHONY, sizeof(synth_voice_t));

    for (int i = 0; i < CONFIG_SYNTH_POLYPHONY; i++) {
        synth->state.voices[i].osc1 = dsp_oscil_new(config->wavetable);
        synth->state.voices[i].env1 = dsp_adsr_new();
        synth->state.voices[i].lfo1 = dsp_oscil_new(config->wavetable);

        synth->state.voices[i].osc2 = dsp_oscil_new(config->wavetable);
        synth->state.voices[i].env2 = dsp_adsr_new();

        float lfo_freq = rand() % 256 / 255.0f * 3.0f + 0.33f;
        dsp_oscil_reinit(synth->state.voices[i].lfo1, lfo_freq, true);
    }

    synth_set_env1_values(synth, config->env1);
    synth_set_env2_values(synth, config->env2);

    dsp_pan_init();

    ESP_LOGI(TAG, "Created synthesizer instance %p with %i voices polyphony", synth, CONFIG_SYNTH_POLYPHONY);

    return synth;
}

/**
 * Free synthesizer instance.
 */
void synth_free(synth_t* synth) {
    ESP_LOGD(TAG, "Freeing synthesizer instance %p", synth);

    for (int i = 0; i < CONFIG_SYNTH_POLYPHONY; i++) {
        dsp_oscil_free(synth->state.voices[i].osc1);
        dsp_adsr_free(synth->state.voices[i].env1);
        dsp_oscil_free(synth->state.voices[i].lfo1);

        dsp_oscil_free(synth->state.voices[i].osc2);
        dsp_adsr_free(synth->state.voices[i].env2);
    }

    free(synth->params.fm.ratios);
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
 * Set parameters of the carrier amplitude envelope generator.
 */
void synth_set_env1_values(synth_t* synth, dsp_adsr_values_t env1) {
    synth->params.env1 = env1;

    for (int i = 0; i < CONFIG_SYNTH_POLYPHONY; i++) {
        dsp_adsr_set_values(synth->state.voices[i].env1, &env1);
    }
}

/**
 * Set parameters of the modulator amplitude envelope generator.
 */
void synth_set_env2_values(synth_t* synth, dsp_adsr_values_t env2) {
    synth->params.env2 = env2;

    for (int i = 0; i < CONFIG_SYNTH_POLYPHONY; i++) {
        dsp_adsr_set_values(synth->state.voices[i].env2, &env2);
    }
}

/**
 * Play a new note or re-trigger playing with with same number. Steal the least sounding
 * voice, if the maximum polyphony is exhausted. Once a voice has been allocated simply
 * trigger its envelope generator. The active flag is tied to the envelope generator status
 * in `synth_process()`.
 */
void synth_note_on(synth_t* synth, int note, float velocity) {
    // Voice allocation with re-triggering and voice stealing
    synth_voice_t* retrigger   = NULL;
    synth_voice_t* free_voice  = NULL;
    synth_voice_t* steal_voice = NULL;
    float          steal_amp   = FLT_MAX;

    for (int i = 0; i < CONFIG_SYNTH_POLYPHONY; i++) {
        synth_voice_t* voice = &synth->state.voices[i];
        float          amp   = voice->velocity * voice->env1->state.value;

        if (voice->note == note) {
            retrigger = voice;
        }

        if (!voice->active) {
            free_voice = voice;
        } else if (amp < steal_amp) {
            steal_voice = voice;
            steal_amp   = amp;
        }
    }

    synth_voice_t* voice = retrigger ? retrigger : free_voice ? free_voice : steal_voice;

    voice->note = note;
    voice->velocity = velocity;

    // Update FM parameters
    int fm_ratio_index = rand() % synth->params.fm.n_ratios;
    voice->fm_ratio_2_1 = synth->params.fm.ratios[fm_ratio_index];
    voice->fm_index_2_1 = rand() % 256 / 255.0f * (synth->params.fm.index_max - synth->params.fm.index_min) + synth->params.fm.index_min;

    // Trigger oscilators and envelope generator
    float freq1 = mtof(note);
    float freq2 = freq1 * voice->fm_ratio_2_1;

    dsp_oscil_reinit(voice->osc1, freq1, false);
    dsp_oscil_reinit(voice->osc2, freq2, false);
    dsp_adsr_trigger_attack(voice->env1);
    dsp_adsr_trigger_attack(voice->env2);
}

/**
 * Stop a currently playing note. Simply triggers the release part of the envelope generator.
 * The active flag of the corresponding voice will be changed in `synth_process` when the
 * envelope generator has stopped.
 */
void synth_note_off(synth_t* synth, int note) {
    for (int i = 0; i < CONFIG_SYNTH_POLYPHONY; i++) {
        synth_voice_t* voice = &synth->state.voices[i];

        if (voice->note == note && voice->active) {
            dsp_adsr_trigger_release(voice->env1);
        }
    }
}

/**
 * Render next audio block. This calls the different DSP methods to synthesize the sound
 * and updates each voice's active flag based on its envelope generator status.
 */
void IRAM_ATTR synth_process(synth_t* synth, float* audio_buffer, size_t length) {
    // Mix next samples into the output buffer
    for (size_t i = 0; i < length; i += 2) {
        for (int j = 0; j < CONFIG_SYNTH_POLYPHONY; j++) {
            synth_voice_t* voice = &synth->state.voices[j];

            float sample2 = dsp_oscil_tick(voice->osc2, 0.0f)    * dsp_adsr_tick(voice->env2) * voice->fm_index_2_1;
            float sample1 = dsp_oscil_tick(voice->osc1, sample2) * dsp_adsr_tick(voice->env1) * synth->state.gain_staging;

            float left, right;
            float pan = dsp_oscil_tick(voice->lfo1, 0.0f) * 0.75f;
            dsp_pan_stereo(sample1, pan, &left, &right);

            audio_buffer[i    ] += left;
            audio_buffer[i + 1] += right;
        }
    }

    // Update voice status
    for (int j = 0; j < CONFIG_SYNTH_POLYPHONY; j++) {
        synth_voice_t* voice = &synth->state.voices[j];
        voice->active = voice->env1->state.status != DSP_ADSR_STOPPED;
    }
}
