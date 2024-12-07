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

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "evdev-scancodes.h"
#include "src/keysym.h"
#include "src/keymap.h"
#include "test.h"

/* Offset between evdev keycodes (where KEY_ESCAPE is 1), and the evdev XKB
 * keycode set (where ESC is 9). */
#define EVDEV_OFFSET 8

static inline xkb_mod_index_t
_xkb_keymap_mod_get_index(struct xkb_keymap *keymap, const char *name)
{
    xkb_mod_index_t mod = xkb_keymap_mod_get_index(keymap, name);
    assert(mod != XKB_MOD_INVALID);
    return mod;
}

static inline xkb_led_index_t
_xkb_keymap_led_get_index(struct xkb_keymap *keymap, const char *name)
{
    xkb_led_index_t led = xkb_keymap_led_get_index(keymap, name);
    assert(led != XKB_LED_INVALID);
    return led;
}

static void
print_modifiers_serialization(struct xkb_state *state)
{
    xkb_mod_mask_t base = xkb_state_serialize_mods(state, XKB_STATE_MODS_DEPRESSED);
    xkb_mod_mask_t latched = xkb_state_serialize_mods(state, XKB_STATE_MODS_LATCHED);
    xkb_mod_mask_t locked = xkb_state_serialize_mods(state, XKB_STATE_MODS_LOCKED);
    xkb_mod_mask_t effective = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
    fprintf(stderr, "\tMods: Base: 0x%x, Latched: 0x%x, Locked: 0x%x, Effective: 0x%x\n", base, latched, locked, effective);
}

static void
print_layout_serialization(struct xkb_state *state)
{
    xkb_mod_mask_t base = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_DEPRESSED);
    xkb_mod_mask_t latched = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LATCHED);
    xkb_mod_mask_t locked = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LOCKED);
    xkb_mod_mask_t effective = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE);
    fprintf(stderr, "\tLayout: Base: 0x%x, Latched: 0x%x, Locked: 0x%x, Effective: 0x%x\n", base, latched, locked, effective);
}

static void
print_state(struct xkb_state *state)
{
    struct xkb_keymap *keymap;
    xkb_layout_index_t group;
    xkb_mod_index_t mod;
    xkb_led_index_t led;

    group = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE);
    mod = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
    /* led = xkb_state_serialize_leds(state, XKB_STATE_LEDS); */
    if (!group && !mod /* && !led */) {
        fprintf(stderr, "\tno state\n");
        return;
    }

    keymap = xkb_state_get_keymap(state);

    for (group = 0; group < xkb_keymap_num_layouts(keymap); group++) {
        if (xkb_state_layout_index_is_active(state, group,
                                             XKB_STATE_LAYOUT_EFFECTIVE |
                                             XKB_STATE_LAYOUT_DEPRESSED |
                                             XKB_STATE_LAYOUT_LATCHED |
                                             XKB_STATE_LAYOUT_LOCKED) <= 0)
            continue;
        fprintf(stderr, "\tgroup %s (%d): %s%s%s%s\n",
                xkb_keymap_layout_get_name(keymap, group),
                group,
                xkb_state_layout_index_is_active(state, group, XKB_STATE_LAYOUT_EFFECTIVE) > 0 ?
                    "effective " : "",
                xkb_state_layout_index_is_active(state, group, XKB_STATE_LAYOUT_DEPRESSED) > 0 ?
                    "depressed " : "",
                xkb_state_layout_index_is_active(state, group, XKB_STATE_LAYOUT_LATCHED) > 0 ?
                    "latched " : "",
                xkb_state_layout_index_is_active(state, group, XKB_STATE_LAYOUT_LOCKED) > 0 ?
                    "locked " : "");
    }

    for (mod = 0; mod < xkb_keymap_num_mods(keymap); mod++) {
        if (xkb_state_mod_index_is_active(state, mod,
                                          XKB_STATE_MODS_EFFECTIVE |
                                          XKB_STATE_MODS_DEPRESSED |
                                          XKB_STATE_MODS_LATCHED |
                                          XKB_STATE_MODS_LOCKED) <= 0)
            continue;
        fprintf(stderr, "\tmod %s (%d): %s%s%s%s\n",
                xkb_keymap_mod_get_name(keymap, mod),
                mod,
                xkb_state_mod_index_is_active(state, mod, XKB_STATE_MODS_EFFECTIVE) > 0 ?
                    "effective " : "",
                xkb_state_mod_index_is_active(state, mod, XKB_STATE_MODS_DEPRESSED) > 0 ?
                    "depressed " : "",
                xkb_state_mod_index_is_active(state, mod, XKB_STATE_MODS_LATCHED) > 0 ?
                    "latched " : "",
                xkb_state_mod_index_is_active(state, mod, XKB_STATE_MODS_LOCKED) > 0 ?
                    "locked " : "");
    }

    for (led = 0; led < xkb_keymap_num_leds(keymap); led++) {
        if (xkb_state_led_index_is_active(state, led) <= 0)
            continue;
        fprintf(stderr, "\tled %s (%d): active\n",
                xkb_keymap_led_get_name(keymap, led),
                led);
    }
}

#define check_serialize_layout(components, expected, got) \
    xkb_state_serialize_layout(expected, components) == \
    xkb_state_serialize_layout(got, components)

#define check_serialize_mods(components, expected, got) \
    xkb_state_serialize_mods(expected, components) == \
    xkb_state_serialize_mods(got, components)

static bool
check_state(struct xkb_state *expected, struct xkb_state *got)
{
    bool ok = check_serialize_layout(XKB_STATE_LAYOUT_DEPRESSED, expected, got) &&
              check_serialize_layout(XKB_STATE_LAYOUT_LATCHED, expected, got) &&
              check_serialize_layout(XKB_STATE_LAYOUT_LOCKED, expected, got) &&
              check_serialize_layout(XKB_STATE_LAYOUT_EFFECTIVE, expected, got) &&
              check_serialize_mods(XKB_STATE_MODS_DEPRESSED, expected, got) &&
              check_serialize_mods(XKB_STATE_MODS_LATCHED, expected, got) &&
              check_serialize_mods(XKB_STATE_MODS_LOCKED, expected, got) &&
              check_serialize_mods(XKB_STATE_MODS_EFFECTIVE, expected, got);

    struct xkb_keymap *keymap = xkb_state_get_keymap(expected);

    for (xkb_led_index_t led = 0; led < xkb_keymap_num_leds(keymap); led++) {
        if (xkb_state_led_index_is_active(expected, led) !=
            xkb_state_led_index_is_active(got, led)) {
            ok = false;
            break;
        }
    }

    if (!ok) {
        fprintf(stderr, "Expected state:\n");
        print_state(expected);
        print_layout_serialization(expected);
        print_modifiers_serialization(expected);
        fprintf(stderr, "Got state:\n");
        print_state(got);
        print_layout_serialization(got);
        print_modifiers_serialization(got);
    }
    return ok;
}

