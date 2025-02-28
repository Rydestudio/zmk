/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <lvgl.h>
#include <zmk/battery.h>
#include <zmk/display.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/split/bluetooth/peripheral.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/usb.h>
#include <zmk/ble.h>

#include "peripheral_status.h"
#include <stdlib.h>

static const struct key_position emblem_keys[] = {
    // Top row
    {100, 95, -800, 100, 300},  // 0
    {195, 32, -400, 200, 200},  // 1
    {300, 0, 0, 0, 0},          // 2
    {400, 29, 0, 0, 0},         // 3
    {500, 42, 0, 0, 0},         // 4
    {950, 42, 0, 0, 0},         // 5
    {1050, 29, 0, 0, 0},        // 6
    {1150, 0, 0, 0, 0},         // 7
    {1249, 32, 400, 1500, 300}, // 8
    {1350, 95, 800, 1500, 300}, // 9

    // Middle row
    {0, 150, -800, 100, 300},    // 10
    {100, 195, -800, 100, 300},  // 11
    {195, 132, -400, 200, 200},  // 12
    {300, 100, 0, 0, 0},         // 13
    {400, 129, 0, 0, 0},         // 14
    {500, 142, 0, 0, 0},         // 15
    {950, 142, 0, 0, 0},         // 16
    {1050, 129, 0, 0, 0},        // 17
    {1150, 100, 0, 0, 0},        // 18
    {1249, 132, 400, 1500, 300}, // 19
    {1350, 195, 800, 1500, 300}, // 20
    {1450, 150, 800, 1500, 300}, // 21

    // Bottom row
    {0, 250, -800, 100, 300},    // 22
    {100, 295, -800, 100, 300},  // 23
    {195, 232, -400, 200, 200},  // 24
    {300, 200, 0, 0, 0},         // 25
    {400, 229, 0, 0, 0},         // 26
    {500, 242, 0, 0, 0},         // 27
    {950, 242, 0, 0, 0},         // 28
    {1050, 229, 0, 0, 0},        // 29
    {1150, 200, 0, 0, 0},        // 30
    {1249, 232, 400, 1500, 300}, // 31
    {1350, 295, 800, 1500, 300}, // 32
    {1450, 250, 800, 1500, 300}, // 33

    // Thumb cluster
    {425, 340, 0, 530, 455},     // 34
    {515, 340, 1500, 530, 455},  // 35
    {600, 320, 3000, 530, 455},  // 36
    {830, 394, -3000, 870, 455}, // 37
    {932, 380, -1500, 870, 455}, // 38
    {1025, 340, 0, 870, 455}     // 39
};

LV_IMG_DECLARE(balloon);
LV_IMG_DECLARE(mountain);

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct peripheral_status_state {
    bool connected;
};

static void draw_top(lv_obj_t *widget, lv_color_t cbuf[], const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 0);

    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &lv_font_montserrat_16, LV_TEXT_ALIGN_RIGHT);
    lv_draw_rect_dsc_t rect_black_dsc;
    init_rect_dsc(&rect_black_dsc, LVGL_BACKGROUND);

    // Fill background
    lv_canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &rect_black_dsc);

    // Draw battery
    draw_battery(canvas, state);

    // Draw output status
    lv_canvas_draw_text(canvas, 0, 0, CANVAS_SIZE, &label_dsc,
                        state->connected ? LV_SYMBOL_WIFI : LV_SYMBOL_CLOSE);

    // Rotate canvas
    rotate_canvas(canvas, cbuf);
}

