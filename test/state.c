/*
 * Copyright © 2012 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/keysym.h>
#include <linux/input.h>
#include "xkbcommon/xkbcommon.h"
#include "utils.h"
#include "XKBcommonint.h"

/* Offset between evdev keycodes (where KEY_ESCAPE is 1), and the evdev XKB
 * keycode set (where ESC is 9). */
#define EVDEV_OFFSET 8

static void
print_state(struct xkb_state *state)
{
    xkb_group_index_t group;
    xkb_mod_index_t mod;
    xkb_led_index_t led;

    if (!state->group && !state->mods && !state->leds) {
        fprintf(stderr, "\tno state\n");
        return;
    }

    for (group = 0; group < xkb_map_num_groups(state->xkb); group++) {
        if (!xkb_state_group_index_is_active(state, group, XKB_STATE_EFFECTIVE))
            continue;
        fprintf(stderr, "\tgroup %s (%d): %s%s%s%s\n",
                xkb_map_group_get_name(state->xkb, group),
                group,
                xkb_state_group_index_is_active(state, group, XKB_STATE_EFFECTIVE) ?
                    "effective " : "",
                xkb_state_group_index_is_active(state, group, XKB_STATE_DEPRESSED) ?
                    "depressed " : "",
                xkb_state_group_index_is_active(state, group, XKB_STATE_LATCHED) ?
                    "latched " : "",
                xkb_state_group_index_is_active(state, group, XKB_STATE_LOCKED) ?
                    "locked " : "");
    }

    for (mod = 0; mod < xkb_map_num_mods(state->xkb); mod++) {
        if (!xkb_state_mod_index_is_active(state, mod, XKB_STATE_EFFECTIVE))
            continue;
        fprintf(stderr, "\tmod %s (%d): %s%s%s\n",
                xkb_map_mod_get_name(state->xkb, mod),
                mod,
                xkb_state_mod_index_is_active(state, mod, XKB_STATE_DEPRESSED) ?
                    "depressed " : "",
                xkb_state_mod_index_is_active(state, mod, XKB_STATE_LATCHED) ?
                    "latched " : "",
                xkb_state_mod_index_is_active(state, mod, XKB_STATE_LOCKED) ?
                    "locked " : "");
    }

    for (led = 0; led < xkb_map_num_leds(state->xkb); led++) {
        if (!xkb_state_led_index_is_active(state, led))
            continue;
        fprintf(stderr, "\tled %s (%d): active\n",
                xkb_map_led_get_name(state->xkb, led),
                led);
    }
}

static void
test_update_key(struct xkb_desc *xkb)
{
    struct xkb_state *state = xkb_state_new(xkb);
    xkb_keysym_t *syms;
    int num_syms;

    assert(state);

    /* LCtrl down */
    xkb_state_update_key(state, KEY_LEFTCTRL + EVDEV_OFFSET, XKB_KEY_DOWN);
    fprintf(stderr, "dumping state for LCtrl down:\n");
    print_state(state);
    assert(xkb_state_mod_name_is_active(state, "Control",
                                        XKB_STATE_DEPRESSED));

    /* LCtrl + RAlt down */
    xkb_state_update_key(state, KEY_RIGHTALT + EVDEV_OFFSET, XKB_KEY_DOWN);
    fprintf(stderr, "dumping state for LCtrl + RAlt down:\n");
    print_state(state);
    assert(xkb_state_mod_name_is_active(state, "Control",
                                        XKB_STATE_DEPRESSED));
    assert(xkb_state_mod_name_is_active(state, "Mod1",
                                        XKB_STATE_DEPRESSED));

    /* RAlt down */
    xkb_state_update_key(state, KEY_LEFTCTRL + EVDEV_OFFSET, XKB_KEY_UP);
    fprintf(stderr, "dumping state for RAlt down:\n");
    print_state(state);
    assert(!xkb_state_mod_name_is_active(state, "Control",
                                         XKB_STATE_EFFECTIVE));
    assert(xkb_state_mod_name_is_active(state, "Mod1",
                                        XKB_STATE_DEPRESSED));

    /* none down */
    xkb_state_update_key(state, KEY_RIGHTALT + EVDEV_OFFSET, XKB_KEY_UP);
    assert(!xkb_state_mod_name_is_active(state, "Mod1",
                                         XKB_STATE_EFFECTIVE));

    /* Caps locked */
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_UP);
    fprintf(stderr, "dumping state for Caps Lock:\n");
    print_state(state);
    assert(xkb_state_mod_name_is_active(state, "Caps Lock",
                                        XKB_STATE_LOCKED));
    assert(xkb_state_led_name_is_active(state, "Caps Lock"));
    num_syms = xkb_key_get_syms(state, KEY_Q + EVDEV_OFFSET, &syms);
    assert(num_syms == 1 && syms[0] == XK_Q);

    /* Caps unlocked */
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_UP);
    assert(!xkb_state_mod_name_is_active(state, "Caps Lock",
                                         XKB_STATE_EFFECTIVE));
    assert(!xkb_state_led_name_is_active(state, "Caps Lock"));
    num_syms = xkb_key_get_syms(state, KEY_Q + EVDEV_OFFSET, &syms);
    assert(num_syms == 1 && syms[0] == XK_q);

    xkb_state_unref(state);
}

