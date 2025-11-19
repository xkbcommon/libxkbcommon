/*
 * Copyright © 2025 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "config.h"

#include "xkbcommon/xkbcommon.h"
#include "keymap.h"

struct state_components {
    /* These may be negative, because of -1 group actions. */
    int32_t base_group; /**< depressed */
    int32_t latched_group;
    int32_t locked_group;
    xkb_layout_index_t group; /**< effective */

    xkb_mod_mask_t base_mods; /**< depressed */
    xkb_mod_mask_t latched_mods;
    xkb_mod_mask_t locked_mods;
    xkb_mod_mask_t mods; /**< effective */

    xkb_led_mask_t leds;

    enum xkb_action_controls controls;
};

struct xkb_event {
    enum xkb_event_type type;
    union {
        xkb_keycode_t keycode;
        struct {
            struct state_components components;
            enum xkb_state_component changed;
        } components;
    };
};

/** Reference count is not updated */
XKB_EXPORT_PRIVATE struct xkb_state *
xkb_state_machine_get_state(struct xkb_state_machine *sm);