static void
test_update_key(struct xkb_keymap *keymap)
{
    struct xkb_state *state = xkb_state_new(keymap);
    const xkb_keysym_t *syms;
    xkb_keysym_t one_sym;
    int num_syms;

    assert(state);

    xkb_mod_index_t ctrl = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_CTRL);
    xkb_mod_index_t mod1 = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_MOD1);
    xkb_mod_index_t alt  = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_ALT);
    xkb_mod_index_t meta = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_META);

    /* LCtrl down */
    xkb_state_update_key(state, KEY_LEFTCTRL + EVDEV_OFFSET, XKB_KEY_DOWN);
    fprintf(stderr, "dumping state for LCtrl down:\n");
    print_state(state);
    assert(xkb_state_mod_name_is_active(state, XKB_MOD_NAME_CTRL,
                                        XKB_STATE_MODS_DEPRESSED) > 0);

    /* LCtrl + RAlt down */
    xkb_state_update_key(state, KEY_RIGHTALT + EVDEV_OFFSET, XKB_KEY_DOWN);
    fprintf(stderr, "dumping state for LCtrl + RAlt down:\n");
    print_state(state);
    assert(xkb_state_mod_name_is_active(state, XKB_MOD_NAME_CTRL,
                                        XKB_STATE_MODS_DEPRESSED) > 0);
    assert(xkb_state_mod_name_is_active(state, XKB_MOD_NAME_MOD1,
                                        XKB_STATE_MODS_DEPRESSED) > 0);
    assert(xkb_state_mod_name_is_active(state, XKB_VMOD_NAME_ALT,
                                        XKB_STATE_MODS_DEPRESSED) > 0);
    assert(xkb_state_mod_name_is_active(state, XKB_VMOD_NAME_META,
                                        XKB_STATE_MODS_DEPRESSED) > 0);
    assert(xkb_state_mod_names_are_active(state, XKB_STATE_MODS_DEPRESSED,
                                          XKB_STATE_MATCH_ALL,
                                          XKB_MOD_NAME_CTRL,
                                          XKB_MOD_NAME_MOD1,
                                          XKB_VMOD_NAME_ALT,
                                          XKB_VMOD_NAME_META,
                                          NULL) > 0);
    assert(xkb_state_mod_indices_are_active(state, XKB_STATE_MODS_DEPRESSED,
                                            XKB_STATE_MATCH_ALL,
                                            ctrl, mod1, alt, meta,
                                            XKB_MOD_INVALID) > 0);
    assert(xkb_state_mod_names_are_active(state, XKB_STATE_MODS_DEPRESSED,
                                          XKB_STATE_MATCH_ALL,
                                          XKB_MOD_NAME_MOD1,
                                          NULL) == 0);
    assert(xkb_state_mod_names_are_active(state, XKB_STATE_MODS_DEPRESSED,
                                          XKB_STATE_MATCH_ALL,
                                          XKB_VMOD_NAME_ALT,
                                          NULL) == 0);
    assert(xkb_state_mod_names_are_active(state, XKB_STATE_MODS_DEPRESSED,
                                          XKB_STATE_MATCH_ALL,
                                          XKB_VMOD_NAME_META,
                                          NULL) == 0);
    assert(xkb_state_mod_names_are_active(state, XKB_STATE_MODS_DEPRESSED,
                                          XKB_STATE_MATCH_ALL |
                                          XKB_STATE_MATCH_NON_EXCLUSIVE,
                                          XKB_MOD_NAME_MOD1,
                                          NULL) > 0);
    assert(xkb_state_mod_names_are_active(state, XKB_STATE_MODS_DEPRESSED,
                                          XKB_STATE_MATCH_ALL |
                                          XKB_STATE_MATCH_NON_EXCLUSIVE,
                                          XKB_VMOD_NAME_ALT,
                                          NULL) > 0);
    assert(xkb_state_mod_names_are_active(state, XKB_STATE_MODS_DEPRESSED,
                                          XKB_STATE_MATCH_ALL |
                                          XKB_STATE_MATCH_NON_EXCLUSIVE,
                                          XKB_VMOD_NAME_META,
                                          NULL) > 0);
    assert(xkb_state_mod_names_are_active(state, XKB_STATE_MODS_DEPRESSED,
                                          (XKB_STATE_MATCH_ANY |
                                           XKB_STATE_MATCH_NON_EXCLUSIVE),
                                          XKB_MOD_NAME_MOD1,
                                          NULL) > 0);
    assert(xkb_state_mod_names_are_active(state, XKB_STATE_MODS_DEPRESSED,
                                          (XKB_STATE_MATCH_ANY |
                                           XKB_STATE_MATCH_NON_EXCLUSIVE),
                                          XKB_VMOD_NAME_ALT,
                                          NULL) > 0);
    assert(xkb_state_mod_names_are_active(state, XKB_STATE_MODS_DEPRESSED,
                                          (XKB_STATE_MATCH_ANY |
                                           XKB_STATE_MATCH_NON_EXCLUSIVE),
                                          XKB_VMOD_NAME_META,
                                          NULL) > 0);

    /* RAlt down */
    xkb_state_update_key(state, KEY_LEFTCTRL + EVDEV_OFFSET, XKB_KEY_UP);
    fprintf(stderr, "dumping state for RAlt down:\n");
    print_state(state);
    assert(xkb_state_mod_name_is_active(state, XKB_MOD_NAME_CTRL,
                                        XKB_STATE_MODS_EFFECTIVE) == 0);
    assert(xkb_state_mod_name_is_active(state, XKB_MOD_NAME_MOD1,
                                        XKB_STATE_MODS_DEPRESSED) > 0);
    assert(xkb_state_mod_name_is_active(state, XKB_VMOD_NAME_ALT,
                                        XKB_STATE_MODS_DEPRESSED) > 0);
    assert(xkb_state_mod_name_is_active(state, XKB_VMOD_NAME_META,
                                        XKB_STATE_MODS_DEPRESSED) > 0);
    assert(xkb_state_mod_names_are_active(state, XKB_STATE_MODS_DEPRESSED,
                                          XKB_STATE_MATCH_ANY,
                                          XKB_MOD_NAME_CTRL,
                                          XKB_MOD_NAME_MOD1,
                                          XKB_VMOD_NAME_ALT,
                                          XKB_VMOD_NAME_META,
                                          NULL) > 0);
    assert(xkb_state_mod_names_are_active(state, XKB_STATE_MODS_LATCHED,
                                          XKB_STATE_MATCH_ANY,
                                          XKB_MOD_NAME_CTRL,
                                          XKB_MOD_NAME_MOD1,
                                          XKB_VMOD_NAME_ALT,
                                          XKB_VMOD_NAME_META,
                                          NULL) == 0);

    /* none down */
    xkb_state_update_key(state, KEY_RIGHTALT + EVDEV_OFFSET, XKB_KEY_UP);
    assert(xkb_state_mod_name_is_active(state, XKB_MOD_NAME_MOD1,
                                        XKB_STATE_MODS_EFFECTIVE) == 0);
    assert(xkb_state_mod_name_is_active(state, XKB_VMOD_NAME_ALT,
                                        XKB_STATE_MODS_EFFECTIVE) == 0);
    assert(xkb_state_mod_name_is_active(state, XKB_VMOD_NAME_META,
                                        XKB_STATE_MODS_EFFECTIVE) == 0);

    /* Caps locked */
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_DOWN);
    assert(xkb_state_mod_name_is_active(state, XKB_MOD_NAME_CAPS,
                                        XKB_STATE_MODS_DEPRESSED) > 0);
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_UP);
    fprintf(stderr, "dumping state for Caps Lock:\n");
    print_state(state);
    assert(xkb_state_mod_name_is_active(state, XKB_MOD_NAME_CAPS,
                                        XKB_STATE_MODS_DEPRESSED) == 0);
    assert(xkb_state_mod_name_is_active(state, XKB_MOD_NAME_CAPS,
                                        XKB_STATE_MODS_LOCKED) > 0);
    assert(xkb_state_led_name_is_active(state, XKB_LED_NAME_CAPS) > 0);
    num_syms = xkb_state_key_get_syms(state, KEY_Q + EVDEV_OFFSET, &syms);
    assert(num_syms == 1 && syms[0] == XKB_KEY_Q);

    /* Num Lock locked */
    xkb_state_update_key(state, KEY_NUMLOCK + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_NUMLOCK + EVDEV_OFFSET, XKB_KEY_UP);
    fprintf(stderr, "dumping state for Caps Lock + Num Lock:\n");
    print_state(state);
    assert(xkb_state_mod_name_is_active(state, XKB_MOD_NAME_CAPS,
                                        XKB_STATE_MODS_LOCKED) > 0);
    assert(xkb_state_mod_name_is_active(state, XKB_MOD_NAME_MOD2,
                                        XKB_STATE_MODS_LOCKED) > 0);
    assert(xkb_state_mod_name_is_active(state, XKB_VMOD_NAME_NUM,
                                        XKB_STATE_MODS_LOCKED) > 0);
    num_syms = xkb_state_key_get_syms(state, KEY_KP1 + EVDEV_OFFSET, &syms);
    assert(num_syms == 1 && syms[0] == XKB_KEY_KP_1);
    assert(xkb_state_led_name_is_active(state, XKB_LED_NAME_NUM) > 0);

    /* Num Lock unlocked */
    xkb_state_update_key(state, KEY_NUMLOCK + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_NUMLOCK + EVDEV_OFFSET, XKB_KEY_UP);

    /* Switch to group 2 */
    xkb_state_update_key(state, KEY_COMPOSE + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_COMPOSE + EVDEV_OFFSET, XKB_KEY_UP);
    assert(xkb_state_led_name_is_active(state, "Group 2") > 0);
    assert(xkb_state_led_name_is_active(state, XKB_LED_NAME_NUM) == 0);

    /* Switch back to group 1. */
    xkb_state_update_key(state, KEY_COMPOSE + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_COMPOSE + EVDEV_OFFSET, XKB_KEY_UP);

    /* Caps unlocked */
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_UP);
    assert(xkb_state_mod_name_is_active(state, XKB_MOD_NAME_CAPS,
                                        XKB_STATE_MODS_EFFECTIVE) == 0);
    assert(xkb_state_led_name_is_active(state, XKB_LED_NAME_CAPS) == 0);
    num_syms = xkb_state_key_get_syms(state, KEY_Q + EVDEV_OFFSET, &syms);
    assert(num_syms == 1 && syms[0] == XKB_KEY_q);

    /* Multiple symbols */
    num_syms = xkb_state_key_get_syms(state, KEY_6 + EVDEV_OFFSET, &syms);
    assert(num_syms == 5 &&
           syms[0] == XKB_KEY_H && syms[1] == XKB_KEY_E &&
           syms[2] == XKB_KEY_L && syms[3] == XKB_KEY_L &&
           syms[4] == XKB_KEY_O);
    one_sym = xkb_state_key_get_one_sym(state, KEY_6 + EVDEV_OFFSET);
    assert(one_sym == XKB_KEY_NoSymbol);
    xkb_state_update_key(state, KEY_6 + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_6 + EVDEV_OFFSET, XKB_KEY_UP);

    one_sym = xkb_state_key_get_one_sym(state, KEY_5 + EVDEV_OFFSET);
    assert(one_sym == XKB_KEY_5);

    xkb_state_unref(state);
}

enum test_entry_input_type {
    INPUT_TYPE_RESET = 0,
    INPUT_TYPE_COMPONENTS,
    INPUT_TYPE_KEY,
};

struct test_state_components {
    enum test_entry_input_type input_type;
    union {
        struct {
            bool affect_latched_group;
            int32_t latched_group;
            bool affect_locked_group;
            int32_t locked_group;
            xkb_mod_mask_t affect_latched_mods;
            xkb_mod_mask_t latched_mods;
            xkb_mod_mask_t affect_locked_mods;
            xkb_mod_mask_t locked_mods;
        } input;
        struct {
            xkb_keycode_t keycode;
            enum key_seq_state direction;
            xkb_keysym_t keysym;
        } key;
    };

    /* Same as state_components, but it is not public */
    int32_t base_group; /**< depressed */
    int32_t latched_group;
    int32_t locked_group;
    xkb_layout_index_t group; /**< effective */
    xkb_mod_mask_t base_mods; /**< depressed */
    xkb_mod_mask_t latched_mods;
    xkb_mod_mask_t locked_mods;
    xkb_mod_mask_t mods; /**< effective */
    xkb_led_mask_t leds;

    enum xkb_state_component changes;
};

#define COMPONENTS_ENTRY(...) \
    .input_type = INPUT_TYPE_COMPONENTS, .input = { __VA_ARGS__ }

#define KEY_ENTRY(_keycode, _direction, _keysym) \
    .input_type = INPUT_TYPE_KEY,                \
    .key = { .keycode=_keycode + EVDEV_OFFSET,   \
             .direction=_direction,              \
             .keysym=_keysym }

#define RESET_STATE { .input_type = INPUT_TYPE_RESET }

static bool
check_update_state(struct xkb_keymap *keymap,
                   const struct test_state_components *components,
                   struct xkb_state *expected, struct xkb_state *got,
                   xkb_keysym_t keysym, enum xkb_state_component changes)
{
    xkb_state_update_mask(expected,
                          mod_mask_get_effective(keymap, components->base_mods),
                          mod_mask_get_effective(keymap, components->latched_mods),
                          mod_mask_get_effective(keymap, components->locked_mods),
                          components->base_group,
                          components->latched_group, components->locked_group);

    if (changes != components->changes) {
        fprintf(stderr, "Expected state change: %u, but got: %u\n",
                components->changes, changes);
        fprintf(stderr, "Expected state:\n");
        print_state(expected);
        fprintf(stderr, "Got state:\n");
        print_state(got);
        return false;
    } else if (components->input_type == INPUT_TYPE_KEY) {
        if (keysym != components->key.keysym) {
            char buf[XKB_KEYSYM_NAME_MAX_SIZE];
            xkb_keysym_get_name(components->key.keysym, buf, sizeof(buf));
            fprintf(stderr, "Expected keysym: %s, ", buf);
            xkb_keysym_get_name(keysym, buf, sizeof(buf));
            fprintf(stderr, "but got: %s\n", buf);
            return false;
        }
    } else if (keysym != XKB_KEY_NoSymbol) {
        return false;
    }
    return check_state(expected, got);
}