static void
test_serialisation(struct xkb_desc *xkb)
{
    struct xkb_state *state = xkb_state_new(xkb);
    xkb_mod_mask_t base_mods;
    xkb_mod_mask_t latched_mods;
    xkb_mod_mask_t locked_mods;
    xkb_mod_mask_t effective_mods;
    xkb_mod_index_t caps, shift, ctrl;
    xkb_group_index_t base_group = 0;
    xkb_group_index_t latched_group = 0;
    xkb_group_index_t locked_group = 0;

    assert(state);

    caps = xkb_map_mod_get_index(state->xkb, "Caps Lock");
    assert(caps != XKB_MOD_INVALID);
    shift = xkb_map_mod_get_index(state->xkb, "Shift");
    assert(shift != XKB_MOD_INVALID);
    ctrl = xkb_map_mod_get_index(state->xkb, "Control");
    assert(ctrl != XKB_MOD_INVALID);

    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_UP);
    base_mods = xkb_state_serialise_mods(state, XKB_STATE_DEPRESSED);
    assert(base_mods == 0);
    latched_mods = xkb_state_serialise_mods(state, XKB_STATE_LATCHED);
    assert(latched_mods == 0);
    locked_mods = xkb_state_serialise_mods(state, XKB_STATE_LOCKED);
    assert(locked_mods == (1 << caps));
    effective_mods = xkb_state_serialise_mods(state, XKB_STATE_EFFECTIVE);
    assert(effective_mods == locked_mods);

    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
    base_mods = xkb_state_serialise_mods(state, XKB_STATE_DEPRESSED);
    assert(base_mods == (1 << shift));
    latched_mods = xkb_state_serialise_mods(state, XKB_STATE_LATCHED);
    assert(latched_mods == 0);
    locked_mods = xkb_state_serialise_mods(state, XKB_STATE_LOCKED);
    assert(locked_mods == (1 << caps));
    effective_mods = xkb_state_serialise_mods(state, XKB_STATE_EFFECTIVE);
    assert(effective_mods == (base_mods | locked_mods));

    base_mods |= (1 << ctrl);
    xkb_state_update_mask(state, base_mods, latched_mods, locked_mods,
                          base_group, latched_group, locked_group);

    assert(xkb_state_mod_index_is_active(state, ctrl, XKB_STATE_DEPRESSED));
    assert(xkb_state_mod_index_is_active(state, ctrl, XKB_STATE_EFFECTIVE));

    xkb_state_unref(state);
}

int
main(int argc, char *argv[])
{
    struct xkb_context *context;
    struct xkb_desc *xkb;
    struct xkb_rule_names rmlvo;

    rmlvo.rules = "evdev";
    rmlvo.model = "pc104";
    rmlvo.layout = "us";
    rmlvo.variant = NULL;
    rmlvo.options = NULL;

    context = xkb_context_new();
    assert(context);

    xkb = xkb_map_new_from_names(context, &rmlvo);
    assert(xkb);

    test_update_key(xkb);
    test_serialisation(xkb);

    xkb_map_unref(xkb);
    xkb_context_unref(context);
}
