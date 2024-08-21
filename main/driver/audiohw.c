/*
 * Klimper: ESP32 I²S Synthesizer Test
 * © 2024 Dennis Schulmeister-Zimolong <dennis@wpvs.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

#include "audiohw.h"

#include <driver/i2s_std.h>                     // i2s…()
#include <esp_log.h>                            // ESP_LOGx

static const char* TAG = "audiohw";             // Logging tag

static bool i2s_isr_on_sent(i2s_chan_handle_t handle, i2s_event_data_t *event, void *user_ctx);

static TaskHandle_t dsp_task;                   // DSP task to notify
static audiohw_buffer_t audiohw_buffer = {};    // Buffer information passed to mixer task
static i2s_chan_handle_t tx_handle;             // I²S Transmit Handle

/**
 * Initialize audio hardware layer.
 */
void audiohw_init(audiohw_config_t* config) {
    dsp_task = config->dsp_task;

    i2s_chan_config_t channel_config = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    channel_config.auto_clear_before_cb = true;
    channel_config.auto_clear_after_cb  = true;
    channel_config.dma_desc_num         = 2;                            // Two buffers
    channel_config.dma_frame_num        = config->n_samples / 2;        // Samples per buffer
    i2s_new_channel(&channel_config, &tx_handle, NULL);

    i2s_std_config_t standard_config = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(config->sample_rate),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),

        .gpio_cfg = {
            .mclk = config->i2s_mck_io,
            .bclk = config->i2s_bck_io,
            .ws   = config->i2s_lrc_io,
            .dout = config->i2s_dout_io,
            .din  = I2S_GPIO_UNUSED,

            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };

    i2s_channel_init_std_mode(tx_handle, &standard_config);

    i2s_event_callbacks_t callbacks = {
        .on_sent = i2s_isr_on_sent,
    };

    i2s_channel_register_event_callback(tx_handle, &callbacks, NULL);
    i2s_channel_enable(tx_handle);

    // Print buffer and latency information
    i2s_chan_info_t channel_info = {};
    i2s_channel_get_info(tx_handle, &channel_info);

    if (channel_config.dma_desc_num            != 0
    && standard_config.slot_cfg.slot_mode      != 0
    && standard_config.slot_cfg.data_bit_width != 0
    && standard_config.clk_cfg.sample_rate_hz  != 0) {
        uint32_t n_frames = channel_info.total_dma_buf_size / channel_config.dma_desc_num / standard_config.slot_cfg.slot_mode / standard_config.slot_cfg.data_bit_width * 8;
        float latency_ms = 1000.0 * n_frames / standard_config.clk_cfg.sample_rate_hz;

        ESP_LOGI(TAG, "I²S Sample Rate............: %i Hz", CONFIG_AUDIO_SAMPLE_RATE);
        ESP_LOGI(TAG, "I²S Samples per Buffer.....: %i (containing two audio channels)", CONFIG_AUDIO_N_SAMPLES_BUFFER);
        ESP_LOGI(TAG, "I²S Resulting DMA Buffer...: %" PRIu32 " Bytes", channel_info.total_dma_buf_size);
        ESP_LOGI(TAG, "I²S Resulting Latency......: %f ms", latency_ms);
    } else {
        // That should not happen!
        ESP_LOGE(TAG, "Cannot calculate I2S latency due to division by zero.");
    }
}

/**
 * ISR being triggered automatically when new audio data must be created.
 */
static IRAM_ATTR bool i2s_isr_on_sent(i2s_chan_handle_t handle, i2s_event_data_t *event, void *user_ctx) {
    audiohw_buffer.data = event->dma_buf;
    audiohw_buffer.size = event->size;

    BaseType_t higherPriorityTaskWoken = pdFALSE;

    xTaskNotifyFromISR(
        /* xTaskToNotify            */ dsp_task,
        /* ulValue                  */ (uint32_t) &audiohw_buffer,
        /* eAction                  */ eSetValueWithOverwrite,
        /* xHigherPriorityTaskWoken */ &higherPriorityTaskWoken
    );

    // See: https://www.freertos.org/RTOS_Task_Notification_As_Binary_Semaphore.html
    // Trigger immediate context switch if needed to return to the highest priority task
    portYIELD_FROM_ISR(higherPriorityTaskWoken);

    return false;
}
