/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 */

#include <zephyr/kernel.h>
#include "util.h"
#include <math.h>
#include <string.h>

#include "key_legend_provider.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// Define a structure to hold behavior mappings
typedef struct {
    const char *name;
    const char *prefix;
} behavior_mapping_t;

// Define the mappings
static const behavior_mapping_t behavior_mappings[] = {
    {"momentary_layer", "MO"}, {"toggle_layer", "TO"}, {"mod_tap", "MT"},
    {"layer_tap", "LT"},       {"tap_dance", "TD"},    {"hold_tap", "HT"},
    {"sticky_key", "SK"},      {"caps_word", "CS"},    {"key_repeat", "REP"},
};

// Function to get the prefix for a given behavior name
const char *get_behavior_prefix(const char *behavior_name) {
    for (size_t i = 0; i < sizeof(behavior_mappings) / sizeof(behavior_mapping_t); i++) {
        if (strcmp(behavior_name, behavior_mappings[i].name) == 0) {
            return behavior_mappings[i].prefix;
        }
    }
    return NULL; // Return NULL if no match is found
}

void transform_coords(int x, int y, int rot, int rx, int ry, float *out_x, float *out_y) {
    // Convert rotation to radians
    float angle = (rot / 100.0f) * (3.14159f / 180.0f);

    // Translate to rotation point
    float dx = x - rx;
    float dy = y - ry;

    // Rotate
    float rotated_x = dx * cosf(angle) - dy * sinf(angle);
    float rotated_y = dx * sinf(angle) + dy * cosf(angle);

    // Translate back
    *out_x = (rotated_x + rx) * SCALE_FACTOR + OFFSET_X;
    *out_y = (rotated_y + ry) * SCALE_FACTOR + OFFSET_Y;
}

LV_IMG_DECLARE(bolt);

void rotate_canvas(lv_obj_t *canvas, lv_color_t cbuf[]) {
    static lv_color_t cbuf_tmp[CANVAS_SIZE * CANVAS_SIZE];
    memcpy(cbuf_tmp, cbuf, sizeof(cbuf_tmp));
    lv_img_dsc_t img;
    img.data = (void *)cbuf_tmp;
    img.header.cf = LV_IMG_CF_TRUE_COLOR;
    img.header.w = CANVAS_SIZE;
    img.header.h = CANVAS_SIZE;

    lv_canvas_fill_bg(canvas, LVGL_BACKGROUND, LV_OPA_COVER);
    lv_canvas_transform(canvas, &img, 1800, LV_IMG_ZOOM_NONE, -1, 0, CANVAS_SIZE / 2,
                        CANVAS_SIZE / 2, true);
}

void draw_battery(lv_obj_t *canvas, const struct status_state *state) {
    lv_draw_rect_dsc_t rect_black_dsc;
    init_rect_dsc(&rect_black_dsc, LVGL_BACKGROUND);
    lv_draw_rect_dsc_t rect_white_dsc;
    init_rect_dsc(&rect_white_dsc, LVGL_FOREGROUND);

    lv_canvas_draw_rect(canvas, 0, 2, 29, 12, &rect_white_dsc);
    lv_canvas_draw_rect(canvas, 1, 3, 27, 10, &rect_black_dsc);
    lv_canvas_draw_rect(canvas, 2, 4, (state->battery + 2) / 4, 8, &rect_white_dsc);
    lv_canvas_draw_rect(canvas, 30, 5, 3, 6, &rect_white_dsc);
    lv_canvas_draw_rect(canvas, 31, 6, 1, 4, &rect_black_dsc);

    if (state->charging) {
        lv_draw_img_dsc_t img_dsc;
        lv_draw_img_dsc_init(&img_dsc);
        lv_canvas_draw_img(canvas, 9, -1, &bolt, &img_dsc);
    }
}

