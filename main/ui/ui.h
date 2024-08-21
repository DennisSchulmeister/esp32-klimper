/*
 * Klimper: ESP32 I²S Synthesizer Test
 * © 2024 Dennis Schulmeister-Zimolong <dennis@wpvs.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

#pragma once

#include "common.h"

/**
 * Configuration parameters for the user interface.
 */
typedef struct {
    int renc_clk_io;                        // Rotary encoder clock (A)
    int renc_dir_io;                        // Rotary encoder direction (B)
    int btn_enter_io;                       // Rotary encoder button (ENTER)
    int btn_exit_io;                        // EXIT button
    int btn_home_io;                        // HOME (Main menu) button

    ui_menu_t main_menu;                    // Main menu
} ui_config_t;

/**
 * Initialize user interface. Sets up the GPIO for the hardware buttons, rotary encoder,
 * display etc., installs the required interrupt handlers and a dedicated UI task on core 0.
 *
 * @param config Configuration parameters (can be freed afterwards)
 */
void ui_init(ui_config_t* config);
