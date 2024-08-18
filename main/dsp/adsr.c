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
 */
static inline void dsp_adsr_recalc_increments(dsp_adsr_t* adsr) {
    float delta, nsmpl;
    static int nsmpl_min = CONFIG_AUDIO_SAMPLE_RATE * 0.01f;

    delta = adsr->envelope.attack.value - adsr->envelope.release.value;
    nsmpl = CONFIG_AUDIO_SAMPLE_RATE * adsr->envelope.attack.duration;
    if (nsmpl < nsmpl_min) nsmpl = nsmpl_min;
    adsr->envelope.attack.increment = delta / nsmpl;

    delta = adsr->envelope.decay.value - adsr->envelope.attack.value;
    nsmpl = CONFIG_AUDIO_SAMPLE_RATE * adsr->envelope.decay.duration;
    if (nsmpl < nsmpl_min) nsmpl = nsmpl_min;
    adsr->envelope.decay.increment = delta / nsmpl;

    delta = adsr->envelope.release.value - adsr->envelope.sustain;
    nsmpl = CONFIG_AUDIO_SAMPLE_RATE * adsr->envelope.release.duration;
    if (nsmpl < nsmpl_min) nsmpl = nsmpl_min;
    adsr->envelope.release.increment = delta / nsmpl;
}

/**
 * Shortcut to set all values at once.
 */
void dsp_adsr_set_values(dsp_adsr_t* adsr, dsp_adsr_values_t* values) {
    adsr->envelope.attack.value     = values->peak;
    adsr->envelope.attack.duration  = values->attack;

    adsr->envelope.peak             = values->peak;

    adsr->envelope.decay.value      = values->sustain;
    adsr->envelope.decay.duration   = values->decay;

    adsr->envelope.sustain          = values->sustain;

    adsr->envelope.release.value    = 0.0f;
    adsr->envelope.release.duration = values->release;

    dsp_adsr_recalc_increments(adsr);
}

/**
 * Set attack time.
 */
void dsp_adsr_set_attack(dsp_adsr_t* adsr, float duration) {
    adsr->envelope.attack.value    = adsr->envelope.peak;
    adsr->envelope.attack.duration = duration;

    dsp_adsr_recalc_increments(adsr);
}

/**
 * Set peak level.
 */
void dsp_adsr_set_peak(dsp_adsr_t* adsr, float level) {
    adsr->envelope.peak = level;
    adsr->envelope.attack.value = level;

    dsp_adsr_recalc_increments(adsr);
}

/**
 * Set decay time.
 */
void dsp_adsr_set_decay(dsp_adsr_t* adsr, float duration) {
    adsr->envelope.decay.value    = adsr->envelope.sustain;
    adsr->envelope.decay.duration = duration;

    dsp_adsr_recalc_increments(adsr);
}

/**
 * Set sustain level.
 */
void dsp_adsr_set_sustain(dsp_adsr_t* adsr, float level) {
    adsr->envelope.sustain = level;
    adsr->envelope.decay.value = level;

    dsp_adsr_recalc_increments(adsr);
}


/**
 * Set release time.
 */
void dsp_adsr_set_release(dsp_adsr_t* adsr, float duration) {
    adsr->envelope.release.value    = 0.0f;
    adsr->envelope.release.duration = duration;

    dsp_adsr_recalc_increments(adsr);
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