static void
test_update_latched_locked(struct xkb_keymap *keymap)
{
    struct xkb_state *state = xkb_state_new(keymap);
    struct xkb_state *expected = xkb_state_new(keymap);
    assert(state);
    assert(expected);

    xkb_mod_index_t shift_idx    = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_SHIFT);
    xkb_mod_index_t capslock_idx = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_CAPS);
    xkb_mod_index_t control_idx  = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_CTRL);
    xkb_mod_index_t level3_idx   = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_LEVEL3);
    xkb_mod_mask_t shift    = 1u << shift_idx;
    xkb_mod_mask_t capslock = 1u << capslock_idx;
    xkb_mod_mask_t control  = 1u << control_idx;
    xkb_mod_mask_t level3   = 1u << level3_idx;
    xkb_led_index_t capslock_led_idx = _xkb_keymap_led_get_index(keymap, XKB_LED_NAME_CAPS);
    xkb_led_index_t group2_led_idx   = _xkb_keymap_led_get_index(keymap, "Group 2");
    xkb_led_mask_t capslock_led = 1u << capslock_led_idx;
    xkb_led_mask_t group2_led   = 1u << group2_led_idx;

    const struct test_state_components test_data[] = {
        { KEY_ENTRY(KEY_A, BOTH, XKB_KEY_a), .changes = 0 },

        /*
         * Groups: lock
         */
#define GROUP_LOCK_ENTRY(group) \
        COMPONENTS_ENTRY(.affect_locked_group = true, .locked_group = group)
#define GROUP_LOCK_CHANGES \
        (XKB_STATE_LAYOUT_LOCKED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS)

        { GROUP_LOCK_ENTRY(1), .locked_group=1, .group=1, .leds=group2_led, .changes=GROUP_LOCK_CHANGES },
        { KEY_ENTRY(KEY_A, BOTH, XKB_KEY_Cyrillic_ef), .locked_group=1, .group=1, .leds = group2_led, .changes = 0 },
        { GROUP_LOCK_ENTRY(0), .locked_group=0, .group=0, .leds=0, .changes=GROUP_LOCK_CHANGES },
        { GROUP_LOCK_ENTRY(0), .locked_group=0, .group=0, .leds=0, .changes=0 },
        { GROUP_LOCK_ENTRY(1), .locked_group=1, .group=1, .leds=group2_led, .changes=GROUP_LOCK_CHANGES },
        { GROUP_LOCK_ENTRY(1), .locked_group=1, .group=1, .leds=group2_led, .changes=0 },
        /* Invalid group */
        { GROUP_LOCK_ENTRY(XKB_MAX_GROUPS), .locked_group=0, .group=0, .leds=0, .changes=GROUP_LOCK_CHANGES },
        /* Previous lock */
        { KEY_ENTRY(KEY_COMPOSE, DOWN, XKB_KEY_ISO_Next_Group),
          .locked_group = 1, .group = 1, .leds = group2_led,
          .changes = XKB_STATE_LAYOUT_LOCKED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS },
        { KEY_ENTRY(KEY_COMPOSE, UP, XKB_KEY_ISO_Next_Group),
          .locked_group = 1, .group = 1, .leds = group2_led,
          .changes = 0 },
        { KEY_ENTRY(KEY_A, BOTH, XKB_KEY_Cyrillic_ef), .locked_group=1, .group=1, .leds = group2_led, .changes = 0 },
        { GROUP_LOCK_ENTRY(0), .locked_group=0, .group=0, .leds=group2_led,
          .changes=XKB_STATE_LAYOUT_LOCKED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS },
        { KEY_ENTRY(KEY_A, BOTH, XKB_KEY_a), .changes = 0 },
        { KEY_ENTRY(KEY_COMPOSE, DOWN, XKB_KEY_ISO_Next_Group),
          .locked_group = 1, .group = 1, .leds = group2_led,
          .changes = XKB_STATE_LAYOUT_LOCKED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS },
        { KEY_ENTRY(KEY_COMPOSE, UP, XKB_KEY_ISO_Next_Group),
          .locked_group = 1, .group = 1, .leds = group2_led,
          .changes = 0 },

        /*
         * Groups: latch
         */
#define GROUP_LATCH_ENTRY(group) \
        COMPONENTS_ENTRY(.affect_latched_group = true, .latched_group = group)

        RESET_STATE,
        { KEY_ENTRY(KEY_A, BOTH, XKB_KEY_a), .base_group = 0, .latched_group = 0, .locked_group = 0, .group = 0, .base_mods = 0, .latched_mods = 0, .locked_mods = 0, .mods = 0, .leds = 0, .changes = 0 },
        { GROUP_LATCH_ENTRY(1), .latched_group=1, .group=1, .leds=group2_led, .changes=XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS },
        { KEY_ENTRY(KEY_A, DOWN, XKB_KEY_Cyrillic_ef), .changes = XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS },
        { KEY_ENTRY(KEY_A, UP, XKB_KEY_a), .changes = 0 },
        { KEY_ENTRY(KEY_A, DOWN, XKB_KEY_a), .changes = 0 },
        { KEY_ENTRY(KEY_A, UP, XKB_KEY_a), .changes = 0 },
        { GROUP_LATCH_ENTRY(1), .latched_group=1, .group=1, .leds=group2_led, .changes=XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS },
        { GROUP_LATCH_ENTRY(1), .latched_group=1, .group=1, .leds=group2_led, .changes=0 },
        { KEY_ENTRY(KEY_A, DOWN, XKB_KEY_Cyrillic_ef), .changes = XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS },
        { KEY_ENTRY(KEY_A, UP, XKB_KEY_a), .changes = 0 },
        { KEY_ENTRY(KEY_A, DOWN, XKB_KEY_a), .changes = 0 },
        { KEY_ENTRY(KEY_A, UP, XKB_KEY_a), .changes = 0 },
        /* Invalid group */
        { GROUP_LATCH_ENTRY(XKB_MAX_GROUPS), .latched_group=XKB_MAX_GROUPS, .group=0, .leds=0, .changes=XKB_STATE_LAYOUT_LATCHED },
        /* Pending latch is cancelled */
        RESET_STATE,
        { KEY_ENTRY(KEY_LEFTMETA, BOTH, XKB_KEY_ISO_Group_Latch),
          .latched_group = 1, .group = 1, .leds = group2_led,
          .changes = XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_DEPRESSED },
        { GROUP_LATCH_ENTRY(2), .latched_group=2, .group=0, .changes=XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS },
        { KEY_ENTRY(KEY_A, DOWN, XKB_KEY_a), .changes = XKB_STATE_LAYOUT_LATCHED },
        /* Pending latch to lock is cancelled */
        RESET_STATE,
        { KEY_ENTRY(KEY_RIGHTMETA, BOTH, XKB_KEY_ISO_Group_Latch),
          .latched_group = 1, .group = 1, .leds = group2_led,
          .changes = XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_DEPRESSED },
        { GROUP_LATCH_ENTRY(2), .latched_group=2, .group=0, .changes=XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS },
        { KEY_ENTRY(KEY_A, DOWN, XKB_KEY_a), .changes = XKB_STATE_LAYOUT_LATCHED },

        /*
         * Groups: latch + lock
         */
        RESET_STATE,
        /* Empty state */
        { COMPONENTS_ENTRY(.affect_latched_group = true, .latched_group = 1,
                           .affect_locked_group = true, .locked_group = 1),
          .latched_group = 1, .locked_group = 1, .group = 0,
          .changes = XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_LOCKED },
        /* Pending latch */
        RESET_STATE,
        { KEY_ENTRY(KEY_LEFTMETA, BOTH, XKB_KEY_ISO_Group_Latch),
          .latched_group = 1, .group = 1, .leds = group2_led,
          .changes = XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_DEPRESSED },
        { COMPONENTS_ENTRY(.affect_locked_group = true, .locked_group = 1),
          .latched_group = 1, .locked_group = 1, .group = 0,
          .changes = XKB_STATE_LAYOUT_LOCKED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS },
        { KEY_ENTRY(KEY_A, DOWN, XKB_KEY_a),
          .locked_group=1, .group=1, .leds = group2_led,
          .changes = XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS },

        /*
         * Modifiers: lock
         */
#define MOD_LOCK_ENTRY(mask, mods) \
        COMPONENTS_ENTRY(.affect_locked_mods = mask, .locked_mods = mods)
#define UNDEFINED_MODMASK (1u << (XKB_MAX_MODS - 1))

        RESET_STATE,
        /* Invalid: mod not in the mask */
        { MOD_LOCK_ENTRY(0, capslock), .changes=0 },
        { MOD_LOCK_ENTRY(0, UNDEFINED_MODMASK), .changes=0 },
        /* Set Caps */
        { MOD_LOCK_ENTRY(capslock, capslock), .locked_mods=capslock, .mods=capslock, .leds=capslock_led, .changes=XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE | XKB_STATE_LEDS },
        { MOD_LOCK_ENTRY(capslock, capslock), .locked_mods=capslock, .mods=capslock, .leds=capslock_led, .changes=0 },
        { KEY_ENTRY(KEY_A, BOTH, XKB_KEY_A), .locked_mods=capslock, .mods=capslock, .leds=capslock_led, .changes = 0 },
        /* Add Control and keep Caps */
        { MOD_LOCK_ENTRY(control, control), .locked_mods=control | capslock, .mods=control | capslock, .leds=capslock_led, .changes=XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE },
        { KEY_ENTRY(KEY_A, BOTH, XKB_KEY_A), .locked_mods=control | capslock, .mods=control | capslock, .leds=capslock_led, .changes = 0 },
        /* Remove Caps and keep Control */
        { MOD_LOCK_ENTRY(capslock, 0), .locked_mods=control, .mods=control, .changes=XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE | XKB_STATE_LEDS },
        { KEY_ENTRY(KEY_A, BOTH, XKB_KEY_a), .locked_mods=control, .mods=control, .leds=0, .changes = 0 },
        /* Add Level3 and remove Control */
        { MOD_LOCK_ENTRY(level3 | control, level3), .locked_mods=level3, .mods=level3, .changes=XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE },
        /* Change undefined modifier */
        { MOD_LOCK_ENTRY(level3, level3 | UNDEFINED_MODMASK), .locked_mods=level3, .mods=level3, .changes=0 },
        { MOD_LOCK_ENTRY(level3 | UNDEFINED_MODMASK, level3 | UNDEFINED_MODMASK), .locked_mods=level3, .mods=level3, .changes=0 },
        { MOD_LOCK_ENTRY(level3 | UNDEFINED_MODMASK, level3), .locked_mods=level3, .mods=level3, .changes=0 },
        /* Previous lock */
        RESET_STATE,
        { KEY_ENTRY(KEY_CAPSLOCK, BOTH, XKB_KEY_Caps_Lock),
          .locked_mods=capslock, .mods=capslock, .leds=capslock_led,
          .changes = XKB_STATE_MODS_DEPRESSED },
        { MOD_LOCK_ENTRY(level3 | control, level3),
          .locked_mods=capslock | level3, .mods=capslock | level3, .leds=capslock_led,
          .changes=XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE },
        { MOD_LOCK_ENTRY(capslock, 0),
          .locked_mods=level3, .mods=level3, .leds=0,
          .changes=XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE | XKB_STATE_LEDS },

        /*
         * Modifiers: latch
         */
#define MODS_LATCH_ENTRY(mask, mods) \
        COMPONENTS_ENTRY(.affect_latched_mods = mask, .latched_mods = mods)

        RESET_STATE,
        { KEY_ENTRY(KEY_A, BOTH, XKB_KEY_a), .changes = 0 },
        /* Invalid: mod not in the mask */
        { MODS_LATCH_ENTRY(0, shift), .changes=0 },
        { MODS_LATCH_ENTRY(0, UNDEFINED_MODMASK), .changes=0 },
        /* Latch Shift */
        { MODS_LATCH_ENTRY(shift, shift), .latched_mods=shift, .mods=shift, .changes=XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE },
        { KEY_ENTRY(KEY_A, DOWN, XKB_KEY_A), .changes = XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE },
        { KEY_ENTRY(KEY_A, UP, XKB_KEY_a), .changes = 0 },
        { KEY_ENTRY(KEY_A, BOTH, XKB_KEY_a), .changes = 0 },
        { MODS_LATCH_ENTRY(shift, shift), .latched_mods=shift, .mods=shift, .changes=XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE },
        { MODS_LATCH_ENTRY(shift, shift), .latched_mods=shift, .mods=shift, .changes=0 },
        { KEY_ENTRY(KEY_A, DOWN, XKB_KEY_A), .changes = XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE },
        { KEY_ENTRY(KEY_A, UP, XKB_KEY_a), .changes = 0 },
        { KEY_ENTRY(KEY_A, BOTH, XKB_KEY_a), .changes = 0 },
        /* Latch Shift, then Caps: latched shift is cancelled */
        { MODS_LATCH_ENTRY(shift, shift), .latched_mods=shift, .mods=shift, .changes=XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE },
        { MODS_LATCH_ENTRY(capslock, capslock), .latched_mods=shift | capslock, .mods=shift | capslock, .changes=XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE },
        { KEY_ENTRY(KEY_A, DOWN, XKB_KEY_a), .changes = XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE },
        { KEY_ENTRY(KEY_A, UP, XKB_KEY_a), .changes = 0 },
        /* Change undefined modifier */
        { MODS_LATCH_ENTRY(level3, level3 | UNDEFINED_MODMASK), .latched_mods=level3, .mods=level3, .changes=XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE },
        { MODS_LATCH_ENTRY(level3 | UNDEFINED_MODMASK, level3 | UNDEFINED_MODMASK), .latched_mods=level3, .mods=level3, .changes=0 },
        { MODS_LATCH_ENTRY(level3 | UNDEFINED_MODMASK, level3), .latched_mods=level3, .mods=level3, .changes=0 },
        /* Pending latch is *not* cancelled if not in affected mods */
        RESET_STATE,
        { KEY_ENTRY(KEY_102ND, BOTH, XKB_KEY_ISO_Level3_Latch),
          .latched_mods=level3, .mods=level3,
          .changes = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED },
        { MODS_LATCH_ENTRY(shift, shift),
          .latched_mods=shift | level3, .mods=shift | level3,
          .changes=XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE },
        { KEY_ENTRY(KEY_A, DOWN, XKB_KEY_A), .changes = XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE },
        /* Pending latch *is* cancelled if in affected mods */
        RESET_STATE,
        { KEY_ENTRY(KEY_102ND, BOTH, XKB_KEY_ISO_Level3_Latch),
          .latched_mods=level3, .mods=level3,
          .changes = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED },
        { MODS_LATCH_ENTRY(shift | level3, shift),
          .latched_mods=shift, .mods=shift,
          .changes=XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE },
        { KEY_ENTRY(KEY_A, DOWN, XKB_KEY_A), .changes = XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE },
        // TODO

        /*
         * Modifiers: latched + locked
         */
        RESET_STATE,
        { COMPONENTS_ENTRY(.affect_latched_mods = shift, .latched_mods = shift,
                           .affect_locked_mods = level3, .locked_mods = level3),
          .latched_mods = shift, .locked_mods = level3, .mods = shift | level3,
          .changes = XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE },
        // TODO

        /*
         * Mix
         */

        /* Lock mods & groups */
        RESET_STATE,
        { COMPONENTS_ENTRY(.affect_locked_group=true, .locked_group=1,
                           .affect_locked_mods=control, .locked_mods=control),
          .locked_group=1, .group=1, .locked_mods=control, .mods=control, .leds=group2_led,
          .changes = XKB_STATE_LAYOUT_LOCKED | XKB_STATE_LAYOUT_EFFECTIVE |
                     XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE | XKB_STATE_LEDS },
        /* When updating latches, mod/group changes should not affect each other */
        RESET_STATE,
        { COMPONENTS_ENTRY(.affect_latched_group=true, .latched_group=1,
                           .affect_latched_mods=control, .latched_mods=control),
          .latched_group=1, .group=1, .latched_mods=control, .mods=control,
          .leds=group2_led,
          .changes = XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_EFFECTIVE |
                     XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE | XKB_STATE_LEDS },
        { KEY_ENTRY(KEY_A, DOWN, XKB_KEY_Cyrillic_ef),
          .changes = XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS |
                     XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE },
        RESET_STATE,
        { KEY_ENTRY(KEY_LEFTMETA, BOTH, XKB_KEY_ISO_Group_Latch),
          .latched_group = 1, .group = 1, .leds = group2_led,
          .changes = XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_DEPRESSED },
        /* Pending group latch */
        { COMPONENTS_ENTRY(.affect_latched_mods=shift, .latched_mods=shift),
          .latched_group=1, .group=1,
          .latched_mods=shift, .mods=shift,
          .leds = group2_led,
          .changes = XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE },
        { KEY_ENTRY(KEY_A, DOWN, XKB_KEY_Cyrillic_EF),
          .leds = group2_led,
          .changes = XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_EFFECTIVE |
                     XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE | XKB_STATE_LEDS },
        { KEY_ENTRY(KEY_RIGHTMETA, BOTH, XKB_KEY_ISO_Group_Latch),
          .latched_group = 1, .group = 1, .leds = group2_led,
          .changes = XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_DEPRESSED },
        /* Pending group latch (with latch to lock + clear) */
        { COMPONENTS_ENTRY(.affect_latched_mods=shift, .latched_mods=shift),
          .latched_group=1, .group=1,
          .latched_mods=shift, .mods=shift,
          .leds = group2_led,
          .changes = XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE },
        { KEY_ENTRY(KEY_A, DOWN, XKB_KEY_Cyrillic_EF),
          .leds = group2_led,
          .changes = XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_EFFECTIVE |
                     XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE | XKB_STATE_LEDS },
        // TODO
    };
    for (size_t k = 0; k < ARRAY_SIZE(test_data); k++) {
        xkb_keysym_t keysym = XKB_KEY_NoSymbol;
        enum xkb_state_component changes = 0;
        switch (test_data[k].input_type) {
            case INPUT_TYPE_COMPONENTS:
                changes = xkb_state_update_latched_locked(
                            state,
                            test_data[k].input.affect_latched_mods,
                            test_data[k].input.latched_mods,
                            test_data[k].input.affect_latched_group,
                            test_data[k].input.latched_group,
                            test_data[k].input.affect_locked_mods,
                            test_data[k].input.locked_mods,
                            test_data[k].input.affect_locked_group,
                            test_data[k].input.locked_group);
                break;
            case INPUT_TYPE_KEY:
                keysym = xkb_state_key_get_one_sym(state, test_data[k].key.keycode);
                if (test_data[k].key.direction == DOWN ||
                    test_data[k].key.direction == BOTH)
                    changes = xkb_state_update_key(state, test_data[k].key.keycode, XKB_KEY_DOWN);
                if (test_data[k].key.direction == UP ||
                    test_data[k].key.direction == BOTH)
                    changes = xkb_state_update_key(state, test_data[k].key.keycode, XKB_KEY_UP);
                break;
            case INPUT_TYPE_RESET:
                xkb_state_unref(state);
                xkb_state_unref(expected);
                state = xkb_state_new(keymap);
                expected = xkb_state_new(keymap);
                continue;
            default:
                assert(false);
        }
        assert_printf(check_update_state(keymap, &test_data[k], expected, state, keysym, changes),
                      "test_update_latched_locked #%zu: type: %d\n", k, test_data[k].input_type);
    }

    xkb_state_unref(expected);
    xkb_state_unref(state);
