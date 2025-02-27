/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 */

#include <zephyr/kernel.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>
#include <math.h>
#include <string.h>

#include "keyboard.h"

#include <zmk/physical_layouts.h>
// #include "key_legend_provider.h"

// Define a structure to hold behavior mappings
// typedef struct {
//     const char *name;
//     const char *prefix;
// } behavior_mapping_t;

// // Define the mappings
// static const behavior_mapping_t behavior_mappings[] = {
//     {"momentary_layer", "MO"}, {"toggle_layer", "TO"}, {"mod_tap", "MT"},
//     {"layer_tap", "LT"},       {"tap_dance", "TD"},    {"hold_tap", "HT"},
//     {"sticky_key", "SK"},      {"caps_word", "CAPS"},  {"key_repeat", "REP"},
// };

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

// Function to get the prefix for a given behavior name
// const char *get_behavior_prefix(const char *behavior_name) {
//     for (size_t i = 0; i < sizeof(behavior_mappings) / sizeof(behavior_mapping_t); i++) {
//         if (strcmp(behavior_name, behavior_mappings[i].name) == 0) {
//             return behavior_mappings[i].prefix;
//         }
//     }
//     return NULL; // Return NULL if no match is found
// }

// void transform_coords(int x, int y, int rot, int rx, int ry, float *out_x, float *out_y) {
//     // Convert rotation to radians
//     float angle = (rot / 100.0f) * (3.14159f / 180.0f);

//     // Translate to rotation point
//     float dx = x - rx;
//     float dy = y - ry;

//     // Rotate
//     float rotated_x = dx * cosf(angle) - dy * sinf(angle);
//     float rotated_y = dx * sinf(angle) + dy * cosf(angle);

//     // Translate back
//     *out_x = (rotated_x + rx) * SCALE_FACTOR + OFFSET_X;
//     *out_y = (rotated_y + ry) * SCALE_FACTOR + OFFSET_Y;
// }

static const struct zmk_behavior_binding *get_resolved_binding(int position, int current_layer) {
    // Iterate through active layers in reverse order
    for (int layer = current_layer; layer >= 0; layer--) {
        if (!zmk_keymap_layer_active(layer)) {
            continue; // Skip inactive layers
        }

        const struct zmk_behavior_binding *binding =
            zmk_keymap_get_layer_binding_at_idx(layer, position);

        // Check if the binding is valid and not transparent
        if (binding && zmk_behavior_get_local_id(binding->behavior_dev) != 0) {
            const char *behavior_name = zmk_behavior_find_behavior_name_from_local_id(
                zmk_behavior_get_local_id(binding->behavior_dev));
            if (behavior_name == NULL || strcmp(behavior_name, "transparent") != 0) {
                return binding; // Return the first valid binding found
            }
        }
    }
    return NULL; // Return NULL if no valid binding is found
}

void get_keyboard_state(key_map *keys, uint8_t layer_index, const int key_mapping[],
                        size_t number_of_keys) {

    for (int i = 0; i < number_of_keys; i++) {

        keys[i].location = key_mapping[i];
        keys[i].position = emblem_keys[key_mapping[i]];
        // TODO figure out how to get the shifted state as this is returning as always true.
        keys[i].has_shift = zmk_hid_mod_is_pressed(HID_USAGE_KEY_KEYBOARD_LEFTSHIFT);

        // Get and draw the key label
        const struct zmk_behavior_binding *binding =
            get_resolved_binding(key_mapping[i], layer_index);

        if (binding != NULL) {
            keys[i].bindings.param1 = binding->param1 & 0xFF;
            keys[i].bindings.param2 = binding->param2 & 0xFF;
            if (binding->behavior_dev != NULL) {
                uint32_t behavior_id = zmk_behavior_get_local_id(binding->behavior_dev);
                const char *behavior_name =
                    zmk_behavior_find_behavior_name_from_local_id(behavior_id);
                if (behavior_name != NULL) {
                    keys[i].bindings.behavior = behavior_name;
                }
            }
        }
    }
}

