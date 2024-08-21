/*
 * Klimper: ESP32 I²S Synthesizer Test
 * © 2024 Dennis Schulmeister-Zimolong <dennis@wpvs.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 */

#include "ui.h"
#include "display.h"

#include <freertos/FreeRTOS.h>              // Common FreeRTOS
#include <freertos/task.h>                  // FreeRTOS task management
#include <freertos/queue.h>                 // FreeRTOS queue data structure
#include <driver/gpio.h>                    // ESP32 GPIO handling
#include <esp_log.h>                        // ESP_LOGx
#include <stdbool.h>                        // bool, true, false
#include <string.h>                         // memcpy, memset

static const char* TAG = "UI";              // Logging tag

static const TickType_t q_wait_delay    = 100 / portTICK_PERIOD_MS;             // Max. waiting time for the event queue
static const TickType_t debounce_button = 500 / portTICK_PERIOD_MS;             // Artificial delay to debounce the buttons
static const TickType_t debounce_rotary = 40  / portTICK_PERIOD_MS;             // Artificial delay to debounce the rotary encoder

ui_menu_t copy_menu                    (ui_menu_t* menu_orig);
uint64_t  gpio_bitmask                 (int io_pin);
uint64_t  calc_cmd_button_gpio_bitmask (ui_menu_t* menu);
void      add_cmd_button_isr_handlers  (ui_menu_t* menu);
void      menu_button_isr_handler      (void *arg);
void      cmd_button_isr_handler       (void *arg);
void      rotary_encoder_isr_handler   (void *arg);
void      debounce                     (TickType_t delay);
void      ui_task                      (void* parameters);

ui_config_t   config       = {};            // UI configuration (global copy)
QueueHandle_t event_queue  = {};            // FIFO queue for messages from ISR to UI task

typedef enum {
    DUMMY,
    ENTER,
    EXIT,
    HOME,
    INCREASE,
    DECREASE,
} ui_event_type_t;

typedef struct {                            // Event sent from ISR to UI task
    ui_event_type_t type;                   // Event type (e.g. execute command)
    ui_command_t*   cmd;                    // Associated command definition
    TickType_t      debounce;               // Debouncing time
} ui_event_t;

typedef struct {                            // Entry in the navigation history below
    ui_menu_t*    menu;                     // Either: Visible menu
unsigned int  selection;                // Selected menu item

    ui_command_t* cmd;                      // Or: Visible float parameter
} ui_screen_t;

#define MAX_SCREENS (10)

struct {                                    // Navigation history for the BACK button
    ui_screen_t q[MAX_SCREENS];             // Queue
    size_t i;                               // Index
} screen_queue = {};

/**
 * Initialize user interface
 */
void ui_init(ui_config_t* cfg) {
    config = *cfg;
    config.main_menu = copy_menu(&cfg->main_menu);

    screen_queue.q[0].menu      = &config.main_menu;
    screen_queue.q[0].selection = 0;

    event_queue = xQueueCreate(10, sizeof(ui_event_t));

    ui_event_t dummy_event = {.type = DUMMY};
    xQueueSend(event_queue, &dummy_event, 0);

    gpio_config_t gpio_config_input = {
        .intr_type    = GPIO_INTR_NEGEDGE,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = true,
        .pin_bit_mask = (
                gpio_bitmask(config.renc_clk_io)  | gpio_bitmask(config.renc_dir_io) |
                gpio_bitmask(config.btn_enter_io) | gpio_bitmask(config.btn_exit_io) | gpio_bitmask(config.btn_home_io) |
                calc_cmd_button_gpio_bitmask(&config.main_menu)
        ),
    };

    gpio_config(&gpio_config_input);
    gpio_install_isr_service(0);

    if (config.renc_clk_io  > 0) gpio_isr_handler_add(config.renc_clk_io,  rotary_encoder_isr_handler, NULL);
    if (config.btn_enter_io > 0) gpio_isr_handler_add(config.btn_enter_io, menu_button_isr_handler,    (void*) ENTER);
    if (config.btn_exit_io  > 0) gpio_isr_handler_add(config.btn_exit_io,  menu_button_isr_handler,    (void*) EXIT);
    if (config.btn_home_io  > 0) gpio_isr_handler_add(config.btn_home_io,  menu_button_isr_handler,    (void*) HOME);

    add_cmd_button_isr_handlers(&config.main_menu);

    xTaskCreatePinnedToCore(
        /* pvTaskCode    */ ui_task,
        /* pcName        */ "ui_task",
        /* uxStackDepth  */ 3584,
        /* pvParameters  */ NULL,
        /* uxPriority    */ 2,
        /* pxCreatedTask */ NULL,
        /* xCoreID       */ 0
    );
}

