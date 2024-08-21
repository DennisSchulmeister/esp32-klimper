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
#include <esp_timer.h>                      // esp_timer_get_time
#include <driver/gpio.h>                    // ESP32 GPIO handling
#include <esp_log.h>                        // ESP_LOGx
#include <stdbool.h>                        // bool, true, false
#include <string.h>                         // memcpy, memset

static const char* TAG = "UI";              // Logging tag

static const TickType_t q_wait_delay   = 100 / portTICK_PERIOD_MS;      // Max. waiting time for the event queue
static const int debounce_button_ms    = 300;                           // Artificial delay to debounce the buttons
static const int debounce_rotary_ms    = 50;                            // Artificial delay to debounce the rotary encoder
static volatile int64_t debounce_until = 0;                             // System time until when to ignore GPIO events

typedef enum {
    UI_BUTTON_NONE = 0,                     // No button-press detected or button was already handled
    UI_BUTTON_ENTER,                        // ENTER button
    UI_BUTTON_EXIT,                         // EXIT button
    UI_BUTTON_HOME,                         // HOME button
    UI_BUTTON_INCREASE,                     // Rotary encoder INCREASE
    UI_BUTTON_DECREASE,                     // Rotary encoder DECREASE
    UI_BUTTON_COMMAND,                      // One of the configured command buttons
} ui_button_t;

typedef struct {
    ui_button_t   btn;                      // Pressed button
    ui_command_t* cmd;                      // Command button: Which command
} ui_button_event_t;

// Utility functions
ui_menu_t         copy_menu                    (ui_menu_t* menu_orig);
uint64_t          gpio_bitmask                 (int io_pin);
uint64_t          calc_cmd_button_gpio_bitmask (ui_menu_t* menu);
void              add_cmd_button_isr_handlers  (ui_menu_t* menu);
void              cmd_button_isr_handler       (void *arg);
void              menu_button_isr_handler      (void *arg);
void              rotary_encoder_isr_handler   (void *arg);
bool              debounce                     (int millis);
ui_button_event_t get_button_event             ();
ui_button_event_t execute_command              (ui_command_t* cmd);

// Screen logic
ui_button_event_t screen_menu                  (ui_menu_t* menu);
ui_button_event_t screen_parameter             (ui_command_t* cmd);
void              ui_task                      (void* parameters);

ui_config_t config = {};                    // UI configuration (global copy)
QueueHandle_t event_queue  = {};            // FIFO queue for messages from ISR to UI task

/**
 * Initialize user interface
 */