void draw_keymap(lv_obj_t *canvas, const key_map *key_mapping, size_t key_count,
                 int keyboard_x_offset) {
    lv_draw_label_dsc_t key_label_dsc;
    init_label_dsc(&key_label_dsc, LVGL_BACKGROUND, &lv_font_montserrat_10, LV_TEXT_ALIGN_CENTER);
    lv_draw_label_dsc_t key_label_dsc_small;
    init_label_dsc(&key_label_dsc_small, LVGL_BACKGROUND, &lv_font_montserrat_8,
                   LV_TEXT_ALIGN_CENTER);

    // Draw keyboard outline
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.radius = 4;
    rect_dsc.bg_color = lv_color_black();
    rect_dsc.border_color = lv_color_white();
    rect_dsc.border_width = 1;

    // For each key in the left half
    for (int i = 0; i < key_count; i++) {
        float x, y;
        int offset_x = 22 + keyboard_x_offset;
        int offset_y = 15;
        const struct key_position key = key_mapping[i].position;

        // Transform coordinates
        transform_coords(key.x, key.y, key.rot, key.rx, key.ry, &x, &y);

        // Draw key outline
        lv_canvas_draw_rect(canvas, x - offset_x, y - offset_y, 100 * SCALE_FACTOR,
                            100 * SCALE_FACTOR, &rect_dsc);

        // Get and draw the key label
        char key_text[32] = {};

        struct key_bindings binding = key_mapping[i].bindings;

        if (binding.param1 != 0) {
            // First check if it's a basic keycode
            const char *key_name = get_key_name(binding.param1, key_mapping[i].has_shift);

            if (key_name != NULL) {
                strncpy(key_text, key_name, sizeof(key_text) - 1);
            } else {
                // Main logic to set key_text based on behavior
                const char *behavior_name = binding.behavior;

                if (behavior_name != NULL) {
                    const char *prefix = get_behavior_prefix(behavior_name);

                    if (strcmp(behavior_name, "caps_word") == 0 ||
                        strcmp(behavior_name, "key_repeat") == 0) {
                        strncpy(key_text, prefix, sizeof(key_text) - 1);
                    } else {
                        uint16_t tap_keycode = (strcmp(behavior_name, "momentary_layer") == 0 ||
                                                strcmp(behavior_name, "layer_tap") == 0 ||
                                                strcmp(behavior_name, "hold_tap") == 0)
                                                   ? key_mapping[i].bindings.param2
                                                   : key_mapping[i].bindings.param1;
                        const char *tap_key_name =
                            get_key_name(tap_keycode, key_mapping[i].has_shift);
                        if (tap_key_name != NULL) {
                            snprintf(key_text, sizeof(key_text), "%s/%s", prefix, tap_key_name);
                        } else {
                            snprintf(key_text, sizeof(key_text), "%s %d", prefix, binding.param1);
                        }
                    }

                    if (prefix == NULL) {
                        strncpy(key_text, behavior_name, sizeof(key_text) - 1);
                    }
                } else {
                    strncpy(key_text, key_name, sizeof(key_text) - 1);
                }
            }
        }

        float adjust_Y = strlen(key_text) > 3 ? 0 : 20;

        // Draw text centered in key
        lv_canvas_draw_text(canvas, (x - offset_x) + (6 * SCALE_FACTOR),
                            (y - offset_y) + (adjust_Y * SCALE_FACTOR), 16,
                            strlen(key_text) > 2 ? &key_label_dsc_small : &key_label_dsc, key_text);
    }
}

void init_label_dsc(lv_draw_label_dsc_t *label_dsc, lv_color_t color, const lv_font_t *font,
                    lv_text_align_t align) {
    lv_draw_label_dsc_init(label_dsc);
    label_dsc->color = color;
    label_dsc->font = font;
    label_dsc->align = align;
}

void init_rect_dsc(lv_draw_rect_dsc_t *rect_dsc, lv_color_t bg_color) {
    lv_draw_rect_dsc_init(rect_dsc);
    rect_dsc->bg_color = bg_color;
}

void init_line_dsc(lv_draw_line_dsc_t *line_dsc, lv_color_t color, uint8_t width) {
    lv_draw_line_dsc_init(line_dsc);
    line_dsc->color = color;
    line_dsc->width = width;
}

void init_arc_dsc(lv_draw_arc_dsc_t *arc_dsc, lv_color_t color, uint8_t width) {
    lv_draw_arc_dsc_init(arc_dsc);
    arc_dsc->color = color;
    arc_dsc->width = width;
}