/**
 * Deep copy of the command array of a menu.
 */
ui_menu_t copy_menu(ui_menu_t* menu_orig) {
    ui_menu_t menu_copy = *menu_orig;

    if (menu_orig->n_commands < 1) {
        menu_copy.commands = NULL;
    } else {
        size_t size = menu_orig->n_commands * sizeof(ui_command_t);
        menu_copy.commands = malloc(size);
        memcpy(menu_copy.commands, menu_orig->commands, size);

        for (int i = 0; i < menu_orig->n_commands; i++) {
            ui_command_t cmd_orig = menu_orig->commands[i];
            ui_command_t cmd_copy = menu_copy.commands[i];

            cmd_copy.name = malloc(strlen(cmd_orig.name) + 1);
            strcpy(cmd_copy.name, cmd_orig.name);
            cmd_copy.sub_menu = copy_menu(&cmd_orig.sub_menu);
        }
    }

    return menu_copy;
}

/**
 * Set GPIO bit mask for a single pin. But only, of the pin number is positive.
 */
uint64_t gpio_bitmask(int io_pin) {
    if (io_pin <= 0) return 0ULL;
    return 1ULL << io_pin;
}

/**
 * Calculate GPIO input pin bit-mask for the additional command buttons of the menu.
 */
uint64_t calc_cmd_button_gpio_bitmask(ui_menu_t* menu) {
    uint64_t result = 0ULL;

    for (int i = 0; i < menu->n_commands; i++) {
        ui_command_t cmd = menu->commands[i];
        result |= gpio_bitmask(cmd.button_io);
        result |= calc_cmd_button_gpio_bitmask(&cmd.sub_menu);
    }

    return result;
}

/**
 * Register button interrupt handler for all hardware buttons of the menu.
 */
void add_cmd_button_isr_handlers(ui_menu_t* menu) {
    for (int i = 0; i < menu->n_commands; i++) {
        ui_command_t* cmd = &menu->commands[i];
        if (cmd->button_io > 0) gpio_isr_handler_add(cmd->button_io, cmd_button_isr_handler, (void*) cmd);
        add_cmd_button_isr_handlers(&cmd->sub_menu);
    }
}

/**
 * Interrupt handler for the menu navigation buttons ENTER and EXIT. Sends the corresponding
 * message to the UI task. Note that event ENTER without a command means to execute the currently
 * highlighted menu item.
 */
void IRAM_ATTR menu_button_isr_handler(void* arg) {
    ui_event_t event = {
        .type     = (ui_event_type_t) arg,
        .cmd      = NULL,
        .debounce = debounce_button,
    };

    BaseType_t higherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(event_queue, &event, &higherPriorityTaskWoken);
    portYIELD_FROM_ISR(higherPriorityTaskWoken);
}

/**
 * Interrupt handler for the additional buttons that directly execute a command.
 * Sends a message to the UI task to execute the selected command.
 */
void IRAM_ATTR cmd_button_isr_handler(void* arg) {
    ui_event_t event = {
        .type     = ENTER,
        .cmd      = (ui_command_t*) arg,
        .debounce = debounce_button,
    };

    BaseType_t higherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(event_queue, &event, &higherPriorityTaskWoken);
    portYIELD_FROM_ISR(higherPriorityTaskWoken);
}

/**
 * Interrupt handler for the rotary encoder. Decodes the direction and sends an appropriate
 * message to the UI task.
 **/