#undef COMPONENTS_ENTRY
}

struct test_active_mods_entry {
    xkb_mod_mask_t state;
    xkb_mod_mask_t active;
};

static void
test_serialisation(struct xkb_keymap *keymap)
{
    struct xkb_state *state = xkb_state_new(keymap);
    xkb_mod_mask_t base_mods;
    xkb_mod_mask_t latched_mods;
    xkb_mod_mask_t locked_mods;
    xkb_mod_mask_t effective_mods;
    xkb_layout_index_t base_group = 0;
    xkb_layout_index_t latched_group = 0;
    xkb_layout_index_t locked_group = 0;

    assert(state);

    xkb_mod_index_t shiftIdx  = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_SHIFT);
    xkb_mod_index_t capsIdx   = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_CAPS);
    xkb_mod_index_t ctrlIdx   = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_CTRL);
    xkb_mod_index_t mod1Idx   = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_MOD1);
    xkb_mod_index_t mod2Idx   = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_MOD2);
    xkb_mod_index_t mod3Idx   = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_MOD3);
    xkb_mod_index_t mod4Idx   = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_MOD4);
    xkb_mod_index_t mod5Idx   = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_MOD5);
    xkb_mod_index_t altIdx    = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_ALT);
    xkb_mod_index_t metaIdx   = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_META);
    xkb_mod_index_t superIdx  = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_SUPER);
    xkb_mod_index_t hyperIdx  = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_HYPER);
    xkb_mod_index_t numIdx    = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_NUM);
    xkb_mod_index_t level3Idx = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_LEVEL3);
    xkb_mod_index_t altGrIdx  = _xkb_keymap_mod_get_index(keymap, "AltGr");
    xkb_mod_mask_t shift  = (1u << shiftIdx);
    xkb_mod_mask_t caps   = (1u << capsIdx);
    xkb_mod_mask_t ctrl   = (1u << ctrlIdx);
    xkb_mod_mask_t mod1   = (1u << mod1Idx);
    xkb_mod_mask_t mod2   = (1u << mod2Idx);
    xkb_mod_mask_t mod3   = (1u << mod3Idx);
    xkb_mod_mask_t mod4   = (1u << mod4Idx);
    xkb_mod_mask_t mod5   = (1u << mod5Idx);
    xkb_mod_mask_t alt    = (1u << altIdx);
    xkb_mod_mask_t meta   = (1u << metaIdx);
    xkb_mod_mask_t super  = (1u << superIdx);
    xkb_mod_mask_t hyper  = (1u << hyperIdx);
    xkb_mod_mask_t num    = (1u << numIdx);
    xkb_mod_mask_t level3 = (1u << level3Idx);
    xkb_mod_mask_t altGr  = (1u << altGrIdx);

    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_UP);
    base_mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_DEPRESSED);
    assert(base_mods == 0);
    latched_mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_LATCHED);
    assert(latched_mods == 0);
    locked_mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_LOCKED);
    assert(locked_mods == caps);
    effective_mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
    assert(effective_mods == locked_mods);

    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
    base_mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_DEPRESSED);
    assert(base_mods == shift);
    latched_mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_LATCHED);
    assert(latched_mods == 0);
    locked_mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_LOCKED);
    assert(locked_mods == caps);
    effective_mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
    assert(effective_mods == (base_mods | locked_mods));

    base_mods |= ctrl;
    xkb_state_update_mask(state, base_mods, latched_mods, locked_mods,
                          base_group, latched_group, locked_group);

    assert(xkb_state_mod_index_is_active(state, ctrlIdx, XKB_STATE_MODS_DEPRESSED) > 0);
    assert(xkb_state_mod_index_is_active(state, ctrlIdx, XKB_STATE_MODS_EFFECTIVE) > 0);

    const struct test_active_mods_entry test_data[] = {
        { .state = 0,            .active = 0                         },
        { .state = shift,        .active = shift                     },
        { .state = caps,         .active = caps                      },
        { .state = ctrl,         .active = ctrl                      },
        { .state = mod1,         .active = mod1 | alt | meta         },
        { .state = mod2,         .active = mod2 | num                },
        { .state = mod3,         .active = mod3                      },
        { .state = mod4,         .active = mod4 | super | hyper      },
        { .state = mod5,         .active = mod5 | level3 | altGr     },
        { .state = shift | mod1, .active = shift | mod1 | alt | meta },
        { .state = shift | mod2, .active = shift | mod2 | num        },
    };

    for (unsigned k = 0; k < ARRAY_SIZE(test_data); k++) {
        const struct test_active_mods_entry *entry = &test_data[k];
#define check_mods(keymap, state, entry, type)                                      \
        for (xkb_mod_index_t idx = 0; idx < xkb_keymap_num_mods(keymap); idx++) {   \
            xkb_mod_mask_t mask = 1u << idx;                                        \
            bool expected = !!(mask & entry->active);                               \
            bool got = !!xkb_state_mod_index_is_active(state, idx, type);           \
            fprintf(stderr, "#%u State 0x%x, mod: %u: expected %u, got: %u\n",      \
                    k, entry->state, idx, expected, got);                           \
            assert_printf(got == expected,                                          \
                          "xkb_state_mod_index_is_active, " STRINGIFY2(type) "\n"); \
            got = !!xkb_state_mod_index_is_active(state, idx,                       \
                                                  XKB_STATE_MODS_EFFECTIVE);        \
            assert_printf(got == expected, "xkb_state_mod_index_is_active, "        \
                          STRINGIFY2(XKB_STATE_MODS_EFFECTIVE) "\n");               \
            got = !!xkb_state_mod_indices_are_active(                               \
                        state, type,                                                \
                        XKB_STATE_MATCH_ALL | XKB_STATE_MATCH_NON_EXCLUSIVE,        \
                        idx, XKB_MOD_INVALID);                                      \
            assert_printf(got == expected, "xkb_state_mod_indices_are_active, "     \
                          STRINGIFY2(type) "\n");                                   \
            got = !!xkb_state_mod_indices_are_active(                               \
                        state, XKB_STATE_MODS_EFFECTIVE,                            \
                        XKB_STATE_MATCH_ALL | XKB_STATE_MATCH_NON_EXCLUSIVE,        \
                        idx, XKB_MOD_INVALID);                                      \
            assert_printf(got == expected, "xkb_state_mod_indices_are_active, "     \
                          STRINGIFY2(XKB_STATE_MODS_EFFECTIVE) "\n");               \
        }
        xkb_state_update_mask(state, entry->state, 0, 0, 0, 0, 0);
        check_mods(keymap, state, entry, XKB_STATE_MODS_DEPRESSED);
        xkb_state_update_mask(state, 0, entry->state, 0, 0, 0, 0);
        check_mods(keymap, state, entry, XKB_STATE_MODS_LATCHED);
        xkb_state_update_mask(state, 0, 0, entry->state, 0, 0, 0);
        check_mods(keymap, state, entry, XKB_STATE_MODS_LOCKED);
    }

    xkb_state_unref(state);
}

