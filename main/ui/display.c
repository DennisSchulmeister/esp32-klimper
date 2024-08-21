/*
 * Klimper: ESP32 I²S Synthesizer Test
 * © 2024 Dennis Schulmeister-Zimolong <dennis@wpvs.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

#include "sdkconfig.h"                      // Needed here to access config options

// Include configured implementation
#if CONFIG_UI_LCD_TYPE_CONSOLE
    #include "display/console.c"
#elif CONFIG_UI_LCD_TYPE_1602P
    #include "display/1602p.c"
#else
    #error No display implementation included. Please configure project!
#endif
