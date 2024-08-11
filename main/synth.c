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
#include <float.h>                          // FLT_MAX
#include <stdlib.h>                         // calloc(), malloc(), free()
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
    synth_set_env1_values(synth, config->env1);

    synth->state.sample_rate = config->sample_rate;
    synth->state.polyphony   = config->polyphony;
    synth->state.voices      = calloc(config->polyphony, sizeof(synth_voice_t));

    for (int i = 0; i < config->polyphony; i++) {
        synth->state.voices[i].osc1 = dsp_oscil_new(config->wavetable);
        synth->state.voices[i].env1 = dsp_adsr_new();
        dsp_adsr_set_values(synth->state.voices[i].env1, config->sample_rate, &config->env1);
    }

    dsp_pan_init();

    ESP_LOGD(TAG, "Created synthesizer instance %p", synth);
    
    return synth;
}

/**
 * Free synthesizer instance.
 */
void synth_free(synth_t* synth) {
    ESP_LOGD(TAG, "Freeing synthesizer instance %p", synth);

    for (int i = 0; i < synth->state.polyphony; i++) {
        free(synth->state.voices[i].osc1);
        free(synth->state.voices[i].env1);
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
void synth_set_env1_values(synth_t* synth, dsp_adsr_values_t env1) {
    synth->params.env1 = env1;

    for (int i = 0; i < synth->state.polyphony; i++) {
        dsp_adsr_set_values(synth->state.voices[i].env1, synth->state.sample_rate, &env1);
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

    for (int i = 0; i < synth->state.polyphony; i++) {
        synth_voice_t* voice = &synth->state.voices[i];
        float          amp   = voice->velocity * voice->env1->state.value;

        if (voice->note == note) {
            retrigger = voice;
        }

        if (voice->active) {
            free_voice = voice;
        } else if (amp < steal_amp) {
            steal_voice = voice;
            steal_amp   = amp;
        }
    }

    // Trigger envelope generator
    synth_voice_t* voice = retrigger ? retrigger : free_voice ? free_voice : steal_voice;
    dsp_adsr_trigger_attack(voice->env1);

    // Set random pan and pan directory
    voice->pan = ((rand() % 512) / 255.0) - 1.0;
    voice->direction = ((rand() % 64) / 31.0) - 1.0;
}

/**
 * Stop a currently playing note. Simply triggers the release part of the envelope generator.
 * The active flag of the corresponding voice will be changed in `synth_process` when the
 * envelope generator has stopped.
 */
void synth_note_off(synth_t* synth, int note) {
    for (int i = 0; i < synth->state.polyphony; i++) {
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
void synth_process(synth_t* synth, float* audio_buffer, size_t length) {
    for (size_t i = 0; i < length; i += 2) {
        for (int j = 0; j < synth->state.polyphony; j++) {
            synth_voice_t* voice = &synth->state.voices[j];
            
            // Update control parameters of the voice
            voice->pan += voice->direction;
        
            if (voice->pan > 1.0 || voice->pan < -1.0) {
                voice->direction *= -1.0;
                voice->pan = (voice->pan > 1.0) ? 2.0 - voice->pan : -2.0 - voice->pan;
            }

            // Mix next sample into the output buffer
            float sample = dsp_oscil_tick(voice->osc1) * dsp_adsr_tick(voice->env1);
            float left, right;

            dsp_pan_stereo(sample, voice->pan, &left, &right);

            audio_buffer[i    ] += left;
            audio_buffer[i + 1] += right;

            // Update active flag of the voice
            voice->active = voice->env1->state.status != DSP_ADSR_STOPPED;
        }
    }
}