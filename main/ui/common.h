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

#include <stdbool.h>

/**
 * Custom callback function for button press.
 */
typedef void (*ui_callback_ptr)(void* arg);

struct ui_command;

/**
 * Definition if a menu, simple consisting of an arbitrary number of commands.
 */
typedef struct {
    int n_commands;                         // Number of commands
    struct ui_command* commands;            // Commands (will be copied)
} ui_menu_t;

/**
 * Definition of a UI command. This is a very flexible structure allowing to define
 * functions (commands) that can be executed via a dedicated hardware button and
 * a simple command menu on the display.
 *
 * Commands will appear in the menu in the order they are defined. If `button_io` is
 * set to a positive number, the command can be directly accessed with a hardware button.
 * In both cases the `cb.exeucte` callback will be called, when defined.
 *
 * If a command is linked to a numeric parameter, that parameter can be changed with the
 * rotary encoder after the command has been activated. The command name and the current
 * value will be shown on the LCD display. If `cb.value` is set that callback will be
 * executed after every value change caused by the UI (e.g. to recalculate envelope
 * factors etc.).
 *
 * Commands can be nested to build a hierarchical menu structure. In this case the parent
 * command would typically not contain any callbacks or numeric value but just a list of
 * sub-commands presented in a new menu.
 */
typedef struct ui_command {
    char* name;                             // Parameter name
    int   button_io;                        // GPIO pin of the hardware button (optional)
    bool  hidden;                           // Don't show command in the menu

    struct {                                // Numeric parameter (optional)
        volatile float* value;              // Value changeable via rotary encoder
        float  min;                         // Minimum allowed value
        float  max;                         // Maximum allowed value
        float  step;                        // Step size (delta for one encoder tick)
    } param;

    struct {                                // Callback functions (optional)
        ui_callback_ptr execute;            // Callback when the function is selected (optional)
        ui_callback_ptr value;              // Callback when the value has been changed (optional)
        void* arg;                          // User argument for the callback functions
    } cb;

    ui_menu_t sub_menu;                     // Sub-menu (optional)
} ui_command_t;
