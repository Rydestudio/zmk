/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <lvgl.h>
#include <zmk/hid.h>
#include "util.h"

// void draw_keyboard_layout(lv_obj_t *canvas, const struct status_state *state,
//                           const int key_mapping[], size_t keymapping_size);
void get_keyboard_state(key_map *keys, uint8_t layer_index, const int key_mapping[],
                        size_t number_of_keys);