/*
 * Klimper: ESP32 I²S Synthesizer Test
 * © 2024 Dennis Schulmeister-Zimolong <dennis@wpvs.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

///////////////////////////////////////////////////////////////////
// Serial console dummy LCD display to simulate a basic 16x2 LCD //
///////////////////////////////////////////////////////////////////

#include "../display.h"
#include "../common.h"

#include <stdio.h>                          // printf
#include <string.h>                         // strncpy, strlen
#include <ctype.h>                          // toupper

/* Initialize display implementation.  */
void ui_display_init() {
    // Nothing to do
}

/* Move cursor to the home position and draw the empty display frame. */
void print_frame() {
    printf("\033[s");                       // Save cursor position
    printf("\033[H");                       // Move cursor to home position
    printf("\033[30;42m");                  // Color black on green

    printf("\n");
    printf("\033[K┏━━━━━━━━━━━━━━━━┓\n");   // Clear line and draw border
    printf("\033[K┃                ┃\n");
    printf("\033[K┃                ┃\n");
    printf("\033[K┗━━━━━━━━━━━━━━━━┛\n");

    printf("\033[0m");                      // Reset colors
    printf("\033[u");                       // Restore cursor position

    fflush(stdout);
}

/* Move cursor to the given line inside the display and print the text. */
void print_line(int nr, char* text) {
    if (nr < 1) nr = 1;
    else if (nr > 2) nr = 2;
    nr++;

    printf("\033[s");                       // Save cursor position
    printf("\033[%d;2H", nr + 1);           // Move cursor to the right line
    printf("\033[30;42m");                  // Color black on green

    char buffer[17];
    strncpy(buffer, text, 16);
    buffer[16] = '\0';

    int p = (16 - strlen(buffer)) / 2;
    printf("%*s%-*s", p, "", 16 - p, buffer);

    printf("\033[0m");                      // Reset colors
    printf("\033[u");                       // Restore cursor position

    fflush(stdout);
}

/* Show menu items */
void ui_display_show_menu(ui_menu_t* menu, unsigned int selection) {
    print_frame();

    char line1[32] = {};
    char line2[32] = {};

    if (menu->commands) {
        line1[0] = '>';
        strncpy(line1 + 1, menu->commands[selection].name, 30);  // 32 - 2 to have \0 at the end

        for (int i = 0; line1[i]; i++) {
            line1[i] = toupper(line1[i]);
        }

        if (menu->n_commands > 1) {
            strncpy(line2, menu->commands[(selection + 1) % menu->n_commands].name, 31);
        }
    }

    print_line(1, line1);
    print_line(2, line2);
}

/* Show numeric parameter. */
void ui_display_show_param(char* name, float value) {
    print_frame();
    print_line(1, name);

    char buffer[32] = {};
    sprintf(buffer, "% f", value);
    print_line(2, buffer);
}