static void
test_update_mask_mods(struct xkb_keymap *keymap)
{
    enum xkb_state_component changed;
    struct xkb_state *state = xkb_state_new(keymap);
    assert(state);

    xkb_mod_index_t capsIdx  = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_CAPS);
    xkb_mod_index_t shiftIdx = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_SHIFT);
    xkb_mod_index_t mod1Idx  = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_MOD1);
    xkb_mod_index_t mod2Idx  = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_MOD2);
    xkb_mod_index_t altIdx   = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_ALT);
    xkb_mod_index_t metaIdx  = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_META);
    xkb_mod_index_t numIdx   = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_NUM);
    xkb_mod_mask_t caps  = (1u << capsIdx);
    xkb_mod_mask_t shift = (1u << shiftIdx);
    xkb_mod_mask_t mod1  = (1u << mod1Idx);
    xkb_mod_mask_t mod2  = (1u << mod2Idx);
    xkb_mod_mask_t alt   = (1u << altIdx);
    xkb_mod_mask_t meta  = (1u << metaIdx);
    xkb_mod_mask_t num   = (1u << numIdx);

    changed = xkb_state_update_mask(state, caps, 0, 0, 0, 0, 0);
    assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE));
    assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE) == caps);

    changed = xkb_state_update_mask(state, caps, 0, shift, 0, 0, 0);
    assert(changed == (XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE |
                       XKB_STATE_LEDS));
    assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE) ==
           (caps | shift));
    assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_DEPRESSED) == caps);
    assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_LATCHED) == 0);
    assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_LOCKED) == shift);

    changed = xkb_state_update_mask(state, 0, 0, 0, 0, 0, 0);
    assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LOCKED |
                       XKB_STATE_MODS_EFFECTIVE | XKB_STATE_LEDS));
    assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE) == 0);

    changed = xkb_state_update_mask(state, alt, 0, 0, 0, 0, 0);
    assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE));
    assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE) ==
           (alt | mod1));

    changed = xkb_state_update_mask(state, meta, 0, 0, 0, 0, 0);
    assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE));
    assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE) ==
           (meta | mod1));

    changed = xkb_state_update_mask(state, 0, 0, num, 0, 0, 0);
    assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LOCKED |
                       XKB_STATE_MODS_EFFECTIVE | XKB_STATE_LEDS));
    assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE) ==
           (num | mod2));

    xkb_state_update_mask(state, 0, 0, 0, 0, 0, 0);

    changed = xkb_state_update_mask(state, mod2, 0, num, 0, 0, 0);
    assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LOCKED |
                       XKB_STATE_MODS_EFFECTIVE | XKB_STATE_LEDS));
    assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE) ==
           (mod2 | num));
    assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_DEPRESSED) ==
           mod2);
    assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_LOCKED) ==
           (num | mod2));

    xkb_state_unref(state);
}

static void
test_repeat(struct xkb_keymap *keymap)
{
    assert(!xkb_keymap_key_repeats(keymap, KEY_LEFTSHIFT + EVDEV_OFFSET));
    assert(xkb_keymap_key_repeats(keymap, KEY_A + EVDEV_OFFSET));
    assert(xkb_keymap_key_repeats(keymap, KEY_8 + EVDEV_OFFSET));
    assert(xkb_keymap_key_repeats(keymap, KEY_DOWN + EVDEV_OFFSET));
    assert(xkb_keymap_key_repeats(keymap, KEY_KBDILLUMDOWN + EVDEV_OFFSET));
}

static void
test_consume(struct xkb_keymap *keymap)
{
    xkb_mod_mask_t mask;
    xkb_mod_index_t shift = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_SHIFT);
    xkb_mod_index_t caps  = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_CAPS);
    xkb_mod_index_t ctrl  = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_CTRL);
    xkb_mod_index_t mod1  = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_MOD1);
    xkb_mod_index_t mod2  = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_MOD2);
    xkb_mod_index_t mod5  = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_MOD5);
    xkb_mod_index_t alt   = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_ALT);
    xkb_mod_index_t meta  = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_META);
    struct xkb_state *state = xkb_state_new(keymap);
    assert(state);

    /* Test remove_consumed() */
    xkb_state_update_key(state, KEY_LEFTALT + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_EQUAL + EVDEV_OFFSET, XKB_KEY_DOWN);

    fprintf(stderr, "dumping state for Alt-Shift-+\n");
    print_state(state);

    mask = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
    assert(mask == ((1U << mod1) | (1U << shift)));
    mask = xkb_state_mod_mask_remove_consumed(state, KEY_EQUAL + EVDEV_OFFSET,
                                              mask);
    assert(mask == (1U << mod1));

    /* Test get_consumed_mods() */
    mask = xkb_state_key_get_consumed_mods(state, KEY_EQUAL + EVDEV_OFFSET);
    assert(mask == (1U << shift));

    mask = xkb_state_key_get_consumed_mods(state, KEY_ESC + EVDEV_OFFSET);
    assert(mask == 0);

    assert(xkb_state_mod_index_is_consumed(state, KEY_EQUAL + EVDEV_OFFSET, shift) > 0);
    assert(xkb_state_mod_index_is_consumed(state, KEY_EQUAL + EVDEV_OFFSET, mod1) == 0);
    assert(xkb_state_mod_index_is_consumed(state, KEY_EQUAL + EVDEV_OFFSET, alt) == 0);
    assert(xkb_state_mod_index_is_consumed(state, KEY_EQUAL + EVDEV_OFFSET, meta) == 0);

    xkb_state_unref(state);

    /* Test is_consumed() - simple ALPHABETIC type. */
    state = xkb_state_new(keymap);
    assert(state);

    mask = xkb_state_key_get_consumed_mods(state, KEY_A + EVDEV_OFFSET);
    assert(mask == ((1U << shift) | (1U << caps)));

    assert(xkb_state_mod_index_is_consumed(state, KEY_A + EVDEV_OFFSET, caps) > 0);
    assert(xkb_state_mod_index_is_consumed(state, KEY_A + EVDEV_OFFSET, shift) > 0);
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_UP);
    assert(xkb_state_mod_index_is_consumed(state, KEY_A + EVDEV_OFFSET, caps) > 0);
    assert(xkb_state_mod_index_is_consumed(state, KEY_A + EVDEV_OFFSET, shift) > 0);
    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
    assert(xkb_state_mod_index_is_consumed(state, KEY_A + EVDEV_OFFSET, caps) > 0);
    assert(xkb_state_mod_index_is_consumed(state, KEY_A + EVDEV_OFFSET, shift) > 0);
    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_UP);
    assert(xkb_state_mod_index_is_consumed(state, KEY_A + EVDEV_OFFSET, caps) > 0);
    assert(xkb_state_mod_index_is_consumed(state, KEY_A + EVDEV_OFFSET, shift) > 0);

    xkb_state_unref(state);

    /* More complicated - CTRL+ALT */
    state = xkb_state_new(keymap);
    assert(state);

    mask = xkb_state_key_get_consumed_mods(state, KEY_F1 + EVDEV_OFFSET);
    assert(mask == ((1U << shift) | (1U << mod1) | (1U << ctrl) | (1U << mod5)));

    /* Shift is preserved. */
    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
    mask = xkb_state_key_get_consumed_mods(state, KEY_F1 + EVDEV_OFFSET);
    assert(mask == ((1U << mod1) | (1U << ctrl) | (1U << mod5)));
    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);

    mask = xkb_state_key_get_consumed_mods(state, KEY_F1 + EVDEV_OFFSET);
    assert(mask == ((1U << shift) | (1U << mod1) | (1U << ctrl) | (1U << mod5)));

    xkb_state_unref(state);

    /* Test XKB_CONSUMED_MODE_GTK, CTRL+ALT */
    state = xkb_state_new(keymap);
    assert(state);

    mask = xkb_state_key_get_consumed_mods2(state, KEY_F1 + EVDEV_OFFSET,
                                            XKB_CONSUMED_MODE_GTK);
    assert(mask == 0);

    xkb_state_update_key(state, KEY_LEFTCTRL + EVDEV_OFFSET, XKB_KEY_DOWN);
    mask = xkb_state_key_get_consumed_mods2(state, KEY_F1 + EVDEV_OFFSET,
                                            XKB_CONSUMED_MODE_GTK);
    assert(mask == 0);

    xkb_state_update_key(state, KEY_LEFTALT + EVDEV_OFFSET, XKB_KEY_DOWN);
    mask = xkb_state_key_get_consumed_mods2(state, KEY_F1 + EVDEV_OFFSET,
                                            XKB_CONSUMED_MODE_GTK);
    assert(mask == ((1U << mod1) | (1U << ctrl)));
    assert(xkb_state_mod_index_is_consumed(state, KEY_F1 + EVDEV_OFFSET, shift) > 0);
    assert(xkb_state_mod_index_is_consumed(state, KEY_F1 + EVDEV_OFFSET, ctrl) > 0);
    assert(xkb_state_mod_index_is_consumed(state, KEY_F1 + EVDEV_OFFSET, mod1) > 0);
    assert(xkb_state_mod_index_is_consumed(state, KEY_F1 + EVDEV_OFFSET, alt) > 0);
    assert(xkb_state_mod_index_is_consumed(state, KEY_F1 + EVDEV_OFFSET, meta) > 0);
    mask = (1U << ctrl) | (1U << mod1) | (1U << mod2);
    mask = xkb_state_mod_mask_remove_consumed(state, KEY_F1 + EVDEV_OFFSET, mask);
    assert(mask == (1U << mod2));
    mask = (1U << ctrl) | (1U << alt) | (1U << meta) | (1U << mod2);
    mask = xkb_state_mod_mask_remove_consumed(state, KEY_F1 + EVDEV_OFFSET, mask);
    assert(mask == (1U << mod2));

    xkb_state_unref(state);

    /* Test XKB_CONSUMED_MODE_GTK, Simple Shift */
    state = xkb_state_new(keymap);
    assert(state);

    mask = xkb_state_key_get_consumed_mods2(state, KEY_A + EVDEV_OFFSET,
                                            XKB_CONSUMED_MODE_GTK);
    assert(mask == ((1U << shift) | (1U << caps)));

    xkb_state_update_key(state, KEY_LEFTALT + EVDEV_OFFSET, XKB_KEY_DOWN);
    mask = xkb_state_key_get_consumed_mods2(state, KEY_A + EVDEV_OFFSET,
                                            XKB_CONSUMED_MODE_GTK);
    assert(mask == ((1U << shift) | (1U << caps)));

    xkb_state_unref(state);
}

