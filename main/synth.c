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

#include <esp_log.h>
#include <stdlib.h>
#include <math.h>

static const char* TAG = "synth";       // Logging tag

#define WAVETABLE_LENGTH (512)
#define PI (3.14159265)

/**
 * State of the envelope generator, which in case of the amplitude envelope
 * is also the playing state of a note.
 */
typedef enum synth_voice_state {
    STOPPED,
    ATTACK,
    DECAY,
    SUSTAIN,
    RELEASE,
} synth_voice_state_t;

/**
 * Tone-generating voice.
 */
typedef struct synth_voice {
    int                 note;           // MIDI note number
    float               pan;            // Panorama
    float               velocity;       // Velocity
    synth_voice_state_t state;          // AEnv state
} synth_voice_t;

/**
 * Private properties of the synthesizer (pimpl idiom).
 */
typedef struct synth_pimpl {
    int            sample_rate;             // Sample rate in Hz
    int            polyphony;               // Number of voices
    synth_voice_t* voices;                  // Voices that actually create sound
    float*         wavetable;               // Wavetable for a simple sine wave
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
    synth->pimpl->wavetable   = malloc(WAVETABLE_LENGTH * sizeof(float));

    for (int i = 0; i < WAVETABLE_LENGTH; i++) {
        synth->pimpl->wavetable[i] = sin((2 * PI * i) / WAVETABLE_LENGTH);
    }

    ESP_LOGD(TAG, "Created synthesizer instance %p", synth);
    
    return synth;
}

/**
 * Free synthesizer instance.
 */
void synth_free(synth_t* synth) {
    ESP_LOGD(TAG, "Freeing synthesizer instance %p", synth);

    free(synth->pimpl->voices);
    free(synth->pimpl->wavetable);
    free(synth->pimpl);
    free(synth);

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