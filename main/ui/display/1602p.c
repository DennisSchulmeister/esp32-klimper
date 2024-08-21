/*
 * Klimper: ESP32 I²S Synthesizer Test
 * © 2024 Dennis Schulmeister-Zimolong <dennis@wpvs.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

////////////////////////////////////////////////////////////
// LCD-Display 1602 with HD44780 4-bit parallel interface //
////////////////////////////////////////////////////////////

#include "../display.h"
#include "../common.h"

#include <hd44780.h>                        // ESP-IDF-LIB: hd44780_xyz
#include <stdio.h>                          // printf
#include <string.h>                         // strncpy, strlen
#include <ctype.h>                          // toupper
#include <stdbool.h>                        // bool, true, false

hd44780_t lcd = {
    .write_cb  = NULL,
    .lines     = 2,
    .backlight = true,
    .font      = HD44780_FONT_5X10,

    .pins = {
        .rs = CONFIG_UI_LCD_1602P_RS_GPIO,
        .e  = CONFIG_UI_LCD_1602P_E_GPIO,
        .d4 = CONFIG_UI_LCD_1602P_D4_GPIO,
        .d5 = CONFIG_UI_LCD_1602P_D5_GPIO,
        .d6 = CONFIG_UI_LCD_1602P_D6_GPIO,
        .d7 = CONFIG_UI_LCD_1602P_D7_GPIO,
        .bl = HD44780_NOT_USED,
    },
};

/* Initialize display implementation.  */
void ui_display_init() {
    hd44780_init(&lcd);
    hd44780_control(&lcd, true, false, false);
    hd44780_clear(&lcd);
}

/* Output a line of text. */
void print_line(int line, char* text) {
    if (line < 1) line = 1;
    else if (line > 2) line = 2;
    line--;

    char buffer[17] = {};
    strncpy(buffer, text, 16);
    buffer[16] = '\0';

    int p = (16 - strlen(buffer)) / 2;
    int col = 0;

    for (; col < p; col++) {
        hd44780_gotoxy(&lcd, col, line);
        hd44780_putc(&lcd, ' ');
    }

    hd44780_gotoxy(&lcd, col + 1, line);
    hd44780_puts(&lcd, text);
}

/* Show menu items */
void ui_display_show_menu(ui_menu_t* menu, unsigned int selection) {
    hd44780_clear(&lcd);

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
    hd44780_clear(&lcd);

    print_line(1, name);

    char buffer[32] = {};
    sprintf(buffer, "% f", value);
    print_line(2, buffer);
}