static void
test_overlapping_mods(struct xkb_context *context)
{
    struct xkb_keymap *keymap;
    struct xkb_state *state;

    /* Super and Hyper are overlapping (full overlap) */
    keymap = test_compile_rules(context, "evdev", NULL, "us", NULL,
                                "overlapping_modifiers:super_hyper,"
                                "grp:win_space_toggle");
    assert(keymap);
    xkb_mod_index_t shiftIdx = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_SHIFT);
    xkb_mod_index_t capsIdx  = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_CAPS);
    xkb_mod_index_t ctrlIdx  = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_CTRL);
    xkb_mod_index_t mod1Idx  = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_MOD1);
    xkb_mod_index_t mod3Idx  = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_MOD3);
    xkb_mod_index_t mod4Idx  = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_MOD4);
    xkb_mod_index_t mod5Idx  = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_MOD5);
    xkb_mod_index_t altIdx   = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_ALT);
    xkb_mod_index_t metaIdx  = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_META);
    xkb_mod_index_t superIdx = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_SUPER);
    xkb_mod_index_t hyperIdx = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_HYPER);
    /* Note: not mapped */
    xkb_mod_index_t scrollIdx = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_SCROLL);
    xkb_mod_mask_t shift = (1u << shiftIdx);
    xkb_mod_mask_t ctrl  = (1u << ctrlIdx);
    xkb_mod_mask_t mod1  = (1u << mod1Idx);
    xkb_mod_mask_t mod3  = (1u << mod3Idx);
    xkb_mod_mask_t mod4  = (1u << mod4Idx);
    xkb_mod_mask_t mod5  = (1u << mod5Idx);
    xkb_mod_mask_t alt   = (1u << altIdx);
    xkb_mod_mask_t meta  = (1u << metaIdx);
    xkb_mod_mask_t super = (1u << superIdx);
    xkb_mod_mask_t hyper = (1u << hyperIdx);
    state = xkb_state_new(keymap);
    assert(state);

    const struct test_active_mods_entry test_data1[] = {
        { .state = 0,           .active = 0                           },
        { .state = mod1,        .active = mod1 | alt | meta           },
        { .state = mod3,        .active = mod3                        },
        { .state = mod4,        .active = mod4                        },
        { .state = alt,         .active = mod1 | alt | meta           },
        { .state = meta,        .active = mod1 | alt | meta           },
        { .state = super,       .active = mod3 | mod4 | super | hyper },
        { .state = hyper,       .active = mod3 | mod4 | super | hyper },
        { .state = mod3 | mod4, .active = mod3 | mod4 | super | hyper },
    };

    for (unsigned k = 0; k < ARRAY_SIZE(test_data1); k++) {
        const struct test_active_mods_entry *entry = &test_data1[k];
        xkb_state_update_mask(state, entry->state, 0, 0, 0, 0, 0);
        check_mods(keymap, state, entry, XKB_STATE_MODS_DEPRESSED);
    }
    assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE) == (mod3 | mod4));
    assert(xkb_state_mod_indices_are_active(state, XKB_STATE_MODS_EFFECTIVE,
                                            XKB_STATE_MATCH_ANY,
                                            mod3Idx, mod4Idx, superIdx, hyperIdx,
                                            XKB_MOD_INVALID) > 0);
    assert(xkb_state_mod_indices_are_active(state, XKB_STATE_MODS_EFFECTIVE,
                                            XKB_STATE_MATCH_ALL,
                                            mod3Idx, mod4Idx, superIdx, hyperIdx,
                                            XKB_MOD_INVALID) > 0);
    assert(xkb_state_key_get_consumed_mods2(state, KEY_F1 + EVDEV_OFFSET, XKB_CONSUMED_MODE_XKB) ==
           (shift | ctrl | mod1 | mod5));
    assert(xkb_state_mod_mask_remove_consumed(state, KEY_F1 + EVDEV_OFFSET, (mod1 | mod4 | mod5)) == mod4);
    assert(xkb_state_mod_mask_remove_consumed(state, KEY_F1 + EVDEV_OFFSET, (alt | super)) == (mod3 | mod4));
    assert(xkb_state_mod_index_is_consumed2(state, KEY_F1 + EVDEV_OFFSET, shiftIdx,  XKB_CONSUMED_MODE_XKB) > 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_F1 + EVDEV_OFFSET, capsIdx,   XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_F1 + EVDEV_OFFSET, ctrlIdx,   XKB_CONSUMED_MODE_XKB) > 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_F1 + EVDEV_OFFSET, mod1Idx,   XKB_CONSUMED_MODE_XKB) > 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_F1 + EVDEV_OFFSET, mod5Idx,   XKB_CONSUMED_MODE_XKB) > 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_F1 + EVDEV_OFFSET, altIdx,    XKB_CONSUMED_MODE_XKB) > 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_F1 + EVDEV_OFFSET, metaIdx,   XKB_CONSUMED_MODE_XKB) > 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_F1 + EVDEV_OFFSET, superIdx,  XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_F1 + EVDEV_OFFSET, hyperIdx,  XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_F1 + EVDEV_OFFSET, scrollIdx, XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_key_get_consumed_mods2(state, KEY_SPACE + EVDEV_OFFSET, XKB_CONSUMED_MODE_XKB) == mod4);
    assert(xkb_state_mod_mask_remove_consumed(state, KEY_SPACE + EVDEV_OFFSET, (mod3 | mod4)) == mod3);
    assert(xkb_state_mod_mask_remove_consumed(state, KEY_SPACE + EVDEV_OFFSET, (super | hyper)) == mod3);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, shiftIdx,  XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, capsIdx,   XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, ctrlIdx,   XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, mod1Idx,   XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, mod5Idx,   XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, altIdx,    XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, metaIdx,   XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, superIdx,  XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, hyperIdx,  XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, scrollIdx, XKB_CONSUMED_MODE_XKB) == 0);
    xkb_state_update_mask(state, mod4, 0, 0, 0, 0, 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, shiftIdx,  XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, capsIdx,   XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, ctrlIdx,   XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, mod1Idx,   XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, mod5Idx,   XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, altIdx,    XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, metaIdx,   XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, superIdx,  XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, hyperIdx,  XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, scrollIdx, XKB_CONSUMED_MODE_XKB) == 0);
    xkb_state_unref(state);
    xkb_keymap_unref(keymap);

    /* Super and Hyper are overlapping (full overlap).
     * Alt overlaps with Meta (incomplete overlap) */
    keymap = test_compile_rules(context, "evdev", NULL, "us", NULL,
                                "overlapping_modifiers:meta,"
                                "grp:win_space_toggle");
    assert(keymap);
    altIdx   = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_ALT);
    metaIdx  = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_META);
    superIdx = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_SUPER);
    hyperIdx = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_HYPER);
    alt   = (1u << altIdx);
    meta  = (1u << metaIdx);
    super = (1u << superIdx);
    hyper = (1u << hyperIdx);
    state = xkb_state_new(keymap);
    assert(state);

    const struct test_active_mods_entry test_data2[] = {
        { .state = 0,                  .active = 0                                               },
        { .state = mod1,               .active = mod1 | alt                                      },
        { .state = mod3,               .active = mod3                                            },
        { .state = mod4,               .active = mod4 | hyper | super                            },
        { .state = alt,                .active = mod1 | alt                                      },
        { .state = meta,               .active = mod1 | mod3 | alt | meta                        },
        { .state = super,              .active = mod4 | hyper | super                            },
        { .state = hyper,              .active = mod4 | hyper | super                            },
        { .state = mod1 | mod3,        .active = mod1 | mod3 | alt | meta                        },
        { .state = mod1 | mod4,        .active = mod1 | mod4 | alt | super | hyper               },
        { .state = mod3 | mod4,        .active = mod3 | mod4 | super | hyper                     },
        { .state = mod1 | mod3 | mod4, .active = mod1 | mod3 | mod4 | alt | meta | super | hyper },
    };

    for (unsigned k = 0; k < ARRAY_SIZE(test_data2); k++) {
        const struct test_active_mods_entry *entry = &test_data2[k];
        xkb_state_update_mask(state, entry->state, 0, 0, 0, 0, 0);
        check_mods(keymap, state, entry, XKB_STATE_MODS_DEPRESSED);
    }
    assert(xkb_state_mod_indices_are_active(state, XKB_STATE_MODS_EFFECTIVE,
                                            XKB_STATE_MATCH_ANY,
                                            mod1Idx, mod3Idx, mod4Idx, altIdx,
                                            metaIdx, superIdx, hyperIdx,
                                            XKB_MOD_INVALID) > 0);
    assert(xkb_state_mod_indices_are_active(state, XKB_STATE_MODS_EFFECTIVE,
                                            XKB_STATE_MATCH_ALL,
                                            mod1Idx, mod3Idx, mod4Idx, altIdx,
                                            metaIdx, superIdx, hyperIdx,
                                            XKB_MOD_INVALID) > 0);
    assert(xkb_state_key_get_consumed_mods2(state, KEY_F1 + EVDEV_OFFSET, XKB_CONSUMED_MODE_XKB) ==
           (shift | ctrl | mod1 | mod5));
    assert(xkb_state_mod_mask_remove_consumed(state, KEY_F1 + EVDEV_OFFSET, (mod1 | mod4 | mod5)) == mod4);
    assert(xkb_state_mod_mask_remove_consumed(state, KEY_F1 + EVDEV_OFFSET, (alt | super)) == mod4);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_F1 + EVDEV_OFFSET, shiftIdx, XKB_CONSUMED_MODE_XKB) > 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_F1 + EVDEV_OFFSET, capsIdx,  XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_F1 + EVDEV_OFFSET, ctrlIdx,  XKB_CONSUMED_MODE_XKB) > 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_F1 + EVDEV_OFFSET, mod1Idx,  XKB_CONSUMED_MODE_XKB) > 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_F1 + EVDEV_OFFSET, mod5Idx,  XKB_CONSUMED_MODE_XKB) > 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_F1 + EVDEV_OFFSET, altIdx,   XKB_CONSUMED_MODE_XKB) > 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_F1 + EVDEV_OFFSET, metaIdx,  XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_F1 + EVDEV_OFFSET, superIdx, XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_F1 + EVDEV_OFFSET, hyperIdx, XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_key_get_consumed_mods2(state, KEY_SPACE + EVDEV_OFFSET, XKB_CONSUMED_MODE_XKB) ==
           mod4);
    assert(xkb_state_mod_mask_remove_consumed(state, KEY_SPACE + EVDEV_OFFSET, (mod3 | mod4)) == mod3);
    assert(xkb_state_mod_mask_remove_consumed(state, KEY_SPACE + EVDEV_OFFSET, (super | hyper)) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, shiftIdx, XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, capsIdx,  XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, ctrlIdx,  XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, mod1Idx,  XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, mod5Idx,  XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, altIdx,   XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, metaIdx,  XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, superIdx, XKB_CONSUMED_MODE_XKB) > 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_SPACE + EVDEV_OFFSET, hyperIdx, XKB_CONSUMED_MODE_XKB) > 0);
    xkb_state_update_mask(state, mod1, 0, 0, 0, 0, 0);
    assert(xkb_state_mod_indices_are_active(state, XKB_STATE_MODS_EFFECTIVE,
                                            XKB_STATE_MATCH_ANY,
                                            mod1Idx, altIdx,
                                            XKB_MOD_INVALID) > 0);
    assert(xkb_state_mod_indices_are_active(state, XKB_STATE_MODS_EFFECTIVE,
                                            XKB_STATE_MATCH_ALL,
                                            mod1Idx, altIdx,
                                            XKB_MOD_INVALID) > 0);
    xkb_state_update_mask(state, mod1 | mod3, 0, 0, 0, 0, 0);
    assert(xkb_state_mod_indices_are_active(state, XKB_STATE_MODS_EFFECTIVE,
                                            XKB_STATE_MATCH_ANY,
                                            mod1Idx, mod3Idx, altIdx, metaIdx,
                                            XKB_MOD_INVALID) > 0);
    assert(xkb_state_mod_indices_are_active(state, XKB_STATE_MODS_EFFECTIVE,
                                            XKB_STATE_MATCH_ALL,
                                            mod1Idx, mod3Idx, altIdx, metaIdx,
                                            XKB_MOD_INVALID) > 0);
    xkb_state_unref(state);
    xkb_keymap_unref(keymap);

    /* Super and Hyper overlaps with Meta; Alt overlaps with Meta */
    keymap = test_compile_rules(context, "evdev", NULL, "us", NULL,
                                "overlapping_modifiers:super_hyper,"
                                "overlapping_modifiers:meta");
    assert(keymap);
    altIdx   = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_ALT);
    metaIdx  = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_META);
    superIdx = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_SUPER);
    hyperIdx = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_HYPER);
    alt   = (1u << altIdx);
    meta  = (1u << metaIdx);
    super = (1u << superIdx);
    hyper = (1u << hyperIdx);
    state = xkb_state_new(keymap);
    assert(state);

    const struct test_active_mods_entry test_data3[] = {
        { .state = 0,                  .active = 0                                               },
        { .state = mod1,               .active = mod1 | alt                                      },
        { .state = mod3,               .active = mod3                                            },
        { .state = mod4,               .active = mod4                                            },
        { .state = alt,                .active = mod1 | alt                                      },
        { .state = meta,               .active = mod1 | mod3 | alt | meta                        },
        { .state = super,              .active = mod3 | mod4 | super | hyper                     },
        { .state = hyper,              .active = mod3 | mod4 | super | hyper                     },
        { .state = mod1 | mod3,        .active = mod1 | mod3 | alt | meta                        },
        { .state = mod1 | mod3,        .active = mod1 | mod3 | alt | meta                        },
        { .state = mod1 | mod4,        .active = mod1 | mod4 | alt                               },
        { .state = mod3 | mod4,        .active = mod3 | mod4 | super | hyper                     },
        { .state = mod1 | mod3 | mod4, .active = mod1 | mod3 | mod4 | alt | meta | super | hyper },
    };

    for (unsigned k = 0; k < ARRAY_SIZE(test_data3); k++) {
        const struct test_active_mods_entry *entry = &test_data3[k];
        xkb_state_update_mask(state, entry->state, 0, 0, 0, 0, 0);
        check_mods(keymap, state, entry, XKB_STATE_MODS_DEPRESSED);
    }
    assert(xkb_state_mod_indices_are_active(state, XKB_STATE_MODS_EFFECTIVE,
                                            XKB_STATE_MATCH_ANY,
                                            mod1Idx, mod3Idx, mod4Idx, altIdx,
                                            metaIdx, superIdx, hyperIdx,
                                            XKB_MOD_INVALID) > 0);
    assert(xkb_state_mod_indices_are_active(state, XKB_STATE_MODS_EFFECTIVE,
                                            XKB_STATE_MATCH_ALL,
                                            mod1Idx, mod3Idx, mod4Idx, altIdx,
                                            metaIdx, superIdx, hyperIdx,
                                            XKB_MOD_INVALID) > 0);
    assert(xkb_state_key_get_consumed_mods2(state, KEY_F1 + EVDEV_OFFSET, XKB_CONSUMED_MODE_XKB) ==
           (shift | ctrl | mod1 | mod5));
    assert(xkb_state_mod_mask_remove_consumed(state, KEY_F1 + EVDEV_OFFSET, (mod1 | mod4 | mod5)) == mod4);
    assert(xkb_state_mod_mask_remove_consumed(state, KEY_F1 + EVDEV_OFFSET, (alt | super)) == (mod3 | mod4));
    assert(xkb_state_mod_index_is_consumed2(state, KEY_F1 + EVDEV_OFFSET, shiftIdx, XKB_CONSUMED_MODE_XKB) > 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_F1 + EVDEV_OFFSET, capsIdx,  XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_F1 + EVDEV_OFFSET, ctrlIdx,  XKB_CONSUMED_MODE_XKB) > 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_F1 + EVDEV_OFFSET, mod1Idx,  XKB_CONSUMED_MODE_XKB) > 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_F1 + EVDEV_OFFSET, mod5Idx,  XKB_CONSUMED_MODE_XKB) > 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_F1 + EVDEV_OFFSET, altIdx,   XKB_CONSUMED_MODE_XKB) > 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_F1 + EVDEV_OFFSET, metaIdx,  XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_F1 + EVDEV_OFFSET, superIdx, XKB_CONSUMED_MODE_XKB) == 0);
    assert(xkb_state_mod_index_is_consumed2(state, KEY_F1 + EVDEV_OFFSET, hyperIdx, XKB_CONSUMED_MODE_XKB) == 0);
    xkb_state_update_mask(state, mod1 | mod3, 0, 0, 0, 0, 0);
    assert(xkb_state_mod_indices_are_active(state, XKB_STATE_MODS_EFFECTIVE,
                                            XKB_STATE_MATCH_ANY,
                                            mod1Idx, mod3Idx, altIdx, metaIdx,
                                            XKB_MOD_INVALID) > 0);
    assert(xkb_state_mod_indices_are_active(state, XKB_STATE_MODS_EFFECTIVE,
                                            XKB_STATE_MATCH_ALL,
                                            mod1Idx, mod3Idx, altIdx, metaIdx,
                                            XKB_MOD_INVALID) > 0);
    xkb_state_update_mask(state, mod1 | mod4, 0, 0, 0, 0, 0);
    assert(xkb_state_mod_indices_are_active(state, XKB_STATE_MODS_EFFECTIVE,
                                            XKB_STATE_MATCH_ANY,
                                            mod1Idx, mod4Idx, altIdx,
                                            XKB_MOD_INVALID) > 0);
    assert(xkb_state_mod_indices_are_active(state, XKB_STATE_MODS_EFFECTIVE,
                                            XKB_STATE_MATCH_ALL,
                                            mod1Idx, mod4Idx, altIdx,
                                            XKB_MOD_INVALID) > 0);
    xkb_state_update_mask(state, mod3 | mod4, 0, 0, 0, 0, 0);
    assert(xkb_state_mod_indices_are_active(state, XKB_STATE_MODS_EFFECTIVE,
                                            XKB_STATE_MATCH_ANY,
                                            mod3Idx, mod4Idx, superIdx, hyperIdx,
                                            XKB_MOD_INVALID) > 0);
    assert(xkb_state_mod_indices_are_active(state, XKB_STATE_MODS_EFFECTIVE,
                                            XKB_STATE_MATCH_ALL,
                                            mod3Idx, mod4Idx, superIdx, hyperIdx,
                                            XKB_MOD_INVALID) > 0);
    xkb_state_unref(state);
    xkb_keymap_unref(keymap);
}