void IRAM_ATTR rotary_encoder_isr_handler(void* arg) {
    bool increase = gpio_get_level(config.renc_dir_io);

    ui_event_t event = {
        .type     = increase ? INCREASE : DECREASE,
        .debounce = debounce_rotary,
    };

    BaseType_t higherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(event_queue, &event, &higherPriorityTaskWoken);
    portYIELD_FROM_ISR(higherPriorityTaskWoken);
}

/**
 * Cheated debouncing of the hardware buttons and rotary encoder. This simply delays
 * the UI task and discards all GPIO events collected in between.
 */
void debounce(TickType_t delay) {
    vTaskDelay(delay);
    xQueueReset(event_queue);

}
/**
 * Background task for the actual UI logic. Responds to button presses and the rotary
 * encoder to update the LCD display and application parameters.
 */
void ui_task(void* parameters) {
    ui_event_t   event   = {};
    bool         refresh = false;
    ui_screen_t* screen  = &screen_queue.q[screen_queue.i];

    while (true) {
        if (!xQueueReceive(event_queue, &event, q_wait_delay)) continue;
        if (event.debounce) debounce(event.debounce);

        switch (event.type) {
            case DUMMY:
                ESP_LOGD(TAG, "Handle DUMMY event");
                refresh = true;
                break;

            case ENTER:
                ESP_LOGD(TAG, "Handle ENTER event");

                ui_command_t* cmd = event.cmd;

                if (event.cmd == NULL && screen->menu && screen->menu->commands) {
                    cmd = &screen->menu->commands[screen->selection];
                }

                if (!cmd) break;
                if (cmd->cb.execute) cmd->cb.execute(cmd->cb.arg);

                if (cmd->param.value || cmd->sub_menu.commands) {
                    if (screen_queue.i >= MAX_SCREENS - 1) screen_queue.i = 0;

                    screen_queue.i++;
                    screen = &screen_queue.q[screen_queue.i];
                    memset(screen, 0, sizeof(ui_screen_t));

                    if (cmd->param.value) screen->cmd = cmd;
                    else if (cmd->sub_menu.commands) screen->menu = &cmd->sub_menu;
                }

                refresh = true;
                break;

            case EXIT:
                ESP_LOGD(TAG, "Handle EXIT event");

                if (screen_queue.i > 0) {
                    screen_queue.i--;
                    refresh = true;
                }

                break;

            case HOME:
                ESP_LOGD(TAG, "Handle HOME event");

                screen_queue.i = 0;
                refresh = true;
                break;

            case INCREASE:
                ESP_LOGD(TAG, "Handle INCREASE event");

                if (screen->cmd && screen->cmd->param.value) {
                    if (*screen->cmd->param.value < screen->cmd->param.max) {
                        *screen->cmd->param.value += screen->cmd->param.step;
                        if (screen->cmd->cb.value) screen->cmd->cb.value(screen->cmd->cb.arg);
                        refresh = true;
                    }
                } else if (screen->menu) {
                    if (screen->selection < (screen->menu->n_commands - 1)) screen->selection++;
                    else screen->selection = 0;
                    refresh = true;
                }

                break;

            case DECREASE:
                ESP_LOGD(TAG, "Handle DECREASE event");

                if (screen->cmd && screen->cmd->param.value) {
                    if (*screen->cmd->param.value > screen->cmd->param.min) {
                        *screen->cmd->param.value -= screen->cmd->param.step;
                        if (screen->cmd->cb.value) screen->cmd->cb.value(screen->cmd->cb.arg);
                        refresh = true;
                    }
                } else if (screen->menu) {
                    if (screen->selection > 0) screen->selection--;
                    else screen->selection = screen->menu->n_commands - 1;
                    refresh = true;
                }

                break;
        }

        if (!refresh) continue;
        ESP_LOGD(TAG, "Refresh display");

        refresh = false;
        screen  = &screen_queue.q[screen_queue.i];

        if (screen->menu) {
            ui_display_show_menu(screen->menu, screen->selection);
        } else if (screen->cmd->param.value) {
            ui_display_show_param(screen->cmd->name, *screen->cmd->param.value);
        }
    }
}
