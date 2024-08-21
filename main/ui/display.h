/*
 * Klimper: ESP32 I²S Synthesizer Test
 * © 2024 Dennis Schulmeister-Zimolong <dennis@wpvs.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

// LCD display implementation used by the UI module. This is a generic interface
// contract that can have multiple implementations, so that the project can be
// built with different display types.
//
// For each supported display type the following methods must be implemented.
// And a new option for `idf.py menuconfig` be defined in the file `Kconfig.projbuild`.
// If the display needs configuration (e.g. GPIO pin numbers), this must also be
// handled with `idf.py menuconfig`.
//
// Finally the file `display.c` must be extended to included the selected implementation.

#pragma once

#include "common.h"

/**
 * Initialize display implementation.
 */
void ui_display_init();

/**
 * Display the provided menu, possibly with a selected menu item.
 *
 * @param menu Menu commands to show
 * @param selection Index of the currently selected item
 */
void ui_display_show_menu(ui_menu_t* menu, unsigned int selection);

/**
 * Show a numeric parameter currently being changed with the rotary encoder.
 *
 * @param name Parameter name (e.g. "OSC1 AEnv Attack")
 * @param value Current value
 */
void ui_display_show_param(char* name, float value);