static void draw_bottom(lv_obj_t *widget, lv_color_t cbuf[]) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 1);

    lv_draw_rect_dsc_t rect_black_dsc;
    init_rect_dsc(&rect_black_dsc, LVGL_BACKGROUND);

    // Fill background
    lv_canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &rect_black_dsc);

    // TODO get the keymap from the physical-layout file
    const int right_keys_mapping[] = {
        5,  6,  7,  8,  9,      // Top row
        16, 17, 18, 19, 20, 21, // Middle row
        28, 29, 30, 31, 32, 33, // Bottom row
        37, 38, 39              // Thumb cluster
    };

    const int fake_key_map[20][2] = {{13, 0}, {15, 0}, {24, 0}, {28, 0}, {51, 0}, {16, 0}, {17, 0},
                                     {8, 0},  {12, 0}, {18, 0}, {41, 0}, {14, 0}, {11, 0}, {54, 0},
                                     {55, 0}, {56, 0}, {52, 0}, {44, 0}, {40, 0}, {42, 0}};

    const char *binding_keys[5][2] = {{(char *)16, "hold_tap"},
                                      {(char *)20, "hold_tap"},
                                      {(char *)33, "layer_tap"},
                                      {(char *)37, "layer_tap"},
                                      {(char *)38, "layer_tap"}};

    const size_t key_mapping_size = sizeof(right_keys_mapping) / sizeof(right_keys_mapping[0]);

    key_map *keys = (key_map *)malloc(key_mapping_size * sizeof(key_map));
    if (keys == NULL) {
        // Handle memory allocation failure
        return;
    }

    // Fake keyboard state until I can get the data
    for (int i = 0; i < key_mapping_size; i++) {
        keys[i].location = right_keys_mapping[i];
        keys[i].position = emblem_keys[right_keys_mapping[i]];
        keys[i].has_shift = false;

        keys[i].bindings.param1 = fake_key_map[i][0] & 0xFF;
        keys[i].bindings.param2 = fake_key_map[i][1] & 0xFF;

        for (int b = 0; b < sizeof(binding_keys) / sizeof(binding_keys[0]); b++) {
            if ((int)binding_keys[b][0] == right_keys_mapping[i]) {
                keys[i].bindings.behavior = binding_keys[right_keys_mapping[i]][1];
            }
        }
    }

    // Draws the keyboard with keys displayed for the current layer
    draw_keymap(canvas, keys, key_mapping_size, 144);

    // Rotate canvas
    rotate_canvas(canvas, cbuf);

    // Free allocated memory
    free(keys);
}

static void set_battery_status(struct zmk_widget_status *widget,
                               struct battery_status_state state) {
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    widget->state.charging = state.usb_present;
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */

    widget->state.battery = state.level;

    draw_top(widget->obj, widget->cbuf, &widget->state);
}

static void battery_status_update_cb(struct battery_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_status(widget, state); }
}

static struct battery_status_state battery_status_get_state(const zmk_event_t *eh) {
    return (struct battery_status_state){
        .level = zmk_battery_state_of_charge(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb_present = zmk_usb_is_powered(),
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_status, struct battery_status_state,
                            battery_status_update_cb, battery_status_get_state)

ZMK_SUBSCRIPTION(widget_battery_status, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_battery_status, zmk_usb_conn_state_changed);
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */

static struct peripheral_status_state get_state(const zmk_event_t *_eh) {
    return (struct peripheral_status_state){.connected = zmk_split_bt_peripheral_is_connected()};
}

static void set_connection_status(struct zmk_widget_status *widget,
                                  struct peripheral_status_state state) {
    widget->state.connected = state.connected;

    draw_top(widget->obj, widget->cbuf, &widget->state);
    draw_bottom(widget->obj, widget->cbuf2);
}

static void output_status_update_cb(struct peripheral_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_connection_status(widget, state); }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_peripheral_status, struct peripheral_status_state,
                            output_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_peripheral_status, zmk_split_peripheral_status_changed);

// static struct peripheral_keymap_state get_state(const zmk_event_t *_eh) {
//     return (struct peripheral_keymap_state){.connected = zmk_split_bt_peripheral_is_connected()};
// }

// static void set_keymap_state(struct zmk_widget_keymap *widget,
//                                   struct peripheral_keymap_state state) {
//     widget->state.connected = state.connected;

//     draw_top(widget->obj, widget->cbuf, &widget->state);
// }

// static void output_keymap_update_cb(struct peripheral_keymap_state state) {
//     struct zmk_widget_keymap *widget;
//     SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_keymap_state(widget, state); }
// }

// ZMK_DISPLAY_WIDGET_LISTENER(widget_peripheral_keymap, struct peripheral_keymap_state,
//     output_keymap_update_cb, get_state)
// ZMK_SUBSCRIPTION(widget_peripheral_keymap, zmk_split_peripheral_keymap_layout_changed);

int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, CANVAS_WIDTH, CANVAS_HEIGHT);

    lv_obj_t *top = lv_canvas_create(widget->obj);
    lv_obj_align(top, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_canvas_set_buffer(top, widget->cbuf, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR);

    lv_obj_t *bottom = lv_canvas_create(widget->obj);
    lv_obj_align(bottom, LV_ALIGN_RIGHT_MID, 0, -50);
    lv_canvas_set_buffer(bottom, widget->cbuf2, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR);

    // lv_obj_t *art = lv_img_create(widget->obj);
    // bool random = sys_rand32_get() & 1;
    // lv_img_set_src(art, random ? &balloon : &mountain);
    // lv_obj_align(art, LV_ALIGN_BOTTOM_MID, 0, -24);

    sys_slist_append(&widgets, &widget->node);
    widget_battery_status_init();
    widget_peripheral_status_init();
    // widget_peripheral_keymap_state();

    return 0;
}

lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget) { return widget->obj; }