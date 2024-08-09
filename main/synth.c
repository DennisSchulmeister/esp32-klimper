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
 * Internal state of a tone-generating voice.
 */
typedef struct {
    int                  note;              // MIDI note number
    float                velocity;          // Velocity
    float                pan;               // Panorama value
    float                direction;         // Panorama change delta
    // synth_voice_status_t status;            // Amplitude envelope status
    // synth_osc_t          osc1;              // Oscillator 1 state
} synth_voice_t;

/**
 * Private properties of the synthesizer (pimpl idiom).
 */
typedef struct synth_pimpl {
    int            sample_rate;             // Sample rate in Hz
    int            polyphony;               // Number of voices
    synth_voice_t* voices;                  // Voices that actually create sound
    float          volume;                  // Overall volume
    // synth_adsr_t   aenv;                    // Amplitude envelope
} synth_pimpl_t;

/**
 * Create new synthesizer instance.
 */
synth_t* synth_new(synth_config_t* config) {
    ESP_LOGD(TAG, "Creating new synthesizer instance");

    synth_t* synth = calloc(1, sizeof(synth_t));

    synth->pimpl = calloc(1, sizeof(synth_pimpl_t));
    synth->pimpl->sample_rate = config->sample_rate;
    synth->pimpl->polyphony   = config->polyphony;
    synth->pimpl->voices      = calloc(config->polyphony, sizeof(synth_voice_t));

    ESP_LOGD(TAG, "Created synthesizer instance %p", synth);
    
    return synth;
}

/**
 * Free synthesizer instance.
 */
void synth_free(synth_t* synth) {
    ESP_LOGD(TAG, "Freeing synthesizer instance %p", synth);

    free(synth->pimpl->voices);
    free(synth->pimpl);
    free(synth);

}

/**
 * Set overall volume of the synthesizer.
 */
void synth_set_volume(synth_t* synth, float volume)  {
    ESP_LOGD(TAG, "Set volume to %f", volume);

    synth->pimpl->volume = volume;
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