void ui_init(ui_config_t* cfg) {
    config = *cfg;
    config.main_menu = copy_menu(&cfg->main_menu);

    event_queue = xQueueCreate(10, sizeof(ui_button_event_t));

    ui_display_init();

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
    if (config.btn_enter_io > 0) gpio_isr_handler_add(config.btn_enter_io, menu_button_isr_handler,    (void*) UI_BUTTON_ENTER);
    if (config.btn_exit_io  > 0) gpio_isr_handler_add(config.btn_exit_io,  menu_button_isr_handler,    (void*) UI_BUTTON_EXIT);
    if (config.btn_home_io  > 0) gpio_isr_handler_add(config.btn_home_io,  menu_button_isr_handler,    (void*) UI_BUTTON_HOME);

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
 * Set GPIO bit mask for a single pin. But only, if the pin number is positive.
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
 * Recursive function to register the interrupt handler for all direct-access buttons
 * inside the given menu.
 */
void add_cmd_button_isr_handlers(ui_menu_t* menu) {
    for (int i = 0; i < menu->n_commands; i++) {
        ui_command_t* cmd = &menu->commands[i];
        if (cmd->button_io > 0) {
            gpio_isr_handler_add(cmd->button_io, cmd_button_isr_handler, (void*) cmd);
        }
        add_cmd_button_isr_handlers(&cmd->sub_menu);
    }
}

/**
 * Interrupt handler for the direct-access command buttons. Posts a button event to the UI task.
 */
void cmd_button_isr_handler(void *arg) {
    if (debounce(debounce_button_ms)) return;
    ui_button_event_t event = {.btn = UI_BUTTON_COMMAND, .cmd = (ui_command_t*) arg};

    BaseType_t higherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(event_queue, &event, &higherPriorityTaskWoken);
    portYIELD_FROM_ISR(higherPriorityTaskWoken);
}

/**
 * Interrupt handler for the navigation buttons. Posts a button event to the UI task.
 */
void menu_button_isr_handler(void *arg) {
    if (debounce(debounce_button_ms)) return;
    ui_button_event_t event = {.btn = (ui_button_t) arg};

    BaseType_t higherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(event_queue, &event, &higherPriorityTaskWoken);
    portYIELD_FROM_ISR(higherPriorityTaskWoken);
}

/**
 * Interrupt handler for the rotary encoder. Posts a button event to the UI task.
 */
void rotary_encoder_isr_handler(void *arg) {
    if (debounce(debounce_rotary_ms)) return;
    bool increase = gpio_get_level(config.renc_dir_io);
    ui_button_event_t event = { .btn = increase ? UI_BUTTON_INCREASE : UI_BUTTON_DECREASE };

    BaseType_t higherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(event_queue, &event, &higherPriorityTaskWoken);
    portYIELD_FROM_ISR(higherPriorityTaskWoken);
}

/**
 * Utility function to debounce GPIO inputs. Returns `true` when we are still debouncing
 * and the current event must be ignored. Saves the time until when debounce occurs.
 */
bool debounce(int millis) {
    int64_t time  = esp_timer_get_time();
    int64_t until = debounce_until;
    debounce_until = time + (millis * 1000);
    return until > time;
}

/**
 * Get latest button event sent from the interrupt handlers. If no event is received
 * after `q_wait_delay` milliseconds a `UI_BUTTON_NONE` event is returned.
 */
ui_button_event_t get_button_event() {
    ui_button_event_t event = {};
    xQueueReceive(event_queue, &event, q_wait_delay);

    if (event.btn) ESP_LOGI(TAG, "Button event: %i", event.btn);
    return event;
}

/**
 * Background task for the actual UI logic. Simply sits in a loop and calls the function
 * the runs the home screen. Also handles the global buttons that the home screen returns
 * because it cannot handle them itself.
 */
void ui_task(void* parameters) {
    while (true) {
        ui_button_event_t event = screen_menu(&config.main_menu);

        if (event.btn == UI_BUTTON_COMMAND && event.cmd) {
            event = execute_command(event.cmd);
        }
    }
}

/**
 * Execute the given command object by calling its execute callback and starting the appropriate
 * screen, if there is any.
 */
ui_button_event_t execute_command(ui_command_t* cmd) {
    if (cmd->cb.execute) cmd->cb.execute(cmd->cb.arg);

    if (cmd->param.value)       return screen_parameter(cmd);
    if (cmd->sub_menu.commands) return screen_menu(&cmd->sub_menu);

    ui_button_event_t dummy = {};
    return dummy;
}

/**
 * Menu screen.
 */
ui_button_event_t screen_menu(ui_menu_t* menu) {
    bool redraw = true;
    int selection = 0;
    ui_button_event_t event = {};

    while (true) {
        if (redraw) {
            if (selection < 0) selection = menu->n_commands - 1;
            else if (selection > menu->n_commands - 1) selection = 0;

            ui_display_show_menu(menu, (unsigned int) selection);
            redraw = false;
        }

        if (!event.btn) event = get_button_event();

        switch (event.btn) {
            case UI_BUTTON_INCREASE:
                event.btn = UI_BUTTON_NONE;
                selection++;
                redraw = true;
                break;

            case UI_BUTTON_DECREASE:
                event.btn = UI_BUTTON_NONE;
                selection--;
                redraw = true;
                break;

            case UI_BUTTON_ENTER:
                event.btn = UI_BUTTON_NONE;
                event = execute_command(&menu->commands[selection]);
                redraw = true;
                break;

            case UI_BUTTON_EXIT:
                event.btn = UI_BUTTON_NONE;
                return event;

            case UI_BUTTON_NONE:
                // Ignored buttons
                event.btn = UI_BUTTON_NONE;
                break;

            default:
                // Buttons handles by one of the parents
                return event;
        }
    }
}

/**
 * Change parameter screen.
 */
ui_button_event_t screen_parameter(ui_command_t* cmd) {
    bool redraw = true;
    ui_button_event_t event = {};

    while (true) {
        if (redraw) {
            if (*cmd->param.value < cmd->param.min) *cmd->param.value = cmd->param.min;
            else if (*cmd->param.value > cmd->param.max) *cmd->param.value = cmd->param.max;

            ui_display_show_param(cmd->name, *cmd->param.value);
            redraw = false;
        }

        if (!event.btn) event = get_button_event();

        switch (event.btn) {
            case UI_BUTTON_INCREASE:
                event.btn = UI_BUTTON_NONE;
                *cmd->param.value += cmd->param.step;
                redraw = true;
                break;

            case UI_BUTTON_DECREASE:
                event.btn = UI_BUTTON_NONE;
                *cmd->param.value -= cmd->param.step;
                redraw = true;
                break;

            case UI_BUTTON_EXIT:
                event.btn = UI_BUTTON_NONE;
                return event;

            case UI_BUTTON_ENTER:
            case UI_BUTTON_NONE:
                // Ignored buttons
                event.btn = UI_BUTTON_NONE;
                break;

            default:
                // Buttons handled by one of the parents
                return event;
        }
    }
}