static void
key_iter(struct xkb_keymap *keymap, xkb_keycode_t key, void *data)
{
    xkb_keycode_t *counter = data;

    assert(*counter == key);
    (*counter)++;
}

static void
test_range(struct xkb_keymap *keymap)
{
    xkb_keycode_t counter;

    assert(xkb_keymap_min_keycode(keymap) == 9);
    assert(xkb_keymap_max_keycode(keymap) == 569);

    counter = xkb_keymap_min_keycode(keymap);
    xkb_keymap_key_for_each(keymap, key_iter, &counter);
    assert(counter == xkb_keymap_max_keycode(keymap) + 1);
}

static void
test_caps_keysym_transformation(struct xkb_keymap *keymap)
{
    int nsyms;
    xkb_keysym_t sym;
    const xkb_keysym_t *syms;
    xkb_mod_index_t shift = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_SHIFT);
    xkb_mod_index_t caps = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_CAPS);
    struct xkb_state *state = xkb_state_new(keymap);
    assert(state);

    /* See xkb_state_key_get_one_sym() for what's this all about. */

    assert(xkb_state_key_get_layout(state, KEY_A + EVDEV_OFFSET) == 0);
    assert(xkb_state_key_get_layout(state, KEY_SEMICOLON + EVDEV_OFFSET) == 0);

    /* Without caps, no transformation. */
    assert(xkb_state_mod_index_is_active(state, caps, XKB_STATE_MODS_EFFECTIVE) == 0);
    assert(xkb_state_mod_index_is_active(state, shift, XKB_STATE_MODS_EFFECTIVE) == 0);
    assert(xkb_state_key_get_level(state, KEY_A + EVDEV_OFFSET, 0) == 0);
    sym = xkb_state_key_get_one_sym(state, KEY_A + EVDEV_OFFSET);
    assert(sym == XKB_KEY_a);
    assert(xkb_state_key_get_level(state, KEY_SEMICOLON + EVDEV_OFFSET, 0) == 0);
    sym = xkb_state_key_get_one_sym(state, KEY_SEMICOLON + EVDEV_OFFSET);
    assert(sym == XKB_KEY_eacute);
    nsyms = xkb_state_key_get_syms(state, KEY_SEMICOLON + EVDEV_OFFSET, &syms);
    assert(nsyms == 1 && syms[0] == XKB_KEY_eacute);

    /* With shift, no transformation (only different level). */
    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
    assert(xkb_state_mod_index_is_active(state, caps, XKB_STATE_MODS_EFFECTIVE) == 0);
    assert(xkb_state_mod_index_is_active(state, shift, XKB_STATE_MODS_EFFECTIVE) > 0);
    assert(xkb_state_key_get_level(state, KEY_A + EVDEV_OFFSET, 0) == 1);
    sym = xkb_state_key_get_one_sym(state, KEY_A + EVDEV_OFFSET);
    assert(sym == XKB_KEY_A);
    sym = xkb_state_key_get_one_sym(state, KEY_SEMICOLON + EVDEV_OFFSET);
    assert(sym == XKB_KEY_odiaeresis);
    nsyms = xkb_state_key_get_syms(state, KEY_SEMICOLON + EVDEV_OFFSET, &syms);
    assert(nsyms == 1 && syms[0] == XKB_KEY_odiaeresis);
    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);
    assert(xkb_state_mod_index_is_active(state, shift, XKB_STATE_MODS_EFFECTIVE) == 0);

    /* With caps, transform in same level, only with _get_one_sym(). */
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_UP);
    assert(xkb_state_mod_index_is_active(state, caps, XKB_STATE_MODS_EFFECTIVE) > 0);
    assert(xkb_state_mod_index_is_active(state, shift, XKB_STATE_MODS_EFFECTIVE) == 0);
    assert(xkb_state_key_get_level(state, KEY_A + EVDEV_OFFSET, 0) == 1);
    sym = xkb_state_key_get_one_sym(state, KEY_A + EVDEV_OFFSET);
    assert(sym == XKB_KEY_A);
    assert(xkb_state_key_get_level(state, KEY_SEMICOLON + EVDEV_OFFSET, 0) == 0);
    sym = xkb_state_key_get_one_sym(state, KEY_SEMICOLON + EVDEV_OFFSET);
    assert(sym == XKB_KEY_Eacute);
    nsyms = xkb_state_key_get_syms(state, KEY_SEMICOLON + EVDEV_OFFSET, &syms);
    assert(nsyms == 1 && syms[0] == XKB_KEY_eacute);
    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);
    assert(xkb_state_mod_index_is_active(state, shift, XKB_STATE_MODS_EFFECTIVE) == 0);
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_UP);

    xkb_state_unref(state);
}

