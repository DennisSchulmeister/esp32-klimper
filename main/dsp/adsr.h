/*
 * ESP32 I²S Synthesizer Test / µDSP Library
 * © 2024 Dennis Schulmeister-Zimolong <dennis@wpvs.de>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

#pragma once

/**
 * Status of the envelope generator, defined as whether it is stopped or which segment
 * of the envelope is currently being generated.
 */
typedef enum {
    DSP_ADSR_STOPPED,
    DSP_ADSR_ATTACK,
    DSP_ADSR_DECAY,
    DSP_ADSR_SUSTAIN,
    DSP_ADSR_RELEASE,
} dsp_adsr_status_t;

/**
 * Breakpoint value converted to samples
 */
typedef struct {
    float value;                            // Target value
    float duration;                         // Duration in seconds
    float increment;                        // Value increment per sample rate tick
} dsp_adsr_breakpoint_t;

/**
 * ADSR envelope generator
 */
typedef struct {
    struct {
        dsp_adsr_breakpoint_t attack;       // Attack time
        dsp_adsr_breakpoint_t decay;        // Decay time
        dsp_adsr_breakpoint_t release;      // Release time
        float                 sustain;      // Sustain level
    } envelope;

    struct {
        dsp_adsr_status_t status;           // Currently active segment
        float             value;            // Next value
    } state;
} dsp_adsr_t;

/**
 * Create a new ADSR envelope generator and initialize it with zero values.
 * @returns Pointer to the new ADSR envelope generator
 */
dsp_adsr_t* dsp_adsr_new();

/**
 * Free memory of a given ADSR envelope generator.
 * @param adsr ADSR envelope generator
 */
void dsp_adsr_free(dsp_adsr_t* adsr);

/**
 * Set attack time.
 * 
 * @param adsr ADSR envelope generator
 * @param sample_rate Sample rate in Hz
 * @param duration Duration in seconds
 */
void dsp_adsr_set_attack(dsp_adsr_t* adsr, int sample_rate, float duration);

/**
 * Set decay time.
 * 
 * @param adsr ADSR envelope generator
 * @param sample_rate Sample rate in Hz
 * @param duration Duration in seconds
 */
void dsp_adsr_set_decay(dsp_adsr_t* adsr, int sample_rate, float duration);

/**
 * Set sustain level.
 * 
 * @param adsr ADSR envelope generator
 * @param sample_rate Sample rate in Hz
 * @param level Sustain level
 */
void dsp_adsr_set_sustain(dsp_adsr_t* adsr, int sample_rate, float level);

/**
 * Set release time.
 * 
 * @param adsr ADSR envelope generator
 * @param sample_rate Sample rate in Hz
 * @param duration Duration in seconds
 */
void dsp_adsr_set_release(dsp_adsr_t* adsr, int sample_rate, float duration);

/**
 * Start the envelope generator to generate the atack/decay/sustain part.
 * @param adsr ADSR envelope generator
 */
void dsp_adsr_trigger_attack(dsp_adsr_t* adsr);

/**
 * Finish the envelope and then stop once it reaches or crosses zero.
 * @param adsr ADSR envelope generator
 */
void dsp_adsr_trigger_release(dsp_adsr_t* adsr);

/**
 * Calculate next sample.
 * @param adsr ADSR envelope generator
 */
inline float dsp_adsr_tick(dsp_adsr_t* adsr);