// TODO move draw keyboard here so it can be used in both status and peripheral status
// void draw_keyboard_layout(lv_obj_t *canvas, const struct status_state *state,
//                           const int key_mapping[], size_t key_mapping_size) {

//     lv_draw_label_dsc_t key_label_dsc;
//     init_label_dsc(&key_label_dsc, LVGL_BACKGROUND, &lv_font_montserrat_10,
//     LV_TEXT_ALIGN_CENTER); lv_draw_label_dsc_t key_label_dsc_small;
//     init_label_dsc(&key_label_dsc_small, LVGL_BACKGROUND, &lv_font_montserrat_8,
//                    LV_TEXT_ALIGN_CENTER);

//     // Draw keyboard outline
//     lv_draw_rect_dsc_t rect_dsc;
//     lv_draw_rect_dsc_init(&rect_dsc);
//     rect_dsc.radius = 4;
//     rect_dsc.bg_color = lv_color_black();
//     rect_dsc.border_color = lv_color_white();
//     rect_dsc.border_width = 1;

//     // For each key in the left half
//     for (int i = 0; i < key_mapping_size; i++) {
//         float x, y;
//         int offset_x = 22;
//         int offset_y = 15;
//         const struct key_position key = left_keys[i];

//         // Transform coordinates
//         transform_coords(key.x, key.y, key.rot, key.rx, key.ry, &x, &y);

//         // Draw key outline
//         lv_canvas_draw_rect(canvas, x - offset_x, y - offset_y, 100 * SCALE_FACTOR,
//                             100 * SCALE_FACTOR, &rect_dsc);

//         // Get and draw the key label
//         char key_text[32] = {};
//         const struct zmk_behavior_binding *binding =
//             get_resolved_binding(key_mapping[i], state->layer_index);

//         if (binding != NULL) {
//             // First check if it's a basic keycode
//             uint16_t keycode = binding->param1 & 0xFF;
//             const char *key_name = get_key_name(keycode);

//             if (key_name != NULL) {
//                 strncpy(key_text, key_name, sizeof(key_text) - 1);
//             } else {
//                 // Check behavior type
//                 uint32_t behavior_id = zmk_behavior_get_local_id(binding->behavior_dev);
//                 const char *behavior_name =
//                     zmk_behavior_find_behavior_name_from_local_id(behavior_id);

//                 // Main logic to set key_text based on behavior
//                 if (behavior_name != NULL) {
//                     const char *prefix = get_behavior_prefix(behavior_name);

//                     if (prefix != NULL) {
//                         if (strcmp(prefix, "CAPS") == 0 || strcmp(prefix, "REP") == 0) {
//                             strncpy(key_text, prefix, sizeof(key_text) - 1);
//                         } else {
//                             uint16_t tap_keycode =
//                                 (strcmp(prefix, "MT") == 0 || strcmp(prefix, "LT") == 0 ||
//                                  strcmp(prefix, "HT") == 0)
//                                     ? binding->param2 & 0xFF
//                                     : binding->param1 & 0xFF;
//                             const char *tap_key_name = get_key_name(tap_keycode);
//                             if (tap_key_name != NULL) {
//                                 snprintf(key_text, sizeof(key_text), "%s/%s", prefix,
//                                 tap_key_name);
//                             } else {
//                                 snprintf(key_text, sizeof(key_text), "%s %d", prefix,
//                                          binding->param1);
//                             }
//                         }
//                     } else {
//                         strncpy(key_text, behavior_name, sizeof(key_text) - 1);
//                     }
//                 }
//             }

//             float adjust_Y = strlen(key_text) > 3 ? 0 : 20;

//             // Draw text centered in key
//             lv_canvas_draw_text(canvas, (x - offset_x) + (6 * SCALE_FACTOR),
//                                 (y - offset_y) + (adjust_Y * SCALE_FACTOR), 16,
//                                 strlen(key_text) > 2 ? &key_label_dsc_small : &key_label_dsc,
//                                 key_text);
//         }
//     }
// }