static void
test_get_utf8_utf32(struct xkb_keymap *keymap)
{
    char buf[256];
    struct xkb_state *state = xkb_state_new(keymap);
    assert(state);

#define TEST_KEY(key, expected_utf8, expected_utf32) do { \
    assert(xkb_state_key_get_utf8(state, key + EVDEV_OFFSET, NULL, 0) == strlen(expected_utf8)); \
    assert(xkb_state_key_get_utf8(state, key + EVDEV_OFFSET, buf, sizeof(buf)) == strlen(expected_utf8)); \
    assert(memcmp(buf, expected_utf8, sizeof(expected_utf8)) == 0); \
    assert(xkb_state_key_get_utf32(state, key + EVDEV_OFFSET) == expected_utf32); \
} while (0)

    /* Simple ASCII. */
    TEST_KEY(KEY_A, "a", 0x61);
    TEST_KEY(KEY_ESC, "\x1B", 0x1B);
    TEST_KEY(KEY_1, "1", 0x31);

    /* Invalid. */
    TEST_KEY(XKB_KEYCODE_INVALID - 8, "", 0);
    TEST_KEY(300, "", 0);

    /* No string. */
    TEST_KEY(KEY_LEFTCTRL, "", 0);
    TEST_KEY(KEY_NUMLOCK, "", 0);

    /* Multiple keysyms. */
    TEST_KEY(KEY_6, "HELLO", 0);
    TEST_KEY(KEY_7, "YES THIS IS DOG", 0);

    /* Check truncation. */
    memset(buf, 'X', sizeof(buf));
    assert(xkb_state_key_get_utf8(state, KEY_6 + EVDEV_OFFSET, buf, 0) == strlen("HELLO"));
    assert(memcmp(buf, "X", 1) == 0);
    assert(xkb_state_key_get_utf8(state, KEY_6 + EVDEV_OFFSET, buf, 1) == strlen("HELLO"));
    assert(memcmp(buf, "", 1) == 0);
    assert(xkb_state_key_get_utf8(state, KEY_6 + EVDEV_OFFSET, buf, 2) == strlen("HELLO"));
    assert(memcmp(buf, "H", 2) == 0);
    assert(xkb_state_key_get_utf8(state, KEY_6 + EVDEV_OFFSET, buf, 3) == strlen("HELLO"));
    assert(memcmp(buf, "HE", 3) == 0);
    assert(xkb_state_key_get_utf8(state, KEY_6 + EVDEV_OFFSET, buf, 5) == strlen("HELLO"));
    assert(memcmp(buf, "HELL", 5) == 0);
    assert(xkb_state_key_get_utf8(state, KEY_6 + EVDEV_OFFSET, buf, 6) == strlen("HELLO"));
    assert(memcmp(buf, "HELLO", 6) == 0);
    assert(xkb_state_key_get_utf8(state, KEY_6 + EVDEV_OFFSET, buf, 7) == strlen("HELLO"));
    assert(memcmp(buf, "HELLO\0X", 7) == 0);

    /* Switch to ru layout */
    xkb_state_update_key(state, KEY_COMPOSE + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_COMPOSE + EVDEV_OFFSET, XKB_KEY_UP);
    assert(xkb_state_key_get_layout(state, KEY_A + EVDEV_OFFSET) == 1);

    /* Non ASCII. */
    TEST_KEY(KEY_ESC, "\x1B", 0x1B);
    TEST_KEY(KEY_A, "ф", 0x0444);
    TEST_KEY(KEY_Z, "я", 0x044F);

    /* Switch back to us layout */
    xkb_state_update_key(state, KEY_COMPOSE + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_COMPOSE + EVDEV_OFFSET, XKB_KEY_UP);
    assert(xkb_state_key_get_layout(state, KEY_A + EVDEV_OFFSET) == 0);

    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
    TEST_KEY(KEY_A, "A", 0x41);
    TEST_KEY(KEY_ESC, "\x1B", 0x1B);
    TEST_KEY(KEY_1, "!", 0x21);
    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);

    TEST_KEY(KEY_6, "HELLO", 0);
    TEST_KEY(KEY_7, "YES THIS IS DOG", 0);

    xkb_state_unref(state);
}

static void
test_ctrl_string_transformation(struct xkb_keymap *keymap)
{
    char buf[256];
    xkb_mod_index_t ctrl = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_CTRL);
    struct xkb_state *state = xkb_state_new(keymap);
    assert(state);

    /* See xkb_state_key_get_utf8() for what's this all about. */


    /* First without. */
    TEST_KEY(KEY_A, "a", 0x61);
    TEST_KEY(KEY_B, "b", 0x62);
    TEST_KEY(KEY_C, "c", 0x63);
    TEST_KEY(KEY_ESC, "\x1B", 0x1B);
    TEST_KEY(KEY_1, "1", 0x31);

    /* And with. */
    xkb_state_update_key(state, KEY_RIGHTCTRL + EVDEV_OFFSET, XKB_KEY_DOWN);
    assert(xkb_state_mod_index_is_active(state, ctrl, XKB_STATE_MODS_EFFECTIVE) > 0);
    TEST_KEY(KEY_A, "\x01", 0x01);
    TEST_KEY(KEY_B, "\x02", 0x02);
    TEST_KEY(KEY_C, "\x03", 0x03);
    TEST_KEY(KEY_ESC, "\x1B", 0x1B);
    TEST_KEY(KEY_1, "1", 0x31);
    xkb_state_update_key(state, KEY_RIGHTCTRL + EVDEV_OFFSET, XKB_KEY_UP);

    /* Switch to ru layout */
    xkb_state_update_key(state, KEY_COMPOSE + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_COMPOSE + EVDEV_OFFSET, XKB_KEY_UP);
    assert(xkb_state_key_get_layout(state, KEY_A + EVDEV_OFFSET) == 1);

    /* Non ASCII. */
    xkb_state_update_key(state, KEY_RIGHTCTRL + EVDEV_OFFSET, XKB_KEY_DOWN);
    assert(xkb_state_mod_index_is_active(state, ctrl, XKB_STATE_MODS_EFFECTIVE) > 0);
    TEST_KEY(KEY_A, "\x01", 0x01);
    TEST_KEY(KEY_B, "\x02", 0x02);
    xkb_state_update_key(state, KEY_RIGHTCTRL + EVDEV_OFFSET, XKB_KEY_UP);

    xkb_state_unref(state);
}

int
main(void)
{
    test_init();

    struct xkb_context *context = test_get_context(CONTEXT_NO_FLAG);
    struct xkb_keymap *keymap;

    assert(context);

    /* Make sure these are allowed. */
    xkb_context_unref(NULL);
    xkb_keymap_unref(NULL);
    xkb_state_unref(NULL);

    keymap = test_compile_rules(context, "evdev", "pc104", "us,ru", NULL,
                                "grp:menu_toggle,grp:lwin_latch,"
                                "grp:rwin_latch_lock_clear,lv3:lsgt_latch");
    assert(keymap);

    test_update_key(keymap);
    test_update_latched_locked(keymap);
    test_serialisation(keymap);
    test_update_mask_mods(keymap);
    test_repeat(keymap);
    test_consume(keymap);
    test_range(keymap);
    test_get_utf8_utf32(keymap);
    test_ctrl_string_transformation(keymap);
    test_overlapping_mods(context);

    xkb_keymap_unref(keymap);
    keymap = test_compile_rules(context, "evdev", NULL, "ch", "fr", NULL);
    assert(keymap);

    test_caps_keysym_transformation(keymap);

    xkb_keymap_unref(keymap);
    xkb_context_unref(context);
}
