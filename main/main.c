/*
 * Klimper: ESP32 I²S Synthesizer Test
 * © 2024 Dennis Schulmeister-Zimolong <dennis@wpvs.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

#include <freertos/FreeRTOS.h>              // Common FreeRTOS (must come first)
#include <freertos/task.h>                  // TaskHandle_t
#include <portmacro.h>                      // spinlock_t
#include <esp_log.h>                        // ESP_LOGx
#include <stdbool.h>                        // bool, true, false

#include "driver/audiohw.h"                 // audiohw_xyz
#include "driver/midiio.h"                  // midiio_xyz
#include "dsp/wavetable.h"                  // dsp_wavetable_xyz
#include "ui/ui.h"                          // ui_xyz
#include "synth.h"                          // synth_xyz
#include "sequencer.h"                      // sequencer_xyz
#include "midi.h"                           // midi_xyz

// DSP task: Called by the audio hardware to generate new audio
void dsp_task(void* parameters);

static TaskHandle_t dsp_task_handle = NULL;

// DSP Stuff
static float dsp_buffer[CONFIG_AUDIO_N_SAMPLES_BUFFER] = {};

static dsp_wavetable_t* wavetable = NULL;
static synth_t*         synth     = NULL;
static sequencer_t*     sequencer = NULL;

void cb_sequencer_start_stop() { sequencer_set_bpm(sequencer, sequencer->params.bpm); }
void cb_sequencer_set_bpm()    { sequencer_set_running(sequencer, !sequencer->params.running); }

/**
 * Application entry point
 */
void app_main(void) {
    // Create wavetable. Note that we are using cosine instead of sine so that we still
    // get values at the Nyquist frequency (sample rate * 0.5). Because there the wavetable
    // will only be read at the first and middle position which with a sine would be zero.
    // See "The Audio Programming Book", p. 299
    wavetable = dsp_wavetable_get(DSP_WAVETABLE_COS);

    // Create synthesizer
    float fm_ratios[] = {1.0f};

    synth_config_t synth_config = {
        .volume    = 1.0,
        .wavetable = wavetable,

        .env1 = {
            .attack  = 0.1f,
            .peak    = 1.0f,
            .decay   = 0.3f,
            .sustain = 0.5f,
            .release = 0.5f,
        },

        .env2 = {
            .attack  = 0.5f,
            .peak    = 1.0f,
            .decay   = 0.0f,
            .sustain = 1.0f,
            .release = 0.2f,
        },

        .fm = {
            .n_ratios = sizeof(fm_ratios) / sizeof(float),
            .ratios   = fm_ratios,
            .index_min = 0.25f,
            .index_max = 0.75f,
        },
    };

    synth = synth_new(&synth_config);

    // Create sequencer
    int notes[] = {60, 62, 64, 65, 67, 69, 71, 72};

    sequencer_config_t sequencer_config = {
        .synth   = synth,
        .n_notes = sizeof(notes) / sizeof(int),
        .notes   = notes,
    };

    sequencer = sequencer_new(&sequencer_config);
    sequencer_set_bpm(sequencer, 80);
    sequencer_set_running(sequencer, true);

    // Initialize audio hardware and mixer task. This must happen after the DSP
    // and synthesizer to avoid using the DSP objects before they are initialized.
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
        .sample_rate = CONFIG_AUDIO_SAMPLE_RATE,
        .n_samples   = CONFIG_AUDIO_N_SAMPLES_BUFFER,
        .i2s_mck_io  = CONFIG_I2S_MCLK_GPIO,
        .i2s_lrc_io  = CONFIG_I2S_WSEL_GPIO,
        .i2s_bck_io  = CONFIG_I2S_BCLK_GPIO,
        .i2s_dout_io = CONFIG_I2S_DOUT_GPIO,
        .dsp_task    = dsp_task_handle,
    };

    audiohw_init(&audiohw_config);

    // TODO: Initialize MIDI I/O.

    // Initialize (hardware) user interface
    ui_command_t main_commands[] = {
        {
            .name       = "Start/Stop",
            .button_io  = CONFIG_UI_BTN_SEQ_START_GPIO,
            .cb.execute = cb_sequencer_start_stop,
        },
        {
            .name      = "Sequencer BPM",
            .button_io = CONFIG_UI_BTN_SEQ_BPM_GPIO,
            .cb.value  = cb_sequencer_set_bpm,
            .param = {
                .value = &sequencer->params.bpm,
                .min   = 1.0f,
                .max   = 280.0f,
                .step  = 1.0f,
            },
        },
        {
            .name      = "Master Volume",
            .button_io = CONFIG_UI_BTN_SYNTH_VOLUME_GPIO,
            .param = {
                .value = &synth->params.volume,
                .min   = 0.0f,
                .max   = 1.0f,
                .step  = 0.1f,
            },
        },
    };

    ui_config_t ui_config = {
        .renc_clk_io  = CONFIG_UI_RENC_CLK_GPIO,
        .renc_dir_io  = CONFIG_UI_RENC_DIR_GPIO,
        .btn_enter_io = CONFIG_UI_BTN_ENTER,
        .btn_exit_io  = CONFIG_UI_BTN_EXIT,
        .btn_home_io  = CONFIG_UI_BTN_HOME,

        .main_menu = {
            .n_commands = sizeof(main_commands) / sizeof(ui_command_t),
            .commands   = main_commands,
        },
    };

    ui_init(&ui_config);
}

/**
 * Background task woken up by the audio hardware whenever new audio data
 * is needed. The DSP task then calls the actual DSP functions to produce
 * a new chunk of audio data.
 */
void dsp_task(void* parameters) {
    while (true) {
        audiohw_buffer_t tx_buffer = *(audiohw_buffer_t*) ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        for (size_t i = 0; i < CONFIG_AUDIO_N_SAMPLES_BUFFER; i++) {
            dsp_buffer[i] = 0.0f;
        }

        for (size_t n_samples_processed = 0; n_samples_processed < CONFIG_AUDIO_N_SAMPLES_BUFFER; n_samples_processed += CONFIG_AUDIO_N_SAMPLES_CYCLE) {
            sequencer_process(sequencer, CONFIG_AUDIO_N_SAMPLES_CYCLE);
            synth_process(synth, dsp_buffer + n_samples_processed, CONFIG_AUDIO_N_SAMPLES_CYCLE);
        }

        for (int i = 0; (i < CONFIG_AUDIO_N_SAMPLES_BUFFER) && (i < tx_buffer.size); i++) {
            // NOTE: The assumption here is that the buffer provided by the audio hardware has the
            // same size as the DSP buffer used here. There is a maximum limit of how big a DMA buffer
            // can be on the ESP32. But we assume that as real-time application we are below that limit.
            float sample = dsp_buffer[i];

            if (sample < -1.0f) sample = -1.0f;
            else if (sample > 1.0f) sample = 1.0f;

            tx_buffer.data[i] = (int16_t) (roundf(32767.0f * sample));
        }
    }
}
