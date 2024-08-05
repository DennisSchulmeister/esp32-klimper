/*
 * ESP32 I²S Synthesizer Test
 * © 2024 Dennis Schulmeister-Zimolong <dennis@wpvs.de>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <portmacro.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <stddef.h>

#include "audiohw.h"
#include "synth.h"
#include "sequencer.h"

// DSP task: Called by the audio hardware to generate new audio
void dsp_task(void* parameters);
TaskHandle_t dsp_task_handle = NULL;
spinlock_t dsp_task_spinlock = portMUX_INITIALIZER_UNLOCKED;

// DSP Stuff
#define SAMPLE_RATE      (44100)
#define N_SAMPLES_BUFFER (880)              // 440 Frames (two samples each for stereo) = 10ms audio latency
#define N_SAMPLES_CYCLE  (220)              // 10ms / 4 = 2ms timing accuracy (must divide the sample buffer by an integer!)

float dsp_buffer[N_SAMPLES_BUFFER] = {};

synth_t* synth = NULL;
sequencer_t* sequencer = NULL;

/**
 * Application entry point
 */
void app_main(void) {
    // Create synthesizer
    synth_config_t synth_config = {
        .sample_rate = SAMPLE_RATE,
        .polyphony   = 8,
    };

    synth = synth_new(&synth_config);

    synth->params.amplitude = 0.2;
    synth->params.attack    = 0.1;
    synth->params.decay     = 0.3;
    synth->params.sustain   = 0.5;
    synth->params.release   = 0.5;

    // Create sequencer
    sequencer_config_t sequencer_config = {
        .sample_rate = SAMPLE_RATE,
        .synth       = synth,
    };

    sequencer = sequencer_new(&sequencer_config);

    sequencer->params.bpm     = 80;
    sequencer->params.n_notes = 8;
    sequencer->params.notes   = calloc(8, sizeof(int));

    sequencer->params.notes[0] = 48;
    sequencer->params.notes[1] = 50;
    sequencer->params.notes[2] = 52;
    sequencer->params.notes[3] = 53;
    sequencer->params.notes[4] = 55;
    sequencer->params.notes[5] = 57;
    sequencer->params.notes[6] = 59;
    sequencer->params.notes[7] = 60;

    // Initialize audio hardware and mixer task. This must happen last to avoid
    // using the DSP objects before they are initialized.
    xTaskCreatePinnedToCore(
        /* pvTaskCode    */ dsp_task,
        /* pcName        */ "dsp_task",
        /* uxStackDepth  */ 3584,
        /* pvParameters  */ NULL,
        /* uxPriority    */ configMAX_PRIORITIES - 1,
        /* pxCreatedTask */ &dsp_task_handle,
        /* xCoreID       */ 1
    );

    audiohw_config_t audiohw_config = {
        .sample_rate = SAMPLE_RATE,
        .n_samples   = N_SAMPLES_BUFFER,
        .i2s_lrc_io  = GPIO_NUM_25,
        .i2s_bck_io  = GPIO_NUM_26,
        .i2s_dout_io = GPIO_NUM_27,
        .dsp_task  = dsp_task_handle,
        .spinlock    = &dsp_task_spinlock,
    };

    audiohw_init(&audiohw_config);
}

/**
 * Background task woken up by the audio hardware whenever new audio data
 * is needed. The DSP task then calls the actual DSP functions to produce
 * a new chunk of audio data.
 */
void dsp_task(void* parameters) {
    while (true) {
        audiohw_buffer_t* tx_buffer = (audiohw_buffer_t*) ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        for (size_t n_samples_processed = 0; n_samples_processed < N_SAMPLES_BUFFER; n_samples_processed += N_SAMPLES_CYCLE) {
            sequencer_process(sequencer, N_SAMPLES_CYCLE);
            synth_process(synth, dsp_buffer + n_samples_processed, N_SAMPLES_CYCLE);
        }

        taskENTER_CRITICAL(&dsp_task_spinlock);
        
        for (int i = 0; (i < N_SAMPLES_BUFFER) && (i < tx_buffer->size); i++) {
            // NOTE: The assumption here is that the buffer provided by the audio hardware has the
            // same size as the DSP buffer used here. There is a maximum limit of how big a DMA buffer
            // can be on the ESP32. But we assume that as real-time applications we are below that limit.
            tx_buffer->data[i] = (int16_t) (32768 * dsp_buffer[i]);
        }

        taskEXIT_CRITICAL(&dsp_task_spinlock);
    }
}