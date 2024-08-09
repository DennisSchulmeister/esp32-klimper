/*
 * ESP32 I²S Synthesizer Test / µDSP Library
 * © 2024 Dennis Schulmeister-Zimolong <dennis@wpvs.de>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

#include "adsr.h"

#include <stdlib.h>                         // calloc(), malloc(), free()

/**
 * Create a new ADSR envelope generator.
 */
dsp_adsr_t* dsp_adsr_new() {
    return calloc(1, sizeof(dsp_adsr_t));
}

/**
 * Free memory of a given ADSR envelope generator.
 */
void dsp_adsr_free(dsp_adsr_t* adsr) {
    free(adsr);
}

/**
 * Recalculate the increment values of all breakpoints after one breakpoint
 * has been changed. In theory, since the attack and release levels are fixed
 * at 1.0 and 0.0, not all values must be recalculated with every change.
 * But let's keep it simple and clean here and foresee the option of a more
 * sophisticated envelope generator in futre.
 * 
 * @param adsr ADSR envelope generator instance
 * @param sample_rate Sample rate in Hz
 */
inline void dsp_adsr_recalc_increments(dsp_adsr_t* adsr, int sample_rate) {
    float delta, nsmpl;

    delta = adsr->envelope.attack.value - adsr->envelope.release.value;
    nsmpl = sample_rate * adsr->envelope.attack.duration;
    adsr->envelope.attack.increment = delta / nsmpl;

    delta = adsr->envelope.decay.value - adsr->envelope.attack.value;
    nsmpl = sample_rate * adsr->envelope.decay.duration;
    adsr->envelope.decay.increment = delta / nsmpl;

    delta = adsr->envelope.release.value - adsr->envelope.sustain;
    nsmpl = sample_rate * adsr->envelope.release.duration;
    adsr->envelope.release.increment = delta / nsmpl;
}

/**
 * Set attack time.
 */
void dsp_adsr_set_attack(dsp_adsr_t* adsr, int sample_rate, float duration) {
    adsr->envelope.attack.value    = 1.0;
    adsr->envelope.attack.duration = duration;

    dsp_adsr_recalc_increments(adsr, sample_rate);
}

/**
 * Set decay time.
 */
void dsp_adsr_set_decay(dsp_adsr_t* adsr, int sample_rate, float duration) {
    adsr->envelope.decay.value    = adsr->envelope.sustain;
    adsr->envelope.decay.duration = duration;

    dsp_adsr_recalc_increments(adsr, sample_rate);
}

/**
 * Set sustain level.
 */
void dsp_adsr_set_sustain(dsp_adsr_t* adsr, int sample_rate, float level) {
    adsr->envelope.sustain = level;

    dsp_adsr_recalc_increments(adsr, sample_rate);
}


/**
 * Set release time.
 */
void dsp_adsr_set_release(dsp_adsr_t* adsr, int sample_rate, float duration) {
    adsr->envelope.release.value    = 0.0;
    adsr->envelope.release.duration = duration;

    dsp_adsr_recalc_increments(adsr, sample_rate);
}

/**
 * Start the envelope generator.
 */
void dsp_adsr_trigger_attack(dsp_adsr_t* adsr) {
    adsr->state.status = DSP_ADSR_ATTACK;
}

/**
 * Finish the envelope and then stop once it reaches or crosses zero.
 */
void dsp_adsr_trigger_release(dsp_adsr_t* adsr) {
    adsr->state.status = DSP_ADSR_RELEASE;
}

/**
 * Calculate next sample.
 */
extern inline float dsp_adsr_tick(dsp_adsr_t* adsr) {
    float value = adsr->state.value;

    switch (adsr->state.status) {
        case DSP_ADSR_ATTACK:
            adsr->state.value += adsr->envelope.attack.increment;

            if (adsr->state.value >= adsr->envelope.attack.value) {
                adsr->state.status = DSP_ADSR_DECAY;
                adsr->state.value  = adsr->envelope.attack.value;
            }

            break;
        case DSP_ADSR_DECAY:
            adsr->state.value += adsr->envelope.decay.increment;

            if (adsr->state.value <= adsr->envelope.decay.value) {
                adsr->state.status = DSP_ADSR_SUSTAIN;
                adsr->state.value  = adsr->envelope.decay.value;
            }

            break;
        case DSP_ADSR_RELEASE:
            adsr->state.value += adsr->envelope.release.increment;

            if (adsr->state.value <= 0) {
                adsr->state.status = DSP_ADSR_STOPPED;
                adsr->state.value  = 0;
            }

            break;
        default:
            // Suppress compiler error: Enumeration value not handled in switch
            break;
    }

    return value;
}