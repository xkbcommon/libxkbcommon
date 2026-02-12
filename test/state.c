/*
 * Copyright © 2012 Intel Corporation
 * SPDX-License-Identifier: MIT
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 */

#include "config.h"
#include "test-config.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "xkbcommon/xkbcommon-keysyms.h"
#include "xkbcommon/xkbcommon-names.h"
#include "xkbcommon/xkbcommon.h"

#include "context.h"
#include "evdev-scancodes.h"
#include "src/keysym.h"
#include "src/keymap.h"
#include "src/state-priv.h"
#include "test.h"
#include "utils.h"

static const enum xkb_keymap_format keymap_formats[] = {
    XKB_KEYMAP_FORMAT_TEXT_V1,
    XKB_KEYMAP_FORMAT_TEXT_V2
};

#define XKB_EVENT_TYPE_NONE 0

/* Offset between evdev keycodes (where KEY_ESCAPE is 1), and the evdev XKB
 * keycode set (where ESC is 9). */
#define EVDEV_OFFSET 8

/* S sharp
 * • U+00DF ß: lower case
 * •       SS: upper case (special mapping, not handled by us)
 * • U+1E9E ẞ: upper case, only for capitals
 */
#ifndef XKB_KEY_Ssharp
#define XKB_KEY_Ssharp (XKB_KEYSYM_UNICODE_OFFSET + 0x1E9E)
#endif

static void
test_state_machine_options(struct xkb_context *ctx)
{
    struct xkb_state_machine_options *options =
        xkb_state_machine_options_new(ctx);
    assert(options);

    /* Invalid flags */
    assert(xkb_state_machine_options_update_a11y_flags(options, -1000, 0) == 1);
    assert(xkb_state_machine_options_update_a11y_flags(options, 1000, 0) == 1);

    /* Valid flags */
    static_assert(XKB_STATE_A11Y_NO_FLAGS == 0, "default flags");
    assert(xkb_state_machine_options_update_a11y_flags(
            options, XKB_STATE_A11Y_NO_FLAGS, 1000) == 0);

    struct xkb_keymap *keymap =
        xkb_keymap_new_from_names(ctx, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);
    assert(keymap);

    struct xkb_state_machine *sm = xkb_state_machine_new(keymap, options);
    assert(sm);

    xkb_state_machine_unref(sm);
    xkb_keymap_unref(keymap);
    xkb_state_machine_options_destroy(options);
}

/* Reference implementation from XkbAdjustGroup in Xorg xserver */
static int32_t
group_wrap_ref(int32_t g, int32_t num_groups)
{
    assert(num_groups >= 0);
    if (num_groups == 0) {
        return 0;
    } else if (g < 0) {
        while (g < 0)
            g += num_groups;
    } else if (g >= num_groups) {
        g %= num_groups;
    }
    return g;
}

/* Function extracted from XkbWrapGroupIntoRange (current) */
static int32_t
group_wrap(int32_t g, int32_t num_groups)
{
    assert(num_groups >= 0);
    if (num_groups == 0)
        return 0;
    if (g >= 0 && g < num_groups)
        return g;
    const int32_t remainder = g % num_groups;
    return (remainder < 0) ? num_groups + remainder : remainder;
}

/* Old bogus implementation */
static int32_t
group_wrap_old(int32_t g, int32_t num_groups)
{
    assert(num_groups >= 0);
    if (num_groups == 0)
        return 0;
    if (g >= 0 && g < num_groups)
        return g;
    /* Invalid modulus arithmetic (see comment in XkbWrapGroupIntoRange) */
    const int32_t remainder = g % num_groups;
    return (g < 0) ? num_groups + remainder : remainder;
}

static bool
is_valid_group(int32_t g, int32_t num_groups)
{
    assert(num_groups >= 0);
    return (num_groups > 0 && g >= 0 && g < num_groups);
}

static void
test_group_wrap(struct xkb_context *ctx)
{
    /* Compare wrap function with reference implementation */
    for (int32_t G = 0; G <= XKB_MAX_GROUPS; G++) {
        for (int32_t g = - 3 * (G + 1); g <= 3 * (G + 1); g++) {
            /* Same as xserver */
            assert(group_wrap(g, G) == group_wrap_ref(g, G));
            /* Old implementation */
            const int32_t old = group_wrap_old(g, G);
            const int32_t new = group_wrap(g, G);
            assert((old == new) ^ (G > 0 && g < 0 && ((-g) % G == 0)));
        }
    }

    /* Check some special cases */
    assert(group_wrap(-2, 0) == 0);
    assert(group_wrap(-1, 0) == 0);
    assert(group_wrap(0, 0) == 0);
    assert(group_wrap(1, 0) == 0);
    assert(group_wrap(2, 0) == 0);

    assert(group_wrap(-2, 1) == 0);
    assert(group_wrap(-1, 1) == 0);
    assert(group_wrap(0, 1) == 0);
    assert(group_wrap(1, 1) == 0);
    assert(group_wrap(2, 1) == 0);

    assert(group_wrap(-6, 2) == 0);
    assert(group_wrap(-5, 2) == 1);
    assert(group_wrap(-4, 2) == 0);
    assert(group_wrap(-3, 2) == 1);
    assert(group_wrap(-2, 2) == 0);
    assert(group_wrap(-1, 2) == 1);
    assert(group_wrap(0, 2) == 0);
    assert(group_wrap(1, 2) == 1);
    assert(group_wrap(2, 2) == 0);
    assert(group_wrap(3, 2) == 1);
    assert(group_wrap(4, 2) == 0);
    assert(group_wrap(5, 2) == 1);
    assert(group_wrap(6, 2) == 0);

    assert(group_wrap(-7, 3) == 2);
    assert(group_wrap(-6, 3) == 0);
    assert(group_wrap(-5, 3) == 1);
    assert(group_wrap(-4, 3) == 2);
    assert(group_wrap(-3, 3) == 0);
    assert(group_wrap(-2, 3) == 1);
    assert(group_wrap(-1, 3) == 2);
    assert(group_wrap(0, 3) == 0);
    assert(group_wrap(1, 3) == 1);
    assert(group_wrap(2, 3) == 2);
    assert(group_wrap(3, 3) == 0);
    assert(group_wrap(4, 3) == 1);
    assert(group_wrap(5, 3) == 2);
    assert(group_wrap(6, 3) == 0);
    assert(group_wrap(7, 3) == 1);

    assert(group_wrap(-9, 4) == 3);
    assert(group_wrap(-8, 4) == 0);
    assert(group_wrap(-7, 4) == 1);
    assert(group_wrap(-6, 4) == 2);
    assert(group_wrap(-5, 4) == 3);
    assert(group_wrap(-4, 4) == 0);
    assert(group_wrap(-3, 4) == 1);
    assert(group_wrap(-2, 4) == 2);
    assert(group_wrap(-1, 4) == 3);
    assert(group_wrap(0, 4) == 0);
    assert(group_wrap(1, 4) == 1);
    assert(group_wrap(2, 4) == 2);
    assert(group_wrap(3, 4) == 3);
    assert(group_wrap(4, 4) == 0);
    assert(group_wrap(5, 4) == 1);
    assert(group_wrap(6, 4) == 2);
    assert(group_wrap(7, 4) == 3);
    assert(group_wrap(8, 4) == 0);
    assert(group_wrap(9, 4) == 1);

    assert(group_wrap(-11, 5) == 4);
    assert(group_wrap(-10, 5) == 0);
    assert(group_wrap(-9, 5) == 1);
    assert(group_wrap(-8, 5) == 2);
    assert(group_wrap(-7, 5) == 3);
    assert(group_wrap(-6, 5) == 4);
    assert(group_wrap(-5, 5) == 0);
    assert(group_wrap(-4, 5) == 1);
    assert(group_wrap(-3, 5) == 2);
    assert(group_wrap(-2, 5) == 3);
    assert(group_wrap(-1, 5) == 4);
    assert(group_wrap(0, 5) == 0);
    assert(group_wrap(1, 5) == 1);
    assert(group_wrap(2, 5) == 2);
    assert(group_wrap(3, 5) == 3);
    assert(group_wrap(4, 5) == 4);
    assert(group_wrap(5, 5) == 0);
    assert(group_wrap(6, 5) == 1);
    assert(group_wrap(7, 5) == 2);
    assert(group_wrap(8, 5) == 3);
    assert(group_wrap(9, 5) == 4);
    assert(group_wrap(10, 5) == 0);
    assert(group_wrap(11, 5) == 1);

    /* Check state group computation */
    const struct {
        const char* keymap;
        xkb_layout_index_t layout_count;
    } keymaps[] = {
        {
            .keymap =
                "default xkb_keymap {\n"
                "    xkb_keycodes { <> = 1; };\n"
                "    xkb_types { type \"ONE_LEVEL\" { map[none] = 1; }; };\n"
                "};",
            .layout_count = 0
        },
        {
            .keymap =
                "default xkb_keymap {\n"
                "    xkb_keycodes { <> = 1; };\n"
                "    xkb_types { type \"ONE_LEVEL\" { map[none] = 1; }; };\n"
                "    xkb_symbols {\n"
                "        key <> { [a] };\n"
                "    };\n"
                "};",
            .layout_count = 1
        },
        {
            .keymap =
                "default xkb_keymap {\n"
                "    xkb_keycodes { <> = 1; };\n"
                "    xkb_types { type \"ONE_LEVEL\" { map[none] = 1; }; };\n"
                "    xkb_symbols {\n"
                "        key <> { [a], [b] };\n"
                "    };\n"
                "};",
            .layout_count = 2
        },
        {
            .keymap =
                /* 3 groups */
                "default xkb_keymap {\n"
                "    xkb_keycodes { <> = 1; };\n"
                "    xkb_types { type \"ONE_LEVEL\" { map[none] = 1; }; };\n"
                "    xkb_symbols {\n"
                "        key <> { [a], [b], [c] };\n"
                "    };\n"
                "};",
            .layout_count = 3
        },
        {
            .keymap =
                /* 4 groups */
                "default xkb_keymap {\n"
                "    xkb_keycodes { <> = 1; };\n"
                "    xkb_types { type \"ONE_LEVEL\" { map[none] = 1; }; };\n"
                "    xkb_symbols {\n"
                "        key <> { [a], [b], [c], [d] };\n"
                "    };\n"
                "};",
            .layout_count = 4
        },
        {
            .keymap =
                "default xkb_keymap {\n"
                "    xkb_keycodes { <> = 1; };\n"
                "    xkb_types { type \"ONE_LEVEL\" { map[none] = 1; }; };\n"
                "    xkb_symbols {\n"
                "        key <> { [a], [b], [c], [d], [e] };\n"
                "    };\n"
                "};",
            .layout_count = 5
        },
        {
            .keymap =
                "default xkb_keymap {\n"
                "    xkb_keycodes { <> = 1; };\n"
                "    xkb_types { type \"ONE_LEVEL\" { map[none] = 1; }; };\n"
                "    xkb_symbols {\n"
                "      key <> {\n"
                "        [a], [b], [c], [d], [e], [f], [g], [h], [i], [j],\n"
                "        [k], [l], [m], [n], [o], [p], [q], [r], [s], [t],\n"
                "        [u], [v], [w], [x], [y], [z], [1], [2], [3], [4],\n"
                "        [5], [6]\n"
                " };\n"
                "    };\n"
                "};",
            .layout_count = XKB_MAX_GROUPS
        }
    };

    static_assert(ARRAY_SIZE(keymaps) > XKB_MAX_GROUPS_X11, "Not enough maps");

    for (int32_t k = 0; k < (int32_t)ARRAY_SIZE(keymaps); k++) {
    for (unsigned int f = (keymaps[k].layout_count < XKB_MAX_GROUPS_X11) ? 0 : 1;
         f < ARRAY_SIZE(keymap_formats); f++) {
        const enum xkb_keymap_format format = keymap_formats[f];
        const int32_t g = (int32_t) keymaps[k].layout_count;
        fprintf(stderr, "------\n*** %s: #%"PRId32" groups, format %d ***\n",
                __func__, g, format);
        struct xkb_keymap *keymap =
            test_compile_buffer(ctx, format,
                                keymaps[k].keymap, strlen(keymaps[k].keymap));
        assert(keymap);
        assert(xkb_keymap_num_layouts(keymap) == keymaps[k].layout_count);
        struct xkb_state *state = xkb_state_new(keymap);
        assert(state);

        const xkb_keycode_t keycode = xkb_keymap_key_by_name(keymap, "");
        assert(keycode == 1);

        for (int32_t base = -2*(g + 1); base <= 2*(g + 1); base++) {
            for (int32_t latched = -2*(g + 1); latched <= 2*(g + 1); latched++) {
                for (int32_t locked = -2*(g + 1); locked <= 2*(g + 2); locked++) {
                    xkb_state_update_mask(state, 0, 0, 0, base, latched, locked);

                    xkb_layout_index_t got;
                    xkb_layout_index_t expected;

                    /* Base layout should be unchanged */
                    got = xkb_state_serialize_layout(state,
                                                     XKB_STATE_LAYOUT_DEPRESSED);
                    expected = (xkb_layout_index_t) base;
                    assert_printf(got == expected,
                                  "Base layout: expected %"PRIu32", "
                                  "got: %"PRIu32"\n",
                                  expected, got);

                    /* Latched layout should be unchanged */
                    got = xkb_state_serialize_layout(state,
                                                     XKB_STATE_LAYOUT_LATCHED);
                    expected = (xkb_layout_index_t) latched;
                    assert_printf(got == expected,
                                  "Latched layout: expected %"PRIu32", "
                                  "got: %"PRIu32"\n",
                                  expected, got);

                    /* Locked layout should be wrapped */
                    got = xkb_state_serialize_layout(state,
                                                     XKB_STATE_LAYOUT_LOCKED);
                    const xkb_layout_index_t locked_expected =
                        group_wrap(locked, g);
                    expected = locked_expected;
                    assert_printf(got == expected,
                                  "Locked layout: expected %"PRIu32", "
                                  "got: %"PRIu32"\n",
                                  expected, got);

                    /* Effective layout should be wrapped */
                    got = xkb_state_serialize_layout(state,
                                                     XKB_STATE_LAYOUT_EFFECTIVE);
                    const xkb_layout_index_t effective_expected =
                        group_wrap(base + latched + (int32_t) locked_expected, g);
                    expected = effective_expected;
                    assert_printf(got == expected,
                                  "Effective layout: expected %"PRIu32", "
                                  "got: %"PRIu32"\n",
                                  expected, got);

                    /*
                     * Ensure all API using a layout index do not segfault
                     */

                    xkb_keymap_layout_get_name(keymap, base);

                    const xkb_level_index_t num_levels =
                        xkb_keymap_num_levels_for_key(keymap, keycode, base);

                    const xkb_level_index_t num_levels_expected = (g > 0);
                    assert_printf(num_levels == num_levels_expected,
                                  "Group=%"PRId32"/%"PRId32": "
                                  "Expected %"PRIu32", got: %"PRIu32"\n",
                                  base + 1, g, num_levels_expected, num_levels);

                    xkb_mod_mask_t masks[1] = {0};
                    const size_t size =
                        xkb_keymap_key_get_mods_for_level(keymap, keycode, base, 0,
                                                          masks, ARRAY_SIZE(masks));
                    const size_t size_expected = (g > 0);
                    assert(size == size_expected && masks[0] == 0);

                    const xkb_keysym_t *keysyms = NULL;
                    const int num_keysyms =
                        xkb_keymap_key_get_syms_by_level(keymap, keycode, base,
                                                         0, &keysyms);
                    const int num_keysyms_expected = (g > 0);
                    assert(num_keysyms == num_keysyms_expected &&
                           (g == 0 || keysyms[0] != XKB_KEY_NoSymbol));

                    const xkb_level_index_t level =
                        xkb_state_key_get_level(state, keycode, base);
                    const xkb_level_index_t level_expected =
                        is_valid_group(base, g) ? 0 : XKB_LEVEL_INVALID;
                    assert_printf(level == level_expected,
                                  "Group=%"PRId32"/%"PRId32": "
                                  "Expected %"PRIu32", got: %"PRIu32"\n",
                                  base + 1, g, level_expected, level);

                    int is_active, is_active_expected;
                    is_active = xkb_state_layout_index_is_active(
                        state, base, XKB_STATE_LAYOUT_DEPRESSED
                    );
                    is_active_expected = is_valid_group(base, g) ? 1 : -1;
                    assert(is_active == is_active_expected);

                    is_active = xkb_state_layout_index_is_active(
                        state, latched, XKB_STATE_LAYOUT_LATCHED
                    );
                    is_active_expected = is_valid_group(latched, g) ? 1 : -1;
                    assert(is_active == is_active_expected);

                    is_active = xkb_state_layout_index_is_active(
                        state, locked, XKB_STATE_LAYOUT_LOCKED
                    );
                    is_active_expected =
                        is_valid_group(locked, g) ? 1 : -1;
                    assert(is_active == is_active_expected);

                    is_active = xkb_state_layout_index_is_active(
                        state, locked_expected, XKB_STATE_LAYOUT_LOCKED
                    );
                    assert(
                        is_valid_group((int32_t) locked_expected, g) == (g > 0)
                    );
                    is_active_expected =
                        is_valid_group((int32_t) locked_expected, g) ? 1 : -1;
                    assert(is_active == is_active_expected);

                    is_active = xkb_state_layout_index_is_active(
                        state, effective_expected, XKB_STATE_LAYOUT_EFFECTIVE
                    );
                    assert(
                        is_valid_group((int32_t) effective_expected, g) == (g > 0)
                    );
                    is_active_expected =
                        is_valid_group((int32_t) effective_expected, g) ? 1 : -1;
                    assert(is_active == is_active_expected);
                }
            }
        }

        xkb_state_unref(state);
        xkb_keymap_unref(keymap);
    }}
}

/* Check that derived state is correctly initialized */
static void
test_initial_derived_values(struct xkb_context *ctx)
{
    struct xkb_keymap * const keymap = test_compile_rules(
        ctx, XKB_KEYMAP_FORMAT_TEXT_V1, "evdev",
        "pc104", "us", NULL, "grp1_led:scroll"
    );
    assert(keymap);

    struct xkb_state *state;

    state = xkb_state_new(keymap);
    assert(state);
    assert(xkb_state_led_name_is_active(state, XKB_LED_NAME_SCROLL));
    xkb_state_unref(state);

    struct xkb_state_machine * const sm = xkb_state_machine_new(keymap, NULL);
    assert(sm);
    state = xkb_state_machine_get_state(sm);
    assert(xkb_state_led_name_is_active(state, XKB_LED_NAME_SCROLL));
    xkb_state_machine_unref(sm);

    xkb_keymap_unref(keymap);
}

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
    fprintf(stderr,
            "\tMods: Base: 0x%"PRIx32", Latched: 0x%"PRIx32", "
            "Locked: 0x%"PRIx32", Effective: 0x%"PRIx32"\n",
            base, latched, locked, effective);
}

static void
print_layout_serialization(struct xkb_state *state)
{
    xkb_mod_mask_t base = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_DEPRESSED);
    xkb_mod_mask_t latched = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LATCHED);
    xkb_mod_mask_t locked = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LOCKED);
    xkb_mod_mask_t effective = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE);
    fprintf(stderr,
            "\tLayout: Base: 0x%"PRIx32", Latched: 0x%"PRIx32", "
            "Locked: 0x%"PRIx32", Effective: 0x%"PRIx32"\n",
            base, latched, locked, effective);
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
        fprintf(stderr, "\tgroup %s (%"PRIu32"): %s%s%s%s\n",
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
        fprintf(stderr, "\tmod %s (%"PRIu32"): %s%s%s%s\n",
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
        fprintf(stderr, "\tled %s (%"PRIu32"): active\n",
                xkb_keymap_led_get_name(keymap, led),
                led);
    }
}

static inline bool
check_serialize_layout(enum xkb_state_component components,
                       struct xkb_state *expected, struct xkb_state *got)
{
    return
        xkb_state_serialize_layout(expected, components) ==
        xkb_state_serialize_layout(got, components);
}

static inline bool
check_serialize_mods(enum xkb_state_component components,
                     struct xkb_state *expected, struct xkb_state *got)
{
    return
        xkb_state_serialize_mods(expected, components) ==
        xkb_state_serialize_mods(got, components);
}

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

static bool
xkb_event_eq(const struct xkb_event *event1, const struct xkb_event *event2)
{
    if (event1->type != event2->type)
        return false;
    switch (event1->type) {
    case XKB_EVENT_TYPE_KEY_DOWN:
    case XKB_EVENT_TYPE_KEY_UP:
        return event1->keycode == event2->keycode;
    case XKB_EVENT_TYPE_COMPONENTS_CHANGE:
        return memcmp(&event1->components, &event2->components,
                      sizeof(event1->components)) == 0;
    default:
        {} /* Label followed by declaration requires C23 */
        static_assert(XKB_EVENT_TYPE_COMPONENTS_CHANGE == 3 &&
                      XKB_EVENT_TYPE_COMPONENTS_CHANGE ==
                      (enum xkb_event_type) _LAST_XKB_EVENT_TYPE,
                      "Missing state event type");
        return false;
    }
}

static void
print_event(const char *prefix, const struct xkb_event *event)
{
    fprintf(stderr, "%s", prefix);
    switch (event->type) {
    case XKB_EVENT_TYPE_KEY_DOWN:
        fprintf(stderr, "type: key down; keycode: %"PRIu32"\n", event->keycode);
        break;
    case XKB_EVENT_TYPE_KEY_UP:
        fprintf(stderr, "type: key up; keycode: %"PRIu32"\n", event->keycode);
        break;
    case XKB_EVENT_TYPE_COMPONENTS_CHANGE:
        fprintf(stderr, "type: components; changed: 0x%x\n"
                "\tgroup: %"PRId32" %"PRId32" %"PRId32" %"PRIu32"\n"
                "\tmods: 0x%08"PRIx32" 0x%08"PRIx32" 0x%08"PRIx32" %08"PRIx32"\n"
                "\tleds: 0x%08"PRIx32"\n"
                "\tcontrols: 0x%08"PRIx32"\n",
                event->components.changed,
                event->components.components.base_group,
                event->components.components.latched_group,
                event->components.components.locked_group,
                event->components.components.group,
                event->components.components.base_mods,
                event->components.components.latched_mods,
                event->components.components.locked_mods,
                event->components.components.mods,
                event->components.components.leds,
                event->components.components.controls);
        break;
    default:
        {} /* Label followed by declaration requires C23 */
        static_assert(XKB_EVENT_TYPE_COMPONENTS_CHANGE == 3 &&
                      XKB_EVENT_TYPE_COMPONENTS_CHANGE ==
                      (enum xkb_event_type) _LAST_XKB_EVENT_TYPE,
                      "Missing state event type");
    }
}

static bool
check_events(struct xkb_event_iterator *iter,
             const struct xkb_event *events, size_t count)
{
    const struct xkb_event *got = NULL;
    size_t got_count = 0;
    bool ok = true;
    if (count == 1 && events[0].type == XKB_EVENT_TYPE_NONE)
        count = 0;
    while ((got = xkb_event_iterator_next(iter))) {
        if (++got_count > count) {
            fprintf(stderr, "%s() error at event #%zu:\n", __func__, got_count);
            print_event("Unexpected event:\n", got);
            break;
        }
        const struct xkb_event *expected = &events[got_count - 1];
        if (!xkb_event_eq(got, expected)) {
            fprintf(stderr, "%s() error at event #%zu:\n", __func__, got_count);
            print_event("Expected: ", expected);
            print_event("Got: ", got);
            ok = false;
            break;
        }
    }
    if (got_count != count) {
        fprintf(stderr, "%s() count error: expected %zu, got: %zu\n",
                __func__, count, got_count);
        ok = false;
    }
    return ok;
}

/* Utils for checking modifier state */
typedef bool (* is_active_t)(int);

static inline bool
is_active(int x)
{
    return x > 0;
}

static inline bool
is_not_active(int x)
{
    return x == 0;
}

static void
test_update_key(struct xkb_context *ctx, struct xkb_keymap *keymap,
                bool pure_vmods)
{
    struct xkb_state *state = xkb_state_new(keymap);
    assert(state);
    struct xkb_state_machine *sm = xkb_state_machine_new(keymap, NULL);
    struct xkb_event_iterator *events =
        xkb_event_iterator_new(ctx, XKB_EVENT_ITERATOR_NO_FLAGS);
    assert(events);
    const xkb_keysym_t *syms;
    xkb_keysym_t one_sym;
    int num_syms;
    is_active_t check_active = pure_vmods ? is_not_active : is_active;

    xkb_mod_index_t ctrl = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_CTRL);
    xkb_mod_index_t mod1 = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_MOD1);
    xkb_mod_index_t alt  = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_ALT);
    xkb_mod_index_t meta = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_META);

    assert(xkb_state_mod_names_are_active(state, XKB_STATE_MODS_EFFECTIVE, 0x4,
                                          XKB_MOD_NAME_CTRL) == -2);
    assert(xkb_state_mod_indices_are_active(state, XKB_STATE_MODS_EFFECTIVE, 0x4,
                                            ctrl) == -2);

#define update_states(state1, sm, key, direction) do {                     \
    xkb_state_update_key((state1), (key), (direction));                    \
    assert(xkb_state_machine_update_key((sm), (events), (key), (direction))\
           == 0);                                                          \
} while (0)

#define check_events_(got, ...) do {                            \
    const struct xkb_event expected[] = { __VA_ARGS__ };        \
    assert(check_events((got), expected, ARRAY_SIZE(expected)));\
} while (0)

    /* LCtrl down */
    update_states(state, sm, KEY_LEFTCTRL + EVDEV_OFFSET, XKB_KEY_DOWN);
    fprintf(stderr, "dumping state for LCtrl down:\n");
    print_state(state);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_KEY_DOWN,
            .keycode = KEY_LEFTCTRL + EVDEV_OFFSET
        },
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
            .components = {
                .components = {
                    .base_mods = xkb_keymap_mod_get_mask2(keymap, ctrl),
                    .mods = xkb_keymap_mod_get_mask2(keymap, ctrl),
                },
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE
            }
        }
    );
    assert(xkb_state_mod_name_is_active(state, XKB_MOD_NAME_CTRL,
                                        XKB_STATE_MODS_DEPRESSED) > 0);

    /* LCtrl + RAlt down */
    update_states(state, sm, KEY_RIGHTALT + EVDEV_OFFSET, XKB_KEY_DOWN);
    fprintf(stderr, "dumping state for LCtrl + RAlt down:\n");
    print_state(state);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_KEY_DOWN,
            .keycode = KEY_RIGHTALT + EVDEV_OFFSET
        },
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
            .components = {
                .components = {
                    .base_mods = xkb_keymap_mod_get_mask2(keymap, ctrl)
                               | xkb_keymap_mod_get_mask2(keymap, alt),
                    .mods = xkb_keymap_mod_get_mask2(keymap, ctrl)
                          | xkb_keymap_mod_get_mask2(keymap, alt),
                },
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE
            }
        }
    );
    assert(xkb_state_mod_name_is_active(state, XKB_MOD_NAME_CTRL,
                                        XKB_STATE_MODS_DEPRESSED) > 0);
    assert(xkb_state_mod_name_is_active(state, XKB_VMOD_NAME_ALT,
                                        XKB_STATE_MODS_DEPRESSED) > 0);
    assert(check_active(xkb_state_mod_name_is_active(state, XKB_MOD_NAME_MOD1,
                                                     XKB_STATE_MODS_DEPRESSED)));
    assert(check_active(xkb_state_mod_name_is_active(state, XKB_VMOD_NAME_META,
                                                     XKB_STATE_MODS_DEPRESSED)));
    if (pure_vmods) {
        assert(xkb_state_mod_names_are_active(state, XKB_STATE_MODS_DEPRESSED,
                                              XKB_STATE_MATCH_ALL,
                                              XKB_MOD_NAME_CTRL,
                                              XKB_VMOD_NAME_ALT,
                                              NULL) > 0);
        assert(xkb_state_mod_indices_are_active(state, XKB_STATE_MODS_DEPRESSED,
                                                XKB_STATE_MATCH_ALL, ctrl, alt,
                                                XKB_MOD_INVALID) > 0);
        assert(xkb_state_mod_names_are_active(state, XKB_STATE_MODS_DEPRESSED,
                                              XKB_STATE_MATCH_ALL,
                                              XKB_MOD_NAME_MOD1,
                                              XKB_VMOD_NAME_META,
                                              NULL) == 0);
        assert(xkb_state_mod_indices_are_active(state, XKB_STATE_MODS_DEPRESSED,
                                                XKB_STATE_MATCH_ALL,
                                                mod1, meta,
                                                XKB_MOD_INVALID) == 0);
    } else {
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
    }
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
                                          XKB_VMOD_NAME_ALT,
                                          NULL) > 0);
    assert(xkb_state_mod_names_are_active(state, XKB_STATE_MODS_DEPRESSED,
                                          (XKB_STATE_MATCH_ANY |
                                           XKB_STATE_MATCH_NON_EXCLUSIVE),
                                          XKB_VMOD_NAME_ALT,
                                          NULL) > 0);
    assert(check_active(xkb_state_mod_names_are_active(
                            state, XKB_STATE_MODS_DEPRESSED,
                            XKB_STATE_MATCH_ALL | XKB_STATE_MATCH_NON_EXCLUSIVE,
                            XKB_MOD_NAME_MOD1,
                            NULL)));
    assert(check_active(xkb_state_mod_names_are_active(
                            state, XKB_STATE_MODS_DEPRESSED,
                            XKB_STATE_MATCH_ALL | XKB_STATE_MATCH_NON_EXCLUSIVE,
                            XKB_VMOD_NAME_META,
                            NULL)));
    assert(check_active(xkb_state_mod_names_are_active(
                            state, XKB_STATE_MODS_DEPRESSED,
                            (XKB_STATE_MATCH_ANY | XKB_STATE_MATCH_NON_EXCLUSIVE),
                            XKB_MOD_NAME_MOD1,
                            NULL)));
    assert(check_active(xkb_state_mod_names_are_active(
                            state, XKB_STATE_MODS_DEPRESSED,
                            (XKB_STATE_MATCH_ANY | XKB_STATE_MATCH_NON_EXCLUSIVE),
                            XKB_VMOD_NAME_META,
                            NULL)));

    /* RAlt down */
    update_states(state, sm, KEY_LEFTCTRL + EVDEV_OFFSET, XKB_KEY_UP);
    fprintf(stderr, "dumping state for RAlt down:\n");
    print_state(state);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_KEY_UP,
            .keycode = KEY_LEFTCTRL + EVDEV_OFFSET
        },
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
            .components = {
                .components = {
                    .base_mods = xkb_keymap_mod_get_mask2(keymap, alt),
                    .mods = xkb_keymap_mod_get_mask2(keymap, alt),
                },
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE
            }
        }
    );
    assert(xkb_state_mod_name_is_active(state, XKB_MOD_NAME_CTRL,
                                        XKB_STATE_MODS_EFFECTIVE) == 0);
    assert(xkb_state_mod_name_is_active(state, XKB_VMOD_NAME_ALT,
                                        XKB_STATE_MODS_DEPRESSED) > 0);
    assert(check_active(xkb_state_mod_name_is_active(state, XKB_MOD_NAME_MOD1,
                                                     XKB_STATE_MODS_DEPRESSED)));
    assert(check_active(xkb_state_mod_name_is_active(state, XKB_VMOD_NAME_META,
                                                     XKB_STATE_MODS_DEPRESSED)));
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
    update_states(state, sm, KEY_RIGHTALT + EVDEV_OFFSET, XKB_KEY_UP);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_KEY_UP,
            .keycode = KEY_RIGHTALT + EVDEV_OFFSET
        },
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
            .components = {
                .components = { .base_mods = 0, .mods = 0, },
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE
            }
        }
    );
    assert(xkb_state_mod_name_is_active(state, XKB_MOD_NAME_MOD1,
                                        XKB_STATE_MODS_EFFECTIVE) == 0);
    assert(xkb_state_mod_name_is_active(state, XKB_VMOD_NAME_ALT,
                                        XKB_STATE_MODS_EFFECTIVE) == 0);
    assert(xkb_state_mod_name_is_active(state, XKB_VMOD_NAME_META,
                                        XKB_STATE_MODS_EFFECTIVE) == 0);

    /* Caps locked */
    update_states(state, sm, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_DOWN);
    assert(xkb_state_mod_name_is_active(state, XKB_MOD_NAME_CAPS,
                                        XKB_STATE_MODS_DEPRESSED) > 0);
    update_states(state, sm, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_UP);
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
    update_states(state, sm, KEY_NUMLOCK + EVDEV_OFFSET, XKB_KEY_DOWN);
    update_states(state, sm, KEY_NUMLOCK + EVDEV_OFFSET, XKB_KEY_UP);
    fprintf(stderr, "dumping state for Caps Lock + Num Lock:\n");
    print_state(state);
    assert(xkb_state_mod_name_is_active(state, XKB_MOD_NAME_CAPS,
                                        XKB_STATE_MODS_LOCKED) > 0);
    assert(xkb_state_mod_name_is_active(state, XKB_VMOD_NAME_NUM,
                                        XKB_STATE_MODS_LOCKED) > 0);
    assert(check_active(xkb_state_mod_name_is_active(state, XKB_MOD_NAME_MOD2,
                                                     XKB_STATE_MODS_LOCKED)));
    num_syms = xkb_state_key_get_syms(state, KEY_KP1 + EVDEV_OFFSET, &syms);
    assert(num_syms == 1 && syms[0] == XKB_KEY_KP_1);
    assert(xkb_state_led_name_is_active(state, XKB_LED_NAME_NUM) > 0);

    /* Num Lock unlocked */
    update_states(state, sm, KEY_NUMLOCK + EVDEV_OFFSET, XKB_KEY_DOWN);
    update_states(state, sm, KEY_NUMLOCK + EVDEV_OFFSET, XKB_KEY_UP);

    /* Switch to group 2 */
    update_states(state, sm, KEY_COMPOSE + EVDEV_OFFSET, XKB_KEY_DOWN);
    update_states(state, sm, KEY_COMPOSE + EVDEV_OFFSET, XKB_KEY_UP);
    assert(xkb_state_led_name_is_active(state, "Group 2") > 0);
    assert(xkb_state_led_name_is_active(state, XKB_LED_NAME_NUM) == 0);

    /* Switch back to group 1. */
    update_states(state, sm, KEY_COMPOSE + EVDEV_OFFSET, XKB_KEY_DOWN);
    update_states(state, sm, KEY_COMPOSE + EVDEV_OFFSET, XKB_KEY_UP);

    /* Caps unlocked */
    update_states(state, sm, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_DOWN);
    update_states(state, sm, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_UP);
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
    update_states(state, sm, KEY_6 + EVDEV_OFFSET, XKB_KEY_DOWN);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_KEY_DOWN,
            .keycode = KEY_6 + EVDEV_OFFSET
        }
    );
    update_states(state, sm, KEY_6 + EVDEV_OFFSET, XKB_KEY_UP);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_KEY_UP,
            .keycode = KEY_6 + EVDEV_OFFSET
        }
    );

    one_sym = xkb_state_key_get_one_sym(state, KEY_5 + EVDEV_OFFSET);
    assert(one_sym == XKB_KEY_5);

    xkb_event_iterator_destroy(events);
    xkb_state_machine_unref(sm);
    xkb_state_unref(state);

#undef update_states
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
    .key = { .keycode=(_keycode) + EVDEV_OFFSET, \
             .direction=(_direction),            \
             .keysym=(_keysym) }

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
                          components->latched_group,
                          components->locked_group);

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

static bool
run_state_update(struct xkb_keymap *keymap,
                 const struct test_state_components *components,
                 struct xkb_state **expected, struct xkb_state **got)
{
    xkb_keysym_t keysym = XKB_KEY_NoSymbol;
    enum xkb_state_component changes = 0;
    switch (components->input_type) {
        case INPUT_TYPE_COMPONENTS:
            changes = xkb_state_update_latched_locked(
                *got,
                components->input.affect_latched_mods,
                components->input.latched_mods,
                components->input.affect_latched_group,
                components->input.latched_group,
                components->input.affect_locked_mods,
                components->input.locked_mods,
                components->input.affect_locked_group,
                components->input.locked_group
            );
            break;
        case INPUT_TYPE_KEY:
            keysym =
                xkb_state_key_get_one_sym(*got, components->key.keycode);
            if (components->key.direction == DOWN ||
                components->key.direction == BOTH)
                changes = xkb_state_update_key(
                    *got, components->key.keycode, XKB_KEY_DOWN
                );
            if (components->key.direction == UP ||
                components->key.direction == BOTH)
                changes = xkb_state_update_key(
                    *got, components->key.keycode, XKB_KEY_UP
                );
            break;
        case INPUT_TYPE_RESET:
            xkb_state_unref(*got);
            xkb_state_unref(*expected);
            *got = xkb_state_new(keymap);
            *expected = xkb_state_new(keymap);
            assert(*got);
            assert(*expected);
            return true;
        default:
            assert_printf(false, "Unsupported input type: %d\n",
                          components->input_type);
    }
    return check_update_state(
        keymap, components, *expected, *got, keysym, changes
    );
}

static void
test_update_latched_locked(struct xkb_keymap *keymap)
{
    struct xkb_state *state = xkb_state_new(keymap);
    struct xkb_state *expected = xkb_state_new(keymap);
    assert(state);
    assert(expected);

    xkb_mod_mask_t shift    = xkb_keymap_mod_get_mask(keymap, XKB_MOD_NAME_SHIFT);
    xkb_mod_mask_t capslock = xkb_keymap_mod_get_mask(keymap, XKB_MOD_NAME_CAPS);
    xkb_mod_mask_t control  = xkb_keymap_mod_get_mask(keymap, XKB_MOD_NAME_CTRL);
    xkb_mod_mask_t level3   = xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_LEVEL3);
    xkb_led_index_t capslock_led_idx = _xkb_keymap_led_get_index(keymap, XKB_LED_NAME_CAPS);
    xkb_led_index_t group2_led_idx   = _xkb_keymap_led_get_index(keymap, "Group 2");
    xkb_led_mask_t capslock_led = UINT32_C(1) << capslock_led_idx;
    xkb_led_mask_t group2_led   = UINT32_C(1) << group2_led_idx;

    const struct test_state_components test_data[] = {
        { KEY_ENTRY(KEY_A, BOTH, XKB_KEY_a), .changes = 0 },

        /*
         * Groups: lock
         */
#define GROUP_LOCK_ENTRY(group) \
        COMPONENTS_ENTRY(.affect_locked_group = true, .locked_group = (group))
#define GROUP_LOCK_CHANGES \
        (XKB_STATE_LAYOUT_LOCKED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS)

        {
            GROUP_LOCK_ENTRY(1),
            .locked_group=1, .group=1,
            .leds=group2_led,
            .changes=GROUP_LOCK_CHANGES
        },
        {
            KEY_ENTRY(KEY_A, BOTH, XKB_KEY_Cyrillic_ef),
            .locked_group=1, .group=1,
            .leds = group2_led,
            .changes = 0
        },
        {
            GROUP_LOCK_ENTRY(0),
            .locked_group=0, .group=0,
            .leds=0,
            .changes=GROUP_LOCK_CHANGES
        },
        {
            GROUP_LOCK_ENTRY(0),
            .locked_group=0, .group=0,
            .leds=0,
            .changes=0
        },
        {
            GROUP_LOCK_ENTRY(-2),
            .locked_group=0, .group=0, /* Wrapped */
            .leds=0,
            .changes=0
        },
        {
            GROUP_LOCK_ENTRY(-2),
            .locked_group=0, .group=0, /* Wrapped */
            .leds=0,
            .changes=0
        },
        {
            GROUP_LOCK_ENTRY(-1),
            .locked_group=1, .group=1, /* Wrapped */
            .leds=group2_led,
            .changes=GROUP_LOCK_CHANGES
        },
        {
            GROUP_LOCK_ENTRY(-1),
            .locked_group=1, .group=1, /* Wrapped */
            .leds=group2_led,
            .changes=0
        },
        {
            GROUP_LOCK_ENTRY(0),
            .locked_group=0, .group=0,
            .leds=0,
            .changes=GROUP_LOCK_CHANGES
        },
        {
            GROUP_LOCK_ENTRY(1),
            .locked_group=1, .group=1,
            .leds=group2_led,
            .changes=GROUP_LOCK_CHANGES
        },
        {
            GROUP_LOCK_ENTRY(1),
            .locked_group=1, .group=1,
            .leds=group2_led,
            .changes=0
        },
        {
            GROUP_LOCK_ENTRY(2),
            .locked_group=0, .group=0, /* Wrapped */
            .leds=0,
            .changes=GROUP_LOCK_CHANGES
        },
        {
            GROUP_LOCK_ENTRY(2),
            .locked_group=0, .group=0, /* Wrapped */
            .leds=0,
            .changes=0
        },
        /* Invalid group */
        {
            GROUP_LOCK_ENTRY(XKB_MAX_GROUPS),
            .locked_group=0, .group=0,
            .leds=0,
            .changes=0
        },
        /* Previous lock */
        {
            KEY_ENTRY(KEY_COMPOSE, DOWN, XKB_KEY_ISO_Next_Group),
            .locked_group = 1, .group = 1,
            .leds = group2_led,
            .changes = XKB_STATE_LAYOUT_LOCKED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS
        },
        {
            KEY_ENTRY(KEY_COMPOSE, UP, XKB_KEY_ISO_Next_Group),
            .locked_group = 1, .group = 1,
            .leds = group2_led,
            .changes = 0
        },
        {
            KEY_ENTRY(KEY_A, BOTH, XKB_KEY_Cyrillic_ef),
            .locked_group=1, .group=1,
            .leds = group2_led,
            .changes = 0
        },
        {
            GROUP_LOCK_ENTRY(0),
            .locked_group=0, .group=0,
            .leds=group2_led,
            .changes=XKB_STATE_LAYOUT_LOCKED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS
        },
        { KEY_ENTRY(KEY_A, BOTH, XKB_KEY_a), .changes = 0 },
        {
            KEY_ENTRY(KEY_COMPOSE, DOWN, XKB_KEY_ISO_Next_Group),
            .locked_group = 1, .group = 1,
            .leds = group2_led,
            .changes = XKB_STATE_LAYOUT_LOCKED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS
        },
        {
            KEY_ENTRY(KEY_COMPOSE, UP, XKB_KEY_ISO_Next_Group),
            .locked_group = 1, .group = 1,
            .leds = group2_led,
            .changes = 0
        },

        /*
         * Groups: latch
         */
#define GROUP_LATCH_ENTRY(group) \
        COMPONENTS_ENTRY(.affect_latched_group = true, .latched_group = (group))

        RESET_STATE,
        {
          KEY_ENTRY(KEY_A, BOTH, XKB_KEY_a),
          .base_group = 0, .latched_group = 0, .locked_group = 0, .group = 0,
          .base_mods = 0, .latched_mods = 0, .locked_mods = 0, .mods = 0,
          .leds = 0,
          .changes = 0
        },
        {
            GROUP_LATCH_ENTRY(1),
            .latched_group=1, .group=1,
            .leds=group2_led,
            .changes=XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS
        },
        {
            KEY_ENTRY(KEY_A, DOWN, XKB_KEY_Cyrillic_ef),
            .changes = XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS
        },
        { KEY_ENTRY(KEY_A, UP, XKB_KEY_a), .changes = 0 },
        { KEY_ENTRY(KEY_A, DOWN, XKB_KEY_a), .changes = 0 },
        { KEY_ENTRY(KEY_A, UP, XKB_KEY_a), .changes = 0 },
        {
            GROUP_LATCH_ENTRY(1),
            .latched_group=1, .group=1,
            .leds=group2_led,
            .changes=XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS
        },
        {
            GROUP_LATCH_ENTRY(1),
            .latched_group=1, .group=1,
            .leds=group2_led,
            .changes=0
        },
        {
            KEY_ENTRY(KEY_A, DOWN, XKB_KEY_Cyrillic_ef),
            .changes = XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS
        },
        { KEY_ENTRY(KEY_A, UP, XKB_KEY_a), .changes = 0 },
        { KEY_ENTRY(KEY_A, DOWN, XKB_KEY_a), .changes = 0 },
        { KEY_ENTRY(KEY_A, UP, XKB_KEY_a), .changes = 0 },
        /* Invalid group */
        {
            GROUP_LATCH_ENTRY(XKB_MAX_GROUPS),
            .latched_group=XKB_MAX_GROUPS, .group=0,
            .leds=0,
            .changes=XKB_STATE_LAYOUT_LATCHED
        },
        /* Pending latch is cancelled */
        RESET_STATE,
        {
            KEY_ENTRY(KEY_LEFTMETA, BOTH, XKB_KEY_ISO_Group_Latch),
            .latched_group = 1, .group = 1,
            .leds = group2_led,
            .changes = XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_DEPRESSED
        },
        {
            GROUP_LATCH_ENTRY(2),
            .latched_group=2, .group=0,
            .leds = 0,
            .changes=XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS
        },
        { KEY_ENTRY(KEY_A, DOWN, XKB_KEY_a), .changes = XKB_STATE_LAYOUT_LATCHED },
        /* Pending latch to lock is cancelled */
        RESET_STATE,
        {
            KEY_ENTRY(KEY_RIGHTMETA, BOTH, XKB_KEY_ISO_Group_Latch),
            .latched_group = 1, .group = 1,
            .leds = group2_led,
            .changes = XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_DEPRESSED
        },
        {
            GROUP_LATCH_ENTRY(2),
            .latched_group=2, .group=0,
            .leds = 0,
            .changes=XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS
        },
        {
            KEY_ENTRY(KEY_A, DOWN, XKB_KEY_a),
            .changes = XKB_STATE_LAYOUT_LATCHED
        },
        /* Broken latch does not unlock if clearLocks is not set */
        RESET_STATE,
        {
            GROUP_LOCK_ENTRY(1),
            .locked_group=1, .group=1,
            .leds=group2_led,
            .changes=XKB_STATE_LAYOUT_LOCKED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS
        },
        {
            KEY_ENTRY(KEY_SCROLLLOCK, DOWN, XKB_KEY_ISO_Group_Latch),
            .base_group=1, .latched_group=0, .locked_group=1, .group=0,
            .leds=0,
            .changes=XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS
        },
        {
            /* Breaks latch */
            KEY_ENTRY(KEY_A, DOWN, XKB_KEY_a),
            .base_group=1, .latched_group=0, .locked_group=1, .group=0,
            .leds=0,
            .changes=0
        },
        {
            KEY_ENTRY(KEY_SCROLLLOCK, UP, XKB_KEY_ISO_Group_Latch),
            .base_group=0, .latched_group=0, .locked_group=1, .group=1,
            .leds=group2_led,
            .changes=XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS
        },

        /*
         * Groups: latch + lock
         */
        RESET_STATE,
        /* Empty state */
        {
            COMPONENTS_ENTRY(.affect_latched_group = true, .latched_group = 1,
                             .affect_locked_group = true, .locked_group = 1),
            .latched_group = 1, .locked_group = 1, .group = 0,
            .changes = XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_LOCKED
        },
        /* Pending latch */
        RESET_STATE,
        {
            KEY_ENTRY(KEY_LEFTMETA, BOTH, XKB_KEY_ISO_Group_Latch),
            .latched_group = 1, .group = 1,
            .leds = group2_led,
            .changes = XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_DEPRESSED
        },
        {
            COMPONENTS_ENTRY(.affect_locked_group = true, .locked_group = 1),
            .latched_group = 1, .locked_group = 1, .group = 0,
            .changes = XKB_STATE_LAYOUT_LOCKED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS
        },
        {
            KEY_ENTRY(KEY_A, DOWN, XKB_KEY_a),
            .locked_group=1, .group=1,
            .leds = group2_led,
            .changes = XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS
        },

        /*
         * Modifiers: lock
         */
#define MOD_LOCK_ENTRY(mask, mods) \
        COMPONENTS_ENTRY(.affect_locked_mods = (mask), .locked_mods = (mods))
#define UNDEFINED_MODMASK (UINT32_C(1) << (XKB_MAX_MODS - 1))

        RESET_STATE,
        /* Invalid: mod not in the mask */
        { MOD_LOCK_ENTRY(0, capslock), .changes=0 },
        { MOD_LOCK_ENTRY(0, UNDEFINED_MODMASK), .changes=0 },
        /* Set Caps */
        {
            MOD_LOCK_ENTRY(capslock, capslock),
            .locked_mods=capslock, .mods=capslock,
            .leds=capslock_led,
            .changes=XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE | XKB_STATE_LEDS
        },
        {
            MOD_LOCK_ENTRY(capslock, capslock),
            .locked_mods=capslock, .mods=capslock, .leds=capslock_led,
            .changes=0
        },
        {
            KEY_ENTRY(KEY_A, BOTH, XKB_KEY_A),
            .locked_mods=capslock, .mods=capslock,
            .leds=capslock_led,
            .changes = 0
        },
        /* Add Control and keep Caps */
        {
            MOD_LOCK_ENTRY(control, control),
            .locked_mods=control | capslock, .mods=control | capslock,
            .leds=capslock_led,
            .changes=XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE
        },
        {
            KEY_ENTRY(KEY_A, BOTH, XKB_KEY_A),
            .locked_mods=control | capslock, .mods=control | capslock,
            .leds=capslock_led,
            .changes = 0
        },
        /* Remove Caps and keep Control */
        {
            MOD_LOCK_ENTRY(capslock, 0),
            .locked_mods=control, .mods=control,
            .changes=XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE | XKB_STATE_LEDS
        },
        {
            KEY_ENTRY(KEY_A, BOTH, XKB_KEY_a),
            .locked_mods=control, .mods=control,
            .leds=0,
            .changes = 0
        },
        /* Add Level3 and remove Control */
        {
            MOD_LOCK_ENTRY(level3 | control, level3),
            .locked_mods=level3, .mods=level3,
            .changes=XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE
        },
        /* Change undefined modifier */
        {
            MOD_LOCK_ENTRY(level3, level3 | UNDEFINED_MODMASK),
            .locked_mods=level3, .mods=level3,
            .changes=0
        },
        {
            MOD_LOCK_ENTRY(level3 | UNDEFINED_MODMASK, level3 | UNDEFINED_MODMASK),
            .locked_mods=level3, .mods=level3,
            .changes=0
        },
        {
            MOD_LOCK_ENTRY(level3 | UNDEFINED_MODMASK, level3),
            .locked_mods=level3, .mods=level3,
            .changes=0
        },
        /* Previous lock */
        RESET_STATE,
        {
            KEY_ENTRY(KEY_CAPSLOCK, BOTH, XKB_KEY_Caps_Lock),
            .locked_mods=capslock, .mods=capslock,
            .leds=capslock_led,
            .changes = XKB_STATE_MODS_DEPRESSED
        },
        {
            MOD_LOCK_ENTRY(level3 | control, level3),
            .locked_mods=capslock | level3, .mods=capslock | level3,
            .leds=capslock_led,
            .changes=XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE },
        {
            MOD_LOCK_ENTRY(capslock, 0),
            .locked_mods=level3, .mods=level3,
            .leds=0,
            .changes=XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE | XKB_STATE_LEDS
        },

        /*
         * Modifiers: latch
         */
#define MODS_LATCH_ENTRY(mask, mods) \
        COMPONENTS_ENTRY(.affect_latched_mods = (mask), .latched_mods = (mods))

        RESET_STATE,
        { KEY_ENTRY(KEY_A, BOTH, XKB_KEY_a), .changes = 0 },
        /* Invalid: mod not in the mask */
        { MODS_LATCH_ENTRY(0, shift), .changes=0 },
        { MODS_LATCH_ENTRY(0, UNDEFINED_MODMASK), .changes=0 },
        /* Latch Shift */
        {
            MODS_LATCH_ENTRY(shift, shift),
            .latched_mods=shift, .mods=shift,
            .changes=XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE
        },
        {
            KEY_ENTRY(KEY_A, DOWN, XKB_KEY_A),
            .changes = XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE
        },
        { KEY_ENTRY(KEY_A, UP, XKB_KEY_a), .changes = 0 },
        { KEY_ENTRY(KEY_A, BOTH, XKB_KEY_a), .changes = 0 },
        {
            MODS_LATCH_ENTRY(shift, shift),
            .latched_mods=shift, .mods=shift,
            .changes=XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE
        },
        {
            MODS_LATCH_ENTRY(shift, shift),
            .latched_mods=shift, .mods=shift,
            .changes=0
        },
        {
            KEY_ENTRY(KEY_A, DOWN, XKB_KEY_A),
            .changes = XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE
        },
        { KEY_ENTRY(KEY_A, UP, XKB_KEY_a), .changes = 0 },
        { KEY_ENTRY(KEY_A, BOTH, XKB_KEY_a), .changes = 0 },
        /* Latch Shift, then Caps: latched shift is cancelled */
        {
            MODS_LATCH_ENTRY(shift, shift),
            .latched_mods=shift, .mods=shift,
            .changes=XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE
        },
        {
            MODS_LATCH_ENTRY(capslock, capslock),
            .latched_mods=shift | capslock, .mods=shift | capslock,
            .changes=XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE
        },
        {
            KEY_ENTRY(KEY_A, DOWN, XKB_KEY_a),
            .changes = XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE
        },
        { KEY_ENTRY(KEY_A, UP, XKB_KEY_a), .changes = 0 },
        /* Change undefined modifier */
        {
            MODS_LATCH_ENTRY(level3, level3 | UNDEFINED_MODMASK),
            .latched_mods=level3, .mods=level3,
            .changes=XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE
        },
        {
            MODS_LATCH_ENTRY(level3 | UNDEFINED_MODMASK, level3 | UNDEFINED_MODMASK),
            .latched_mods=level3, .mods=level3,
            .changes=0
        },
        {
            MODS_LATCH_ENTRY(level3 | UNDEFINED_MODMASK, level3),
            .latched_mods=level3, .mods=level3,
            .changes=0
        },
        /* Pending latch is *not* cancelled if not in affected mods */
        RESET_STATE,
        {
            KEY_ENTRY(KEY_102ND, BOTH, XKB_KEY_ISO_Level3_Latch),
            .latched_mods=level3, .mods=level3,
            .changes = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED
        },
        {
            MODS_LATCH_ENTRY(shift, shift),
            .latched_mods=shift | level3, .mods=shift | level3,
            .changes=XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE
        },
        {
            KEY_ENTRY(KEY_A, DOWN, XKB_KEY_A),
            .changes = XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE
        },
        /* Pending latch *is* cancelled if in affected mods */
        RESET_STATE,
        {
            KEY_ENTRY(KEY_102ND, BOTH, XKB_KEY_ISO_Level3_Latch),
            .latched_mods=level3, .mods=level3,
            .changes = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED
        },
        {
            MODS_LATCH_ENTRY(shift | level3, shift),
            .latched_mods=shift, .mods=shift,
            .changes=XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE
        },
        {
            KEY_ENTRY(KEY_A, DOWN, XKB_KEY_A),
            .changes = XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE
        },
        /* Latch-to-lock */
        RESET_STATE,
        {
            /* Set capslock, to ensure that when *mutating* the latch to a lock,
             * the `priv` field and refcnt fields are set accordingly. */
            MOD_LOCK_ENTRY(capslock, capslock),
            .locked_mods=capslock, .mods=capslock,
            .changes=XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE | XKB_STATE_LEDS
        },
        {
            KEY_ENTRY(KEY_102ND, BOTH, XKB_KEY_ISO_Level3_Latch),
            /* Pending latch */
            .base_mods=0, .latched_mods=level3, .locked_mods=capslock, .mods=capslock | level3,
            .changes = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED
        },
        {
            KEY_ENTRY(KEY_102ND, BOTH, XKB_KEY_ISO_Level3_Latch),
            /* Mutate to a lock */
            .base_mods=0, .latched_mods=0, .locked_mods=capslock | level3, .mods=capslock | level3,
            .changes = XKB_STATE_MODS_DEPRESSED
        },
        {
            KEY_ENTRY(KEY_102ND, BOTH, XKB_KEY_ISO_Level3_Latch),
            /* Unlock via latch’s ACTION_LOCK_CLEAR */
            .base_mods=0, .latched_mods=0, .locked_mods=capslock, .mods=capslock,
            .changes = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE
        },
        /* Broken latch does not unlock if clearLocks is not set */
        RESET_STATE,
        {
            /* Set capslock, to ensure that when *mutating* the latch to a lock,
             * the `priv` field and refcnt fields are set accordingly. */
            MOD_LOCK_ENTRY(level3, level3),
            .locked_mods=level3, .mods=level3,
            .changes=XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE
        },
        {
            KEY_ENTRY(KEY_BACKSLASH, DOWN, XKB_KEY_ISO_Level3_Latch),
            .base_mods=level3, .latched_mods=0, .locked_mods=level3, .mods=level3,
            .changes = XKB_STATE_MODS_DEPRESSED
        },
        {
            /* Breaks latch */
            KEY_ENTRY(KEY_A, DOWN, XKB_KEY_a),
            .base_mods=level3, .latched_mods=0, .locked_mods=level3, .mods=level3,
            .changes = 0
        },
        {
            KEY_ENTRY(KEY_BACKSLASH, UP, XKB_KEY_ISO_Level3_Latch),
            .base_mods=0, .latched_mods=0, .locked_mods=level3, .mods=level3,
            .changes = XKB_STATE_MODS_DEPRESSED
        },
        // TODO

        /*
         * Modifiers: latched + locked
         */
        RESET_STATE,
        {
            COMPONENTS_ENTRY(.affect_latched_mods = shift, .latched_mods = shift,
                             .affect_locked_mods = level3, .locked_mods = level3),
            .latched_mods = shift, .locked_mods = level3, .mods = shift | level3,
            .changes = XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_LOCKED |
                       XKB_STATE_MODS_EFFECTIVE
        },
        // TODO

        /*
         * Mix
         */

        /* Lock mods & groups */
        RESET_STATE,
        {
            COMPONENTS_ENTRY(.affect_locked_group=true, .locked_group=1,
                             .affect_locked_mods=control, .locked_mods=control),
            .locked_group=1, .group=1,
            .locked_mods=control, .mods=control,
            .leds=group2_led,
            .changes = XKB_STATE_LAYOUT_LOCKED | XKB_STATE_LAYOUT_EFFECTIVE |
                       XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE | XKB_STATE_LEDS
        },
        /* When updating latches, mod/group changes should not affect each other */
        RESET_STATE,
        {
            COMPONENTS_ENTRY(.affect_latched_group=true, .latched_group=1,
                             .affect_latched_mods=control, .latched_mods=control),
            .latched_group=1, .group=1,
            .latched_mods=control, .mods=control,
            .leds=group2_led,
            .changes = XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_EFFECTIVE |
                       XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE | XKB_STATE_LEDS
        },
        {
            KEY_ENTRY(KEY_A, DOWN, XKB_KEY_Cyrillic_ef),
            .changes = XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS |
                       XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE
        },
        RESET_STATE,
        {
            KEY_ENTRY(KEY_LEFTMETA, BOTH, XKB_KEY_ISO_Group_Latch),
            .latched_group = 1, .group = 1,
            .leds = group2_led,
            .changes = XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_DEPRESSED
        },
        /* Pending group latch */
        {  COMPONENTS_ENTRY(.affect_latched_mods=shift, .latched_mods=shift),
            .latched_group=1, .group=1,
            .latched_mods=shift, .mods=shift,
            .leds = group2_led,
            .changes = XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE
        },
        {
            KEY_ENTRY(KEY_A, DOWN, XKB_KEY_Cyrillic_EF),
            .leds = group2_led,
            .changes = XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_EFFECTIVE |
                       XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE | XKB_STATE_LEDS
        },
        {
            KEY_ENTRY(KEY_RIGHTMETA, BOTH, XKB_KEY_ISO_Group_Latch),
            .latched_group = 1, .group = 1,
            .leds = group2_led,
            .changes = XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_DEPRESSED
        },
        /* Pending group latch (with latch to lock + clear) */
        {
            COMPONENTS_ENTRY(.affect_latched_mods=shift, .latched_mods=shift),
            .latched_group=1, .group=1,
            .latched_mods=shift, .mods=shift,
            .leds = group2_led,
            .changes = XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE
        },
        {
            KEY_ENTRY(KEY_A, DOWN, XKB_KEY_Cyrillic_EF),
            .leds = group2_led,
            .changes = XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_EFFECTIVE |
                       XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE | XKB_STATE_LEDS
        },
        // TODO
    };
    for (size_t k = 0; k < ARRAY_SIZE(test_data); k++) {
        assert_printf(
            run_state_update(keymap, &test_data[k], &expected, &state),
            "%s #%zu: type: %d\n",
            __func__, k, test_data[k].input_type
        );
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
test_serialisation(struct xkb_keymap *keymap, bool pure_vmods)
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
    xkb_mod_mask_t shift  = (UINT32_C(1) << shiftIdx);
    xkb_mod_mask_t caps   = (UINT32_C(1) << capsIdx);
    xkb_mod_mask_t ctrl   = (UINT32_C(1) << ctrlIdx);
    xkb_mod_mask_t mod1   = (UINT32_C(1) << mod1Idx);
    xkb_mod_mask_t mod2   = (UINT32_C(1) << mod2Idx);
    xkb_mod_mask_t mod3   = (UINT32_C(1) << mod3Idx);
    xkb_mod_mask_t mod4   = (UINT32_C(1) << mod4Idx);
    xkb_mod_mask_t mod5   = (UINT32_C(1) << mod5Idx);
    xkb_mod_mask_t alt    = (UINT32_C(1) << altIdx);
    xkb_mod_mask_t meta   = (UINT32_C(1) << metaIdx);
    xkb_mod_mask_t super  = (UINT32_C(1) << superIdx);
    xkb_mod_mask_t hyper  = (UINT32_C(1) << hyperIdx);
    xkb_mod_mask_t num    = (UINT32_C(1) << numIdx);
    xkb_mod_mask_t level3 = (UINT32_C(1) << level3Idx);
    xkb_mod_mask_t altGr  = (UINT32_C(1) << altGrIdx);

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

    const struct test_active_mods_entry test_data_real[] = {
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
    const struct test_active_mods_entry test_data_virtual[] = {
        { .state = 0,            .active = 0                         },
        { .state = shift,        .active = shift                     },
        { .state = caps,         .active = caps                      },
        { .state = ctrl,         .active = ctrl                      },
        { .state = mod1,         .active = mod1                      },
        { .state = mod2,         .active = mod2                      },
        { .state = mod3,         .active = mod3                      },
        { .state = mod4,         .active = mod4                      },
        { .state = mod5,         .active = mod5                      },
        { .state = alt,          .active = alt                       },
        { .state = meta,         .active = meta                      },
        { .state = super,        .active = super                     },
        { .state = hyper,        .active = hyper                     },
        { .state = num,          .active = num                       },
        { .state = level3,       .active = level3                    },
        { .state = shift | mod1, .active = shift | mod1              },
        { .state = mod1 | alt,   .active = mod1 | alt                },
        { .state = alt | meta,   .active = alt | meta                },
        { .state = alt | level3, .active = alt | level3              },
    };
    const struct test_active_mods_entry *test_data = pure_vmods
                                                   ? test_data_virtual
                                                   : test_data_real;
    unsigned int k_max = pure_vmods
                       ? ARRAY_SIZE(test_data_virtual)
                       : ARRAY_SIZE(test_data_real);

    for (unsigned int k = 0; k < k_max; k++) {
        const struct test_active_mods_entry *entry = &test_data[k];
#define check_mods(keymap, state_, entry, type)                                     \
        for (xkb_mod_index_t idx = 0; idx < xkb_keymap_num_mods(keymap); idx++) {   \
            xkb_mod_mask_t mask = UINT32_C(1) << idx;                               \
            fprintf(stderr, "#%u State %#"PRIx32", mod: %s (%"PRIu32")\n",          \
                    k, (entry)->state, xkb_keymap_mod_get_name(keymap, idx), idx);  \
            {                                                                       \
                xkb_mod_mask_t expected = mod_mask_get_effective(keymap,            \
                                                                 (entry)->state);   \
                xkb_mod_mask_t got = xkb_state_serialize_mods(state, type);         \
                assert_printf(got == expected,                                      \
                              "xkb_state_serialize_mods, " STRINGIFY2(type)         \
                              ", expected %#"PRIx32", got %#"PRIx32"\n",            \
                              expected, got);                                       \
            }                                                                       \
            bool expected = !!(mask & (entry)->active);                             \
            bool got = !!xkb_state_mod_index_is_active(state_, idx, type);          \
            assert_printf(got == expected,                                          \
                          "xkb_state_mod_index_is_active, " STRINGIFY2(type) "\n"); \
            got = !!xkb_state_mod_index_is_active(state_, idx,                      \
                                                  XKB_STATE_MODS_EFFECTIVE);        \
            assert_printf(got == expected, "xkb_state_mod_index_is_active, "        \
                          STRINGIFY2(XKB_STATE_MODS_EFFECTIVE) "\n");               \
            got = !!xkb_state_mod_indices_are_active(                               \
                        state_, type,                                               \
                        XKB_STATE_MATCH_ALL | XKB_STATE_MATCH_NON_EXCLUSIVE,        \
                        idx, XKB_MOD_INVALID);                                      \
            assert_printf(got == expected, "xkb_state_mod_indices_are_active, "     \
                          STRINGIFY2(type) "\n");                                   \
            got = !!xkb_state_mod_indices_are_active(                               \
                        state_, XKB_STATE_MODS_EFFECTIVE,                           \
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

static inline xkb_mod_mask_t
canonical_mask(bool is_pure, xkb_mod_mask_t vmod, xkb_mod_mask_t real)
{
    return is_pure ? vmod : real;
}

static void
test_update_mask_mods(struct xkb_keymap *keymap, bool pure_vmods)
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
    xkb_mod_mask_t caps  = (UINT32_C(1) << capsIdx);
    xkb_mod_mask_t shift = (UINT32_C(1) << shiftIdx);
    xkb_mod_mask_t mod1  = (UINT32_C(1) << mod1Idx);
    xkb_mod_mask_t mod2  = (UINT32_C(1) << mod2Idx);
    xkb_mod_mask_t alt   = (UINT32_C(1) << altIdx);
    xkb_mod_mask_t meta  = (UINT32_C(1) << metaIdx);
    xkb_mod_mask_t num   = (UINT32_C(1) << numIdx);

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
           canonical_mask(pure_vmods, alt, mod1));

    changed = xkb_state_update_mask(state, meta, 0, 0, 0, 0, 0);
    assert(changed == (pure_vmods
                       ? (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE)
                       : 0 /* Same canonical modifier state */));
    assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE) ==
           canonical_mask(pure_vmods, meta, mod1));

    changed = xkb_state_update_mask(state, 0, 0, num, 0, 0, 0);
    assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LOCKED |
                       XKB_STATE_MODS_EFFECTIVE | XKB_STATE_LEDS));
    assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE) ==
           canonical_mask(pure_vmods, num, mod2));

    xkb_state_update_mask(state, 0, 0, 0, 0, 0, 0);

    changed = xkb_state_update_mask(state, mod2, 0, num, 0, 0, 0);
    assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LOCKED |
                       XKB_STATE_MODS_EFFECTIVE | XKB_STATE_LEDS));
    assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE) ==
           canonical_mask(pure_vmods, mod2 | num, mod2));
    assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_DEPRESSED) == mod2);
    assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_LOCKED) ==
           canonical_mask(pure_vmods, num, mod2));

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
test_consume(struct xkb_keymap *keymap, bool pure_vmods)
{
    xkb_mod_mask_t mask;
    xkb_mod_index_t shiftIdx  = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_SHIFT);
    xkb_mod_index_t capsIdx   = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_CAPS);
    xkb_mod_index_t ctrlIdx   = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_CTRL);
    xkb_mod_index_t mod1Idx   = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_MOD1);
    xkb_mod_index_t mod2Idx   = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_MOD2);
    xkb_mod_index_t mod5Idx   = _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_MOD5);
    xkb_mod_index_t altIdx    = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_ALT);
    xkb_mod_index_t metaIdx   = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_META);
    xkb_mod_index_t numIdx    = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_NUM);
    xkb_mod_index_t level3Idx = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_LEVEL3);
    xkb_mod_mask_t caps   = (UINT32_C(1) << capsIdx);
    xkb_mod_mask_t shift  = (UINT32_C(1) << shiftIdx);
    xkb_mod_mask_t ctrl   = (UINT32_C(1) << ctrlIdx);
    xkb_mod_mask_t mod1   = (UINT32_C(1) << mod1Idx);
    xkb_mod_mask_t mod2   = (UINT32_C(1) << mod2Idx);
    xkb_mod_mask_t mod5   = (UINT32_C(1) << mod5Idx);
    xkb_mod_mask_t alt    = (UINT32_C(1) << altIdx);
    xkb_mod_mask_t meta   = (UINT32_C(1) << metaIdx);
    xkb_mod_mask_t num    = (UINT32_C(1) << numIdx);
    xkb_mod_mask_t level3 = (UINT32_C(1) << level3Idx);
    struct xkb_state *state = xkb_state_new(keymap);
    assert(state);

    /* Test remove_consumed() */
    xkb_state_update_key(state, KEY_LEFTALT + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_EQUAL + EVDEV_OFFSET, XKB_KEY_DOWN);

    fprintf(stderr, "dumping state for Alt-Shift-+\n");
    print_state(state);

    mask = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
    assert(mask == (canonical_mask(pure_vmods, alt, mod1) | shift));
    mask = xkb_state_mod_mask_remove_consumed(state, KEY_EQUAL + EVDEV_OFFSET,
                                              mask);
    assert(mask == canonical_mask(pure_vmods, alt, mod1));

    /* Test get_consumed_mods() */
    mask = xkb_state_key_get_consumed_mods(state, KEY_EQUAL + EVDEV_OFFSET);
    assert(mask == shift);

    mask = xkb_state_key_get_consumed_mods(state, KEY_ESC + EVDEV_OFFSET);
    assert(mask == 0);

    assert(xkb_state_mod_index_is_consumed(state, KEY_EQUAL + EVDEV_OFFSET, shiftIdx) > 0);
    assert(xkb_state_mod_index_is_consumed(state, KEY_EQUAL + EVDEV_OFFSET, mod1Idx) == 0);
    assert(xkb_state_mod_index_is_consumed(state, KEY_EQUAL + EVDEV_OFFSET, altIdx) == 0);
    assert(xkb_state_mod_index_is_consumed(state, KEY_EQUAL + EVDEV_OFFSET, metaIdx) == 0);

    xkb_state_unref(state);

    /* Test is_consumed() - simple ALPHABETIC type. */
    state = xkb_state_new(keymap);
    assert(state);

    mask = xkb_state_key_get_consumed_mods(state, KEY_A + EVDEV_OFFSET);
    assert(mask == (shift | caps));

    assert(xkb_state_mod_index_is_consumed(state, KEY_A + EVDEV_OFFSET, capsIdx) > 0);
    assert(xkb_state_mod_index_is_consumed(state, KEY_A + EVDEV_OFFSET, shiftIdx) > 0);
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_UP);
    assert(xkb_state_mod_index_is_consumed(state, KEY_A + EVDEV_OFFSET, capsIdx) > 0);
    assert(xkb_state_mod_index_is_consumed(state, KEY_A + EVDEV_OFFSET, shiftIdx) > 0);
    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
    assert(xkb_state_mod_index_is_consumed(state, KEY_A + EVDEV_OFFSET, capsIdx) > 0);
    assert(xkb_state_mod_index_is_consumed(state, KEY_A + EVDEV_OFFSET, shiftIdx) > 0);
    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_UP);
    assert(xkb_state_mod_index_is_consumed(state, KEY_A + EVDEV_OFFSET, capsIdx) > 0);
    assert(xkb_state_mod_index_is_consumed(state, KEY_A + EVDEV_OFFSET, shiftIdx) > 0);

    xkb_state_unref(state);

    /* More complicated - CTRL+ALT */
    state = xkb_state_new(keymap);
    assert(state);

    mask = xkb_state_key_get_consumed_mods(state, KEY_F1 + EVDEV_OFFSET);
    assert(mask == (shift | canonical_mask(pure_vmods, alt, mod1) |
                    ctrl | canonical_mask(pure_vmods, level3, mod5)));

    /* Shift is preserved. */
    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
    mask = xkb_state_key_get_consumed_mods(state, KEY_F1 + EVDEV_OFFSET);
    assert(mask == (canonical_mask(pure_vmods, alt, mod1) | ctrl |
                    canonical_mask(pure_vmods, level3, mod5)));
    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);

    mask = xkb_state_key_get_consumed_mods(state, KEY_F1 + EVDEV_OFFSET);
    assert(mask == (shift | canonical_mask(pure_vmods, alt, mod1) |
                    ctrl | canonical_mask(pure_vmods, level3, mod5)));

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
    assert(mask == (canonical_mask(pure_vmods, alt, mod1) | ctrl));
    assert(xkb_state_mod_index_is_consumed(state, KEY_F1 + EVDEV_OFFSET, shiftIdx) > 0);
    assert(xkb_state_mod_index_is_consumed(state, KEY_F1 + EVDEV_OFFSET, ctrlIdx) > 0);
    assert(xkb_state_mod_index_is_consumed(state, KEY_F1 + EVDEV_OFFSET, altIdx) > 0);
    if (pure_vmods) {
        assert(xkb_state_mod_index_is_consumed(state, KEY_F1 + EVDEV_OFFSET, mod1Idx) == 0);
        assert(xkb_state_mod_index_is_consumed(state, KEY_F1 + EVDEV_OFFSET, metaIdx) == 0);
    } else {
        assert(xkb_state_mod_index_is_consumed(state, KEY_F1 + EVDEV_OFFSET, mod1Idx) > 0);
        assert(xkb_state_mod_index_is_consumed(state, KEY_F1 + EVDEV_OFFSET, metaIdx) > 0);
    }
    mask = ctrl | canonical_mask(pure_vmods, alt, mod1) |
           canonical_mask(pure_vmods, num, mod2);
    mask = xkb_state_mod_mask_remove_consumed(state, KEY_F1 + EVDEV_OFFSET, mask);
    assert(mask == canonical_mask(pure_vmods, num, mod2));
    mask = ctrl | alt | canonical_mask(pure_vmods, alt, meta) |
           canonical_mask(pure_vmods, num, mod2);
    mask = xkb_state_mod_mask_remove_consumed(state, KEY_F1 + EVDEV_OFFSET, mask);
    assert(mask == canonical_mask(pure_vmods, num, mod2));

    xkb_state_unref(state);

    /* Test XKB_CONSUMED_MODE_GTK, Simple Shift */
    state = xkb_state_new(keymap);
    assert(state);

    mask = xkb_state_key_get_consumed_mods2(state, KEY_A + EVDEV_OFFSET,
                                            XKB_CONSUMED_MODE_GTK);
    assert(mask == (shift | caps));

    xkb_state_update_key(state, KEY_LEFTALT + EVDEV_OFFSET, XKB_KEY_DOWN);
    mask = xkb_state_key_get_consumed_mods2(state, KEY_A + EVDEV_OFFSET,
                                            XKB_CONSUMED_MODE_GTK);
    assert(mask == (shift | caps));

    xkb_state_unref(state);
}

static void
test_overlapping_mods(struct xkb_context *context)
{
    struct xkb_keymap *keymap;
    struct xkb_state *state;

    /* Super and Hyper are overlapping (full overlap) */
    keymap = test_compile_rules(context, XKB_KEYMAP_FORMAT_TEXT_V1, "evdev",
                                NULL, "us", NULL,
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
    xkb_mod_mask_t shift = (UINT32_C(1) << shiftIdx);
    xkb_mod_mask_t ctrl  = (UINT32_C(1) << ctrlIdx);
    xkb_mod_mask_t mod1  = (UINT32_C(1) << mod1Idx);
    xkb_mod_mask_t mod3  = (UINT32_C(1) << mod3Idx);
    xkb_mod_mask_t mod4  = (UINT32_C(1) << mod4Idx);
    xkb_mod_mask_t mod5  = (UINT32_C(1) << mod5Idx);
    xkb_mod_mask_t alt   = (UINT32_C(1) << altIdx);
    xkb_mod_mask_t meta  = (UINT32_C(1) << metaIdx);
    xkb_mod_mask_t super = (UINT32_C(1) << superIdx);
    xkb_mod_mask_t hyper = (UINT32_C(1) << hyperIdx);
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

    for (unsigned int k = 0; k < ARRAY_SIZE(test_data1); k++) {
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
    keymap = test_compile_rules(context, XKB_KEYMAP_FORMAT_TEXT_V1, "evdev",
                                NULL, "us", NULL,
                                "overlapping_modifiers:meta,"
                                "grp:win_space_toggle");
    assert(keymap);
    altIdx   = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_ALT);
    metaIdx  = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_META);
    superIdx = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_SUPER);
    hyperIdx = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_HYPER);
    alt   = (UINT32_C(1) << altIdx);
    meta  = (UINT32_C(1) << metaIdx);
    super = (UINT32_C(1) << superIdx);
    hyper = (UINT32_C(1) << hyperIdx);
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

    for (unsigned int k = 0; k < ARRAY_SIZE(test_data2); k++) {
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
    keymap = test_compile_rules(context, XKB_KEYMAP_FORMAT_TEXT_V1, "evdev",
                                NULL, "us", NULL,
                                "overlapping_modifiers:super_hyper,"
                                "overlapping_modifiers:meta");
    assert(keymap);
    altIdx   = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_ALT);
    metaIdx  = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_META);
    superIdx = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_SUPER);
    hyperIdx = _xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_HYPER);
    alt   = (UINT32_C(1) << altIdx);
    meta  = (UINT32_C(1) << metaIdx);
    super = (UINT32_C(1) << superIdx);
    hyper = (UINT32_C(1) << hyperIdx);
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

    for (unsigned int k = 0; k < ARRAY_SIZE(test_data3); k++) {
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
test_inactive_key_type_entry(struct xkb_context *context)
{
    const char keymap_str[] =
        "xkb_keymap {\n"
        "    xkb_keycodes {\n"
        "        <a> = 38;\n"
        "        <leftshift> = 50;\n"
        "    };\n"
        "    xkb_types {\n"
        "        virtual_modifiers Bound = Shift, Unbound;\n"
        "        type \"X\" {\n"
        "            modifiers = Bound+Unbound;\n"
        /* This entry should be ignored because it has an unbound modifier */
        "            map[Bound+Unbound] = Level1;\n"
        "            map[Bound] = Level2;\n"
        "        };\n"
        "    };\n"
        "    xkb_symbols {\n"
        "        key <a>         { [ a, A ], type = \"X\" };\n"
        "        key <leftshift> { [ SetMods(mods = Shift) ] };\n"
        "    };\n"
        "};";
    struct xkb_keymap *keymap =
        test_compile_buffer(context, XKB_KEYMAP_FORMAT_TEXT_V1,
                            keymap_str, sizeof(keymap_str));
    assert(keymap);
    struct xkb_state *state = xkb_state_new(keymap);
    assert(state);
    const xkb_mod_mask_t shift = (UINT32_C(1) << XKB_MOD_INDEX_SHIFT);
    assert(xkb_state_key_get_one_sym(state, KEY_A + EVDEV_OFFSET) == XKB_KEY_a);
    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
    assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE) == shift);
    assert(xkb_state_key_get_one_sym(state, KEY_A + EVDEV_OFFSET) == XKB_KEY_A);
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
test_keycode_range(struct xkb_keymap *keymap)
{
    xkb_keycode_t counter;

    assert(xkb_keymap_min_keycode(keymap) == 9);
    assert(xkb_keymap_max_keycode(keymap) == 569);

    counter = xkb_keymap_min_keycode(keymap);
    xkb_keymap_key_for_each(keymap, key_iter, &counter);
    assert(counter == xkb_keymap_max_keycode(keymap) + 1);
}

static void
test_caps_keysym_transformation(struct xkb_context *context)
{
    int nsyms;
    xkb_keysym_t sym;
    const xkb_keysym_t *syms;

    const char keymap_str[] =
        "xkb_keymap {\n"
        " xkb_keycodes { include \"evdev\" };\n"
        " xkb_compat { include \"basic\" };\n"
        " xkb_types { include \"complete\" };\n"
        " xkb_symbols {\n"
        "  include \"pc+ch(fr)\"\n"
        "  key <AE13> { [{oe, ssharp}, {ae, s, s}] };"
        "  key <AB11> { [{3, ntilde}] };"
        "  replace key <RCTL> { [{Control_R, ISO_Next_Group}] };"
        " };"
        "};";

    struct xkb_keymap* const keymap =
        test_compile_buffer(context, XKB_KEYMAP_FORMAT_TEXT_V1,
                            keymap_str, sizeof(keymap_str));
    assert(keymap);

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
    assert(xkb_state_key_get_level(state, KEY_YEN + EVDEV_OFFSET, 0) == 0);
    sym = xkb_state_key_get_one_sym(state, KEY_YEN + EVDEV_OFFSET);
    assert(sym == XKB_KEY_NoSymbol);
    nsyms = xkb_state_key_get_syms(state, KEY_YEN + EVDEV_OFFSET, &syms);
    assert(nsyms == 2 && syms[0] == XKB_KEY_oe && syms[1] == XKB_KEY_ssharp);
    assert(xkb_state_key_get_level(state, KEY_RO + EVDEV_OFFSET, 0) == 0);
    sym = xkb_state_key_get_one_sym(state, KEY_RO + EVDEV_OFFSET);
    assert(sym == XKB_KEY_NoSymbol);
    nsyms = xkb_state_key_get_syms(state, KEY_RO + EVDEV_OFFSET, &syms);
    assert(nsyms == 2 && syms[0] == XKB_KEY_3 && syms[1] == XKB_KEY_ntilde);
    assert(xkb_state_key_get_level(state, KEY_RIGHTCTRL + EVDEV_OFFSET, 0) == 0);
    sym = xkb_state_key_get_one_sym(state, KEY_RIGHTCTRL + EVDEV_OFFSET);
    assert(sym == XKB_KEY_NoSymbol);
    nsyms = xkb_state_key_get_syms(state, KEY_RIGHTCTRL + EVDEV_OFFSET, &syms);
    assert(nsyms == 2 && syms[0] == XKB_KEY_Control_R && syms[1] == XKB_KEY_ISO_Next_Group);

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
    assert(xkb_state_key_get_level(state, KEY_YEN + EVDEV_OFFSET, 0) == 1);
    sym = xkb_state_key_get_one_sym(state, KEY_YEN + EVDEV_OFFSET);
    assert(sym == XKB_KEY_NoSymbol);
    nsyms = xkb_state_key_get_syms(state, KEY_YEN + EVDEV_OFFSET, &syms);
    assert(nsyms == 3 && syms[0] == XKB_KEY_ae && syms[1] == XKB_KEY_s && syms[2] == XKB_KEY_s);
    assert(xkb_state_key_get_level(state, KEY_RO + EVDEV_OFFSET, 0) == 0);
    sym = xkb_state_key_get_one_sym(state, KEY_RO + EVDEV_OFFSET);
    assert(sym == XKB_KEY_NoSymbol);
    nsyms = xkb_state_key_get_syms(state, KEY_RO + EVDEV_OFFSET, &syms);
    assert(nsyms == 2 && syms[0] == XKB_KEY_3 && syms[1] == XKB_KEY_ntilde);
    assert(xkb_state_key_get_level(state, KEY_RIGHTCTRL + EVDEV_OFFSET, 0) == 0);
    sym = xkb_state_key_get_one_sym(state, KEY_RIGHTCTRL + EVDEV_OFFSET);
    assert(sym == XKB_KEY_NoSymbol);
    nsyms = xkb_state_key_get_syms(state, KEY_RIGHTCTRL + EVDEV_OFFSET, &syms);
    assert(nsyms == 2 && syms[0] == XKB_KEY_Control_R && syms[1] == XKB_KEY_ISO_Next_Group);
    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);
    assert(xkb_state_mod_index_is_active(state, shift, XKB_STATE_MODS_EFFECTIVE) == 0);

    /* With caps, transform in same level. */
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
    assert(nsyms == 1 && syms[0] == XKB_KEY_Eacute);
    assert(xkb_state_key_get_level(state, KEY_YEN + EVDEV_OFFSET, 0) == 0);
    sym = xkb_state_key_get_one_sym(state, KEY_YEN + EVDEV_OFFSET);
    assert(sym == XKB_KEY_NoSymbol);
    nsyms = xkb_state_key_get_syms(state, KEY_YEN + EVDEV_OFFSET, &syms);
    assert(nsyms == 2 && syms[0] == XKB_KEY_OE && syms[1] == XKB_KEY_Ssharp);
    assert(xkb_state_key_get_level(state, KEY_RO + EVDEV_OFFSET, 0) == 0);
    sym = xkb_state_key_get_one_sym(state, KEY_RO + EVDEV_OFFSET);
    assert(sym == XKB_KEY_NoSymbol);
    nsyms = xkb_state_key_get_syms(state, KEY_RO + EVDEV_OFFSET, &syms);
    assert(nsyms == 2 && syms[0] == XKB_KEY_3 && syms[1] == XKB_KEY_Ntilde);
    assert(xkb_state_key_get_level(state, KEY_RIGHTCTRL + EVDEV_OFFSET, 0) == 0);
    sym = xkb_state_key_get_one_sym(state, KEY_RIGHTCTRL + EVDEV_OFFSET);
    assert(sym == XKB_KEY_NoSymbol);
    nsyms = xkb_state_key_get_syms(state, KEY_RIGHTCTRL + EVDEV_OFFSET, &syms);
    assert(nsyms == 2 && syms[0] == XKB_KEY_Control_R && syms[1] == XKB_KEY_ISO_Next_Group);
    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);
    assert(xkb_state_mod_index_is_active(state, shift, XKB_STATE_MODS_EFFECTIVE) == 0);
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_UP);

    xkb_state_unref(state);
    xkb_keymap_unref(keymap);
}

static void
test_get_utf8_utf32(struct xkb_keymap *keymap)
{
    char buf[256];
    struct xkb_state *state = xkb_state_new(keymap);
    assert(state);

#define TEST_KEY(key, expected_utf8, expected_utf32) do { \
    assert(xkb_state_key_get_utf8(state, (key) + EVDEV_OFFSET, NULL, 0) == (int) strlen(expected_utf8)); \
    assert(xkb_state_key_get_utf8(state, (key) + EVDEV_OFFSET, buf, sizeof(buf)) == (int) strlen(expected_utf8)); \
    assert(memcmp(buf, expected_utf8, sizeof(expected_utf8)) == 0); \
    assert(xkb_state_key_get_utf32(state, (key) + EVDEV_OFFSET) == (expected_utf32)); \
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
    assert(xkb_state_key_get_utf8(state, KEY_6 + EVDEV_OFFSET, buf, 0) == (int) strlen("HELLO"));
    assert(memcmp(buf, "X", 1) == 0);
    assert(xkb_state_key_get_utf8(state, KEY_6 + EVDEV_OFFSET, buf, 1) == (int) strlen("HELLO"));
    assert(memcmp(buf, "", 1) == 0);
    assert(xkb_state_key_get_utf8(state, KEY_6 + EVDEV_OFFSET, buf, 2) == (int) strlen("HELLO"));
    assert(memcmp(buf, "H", 2) == 0);
    assert(xkb_state_key_get_utf8(state, KEY_6 + EVDEV_OFFSET, buf, 3) == (int) strlen("HELLO"));
    assert(memcmp(buf, "HE", 3) == 0);
    assert(xkb_state_key_get_utf8(state, KEY_6 + EVDEV_OFFSET, buf, 5) == (int) strlen("HELLO"));
    assert(memcmp(buf, "HELL", 5) == 0);
    assert(xkb_state_key_get_utf8(state, KEY_6 + EVDEV_OFFSET, buf, 6) == (int) strlen("HELLO"));
    assert(memcmp(buf, "HELLO", 6) == 0);
    assert(xkb_state_key_get_utf8(state, KEY_6 + EVDEV_OFFSET, buf, 7) == (int) strlen("HELLO"));
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

static bool
test_active_leds(struct xkb_state *state, xkb_led_mask_t leds_expected)
{
    struct xkb_keymap *keymap = xkb_state_get_keymap(state);
    bool ret = true;
    xkb_led_mask_t leds_got = 0;
    for (xkb_led_index_t led = 0; led < xkb_keymap_num_leds(keymap); led++) {
        const int status = xkb_state_led_index_is_active(state, led);
        if (status < 0)
            continue;
        const xkb_led_mask_t mask = (UINT32_C(1) << led);
        const bool expected = !!(leds_expected & mask);
        if (status)
            leds_got |= mask;
        if (!!status ^ expected) {
            fprintf(stderr, "ERROR: LED \"%s\" status: expected %d, got %d\n",
                    xkb_keymap_led_get_name(keymap, led), expected, !!status);
            ret = false;
        }
    }
    if (!ret) {
        fprintf(stderr, "ERROR: LEDs: expected 0x%"PRIx32", got 0x%"PRIx32"\n",
                leds_expected, leds_got);
    }
    return ret;
}

static void
test_leds(struct xkb_context *ctx)
{
    const char buf[] =
        "xkb_keymap {\n"
        "    xkb_keycodes { include \"evdev\" };\n"
        "    xkb_types { include \"basic\" };\n"
        "    xkb_compat {\n"
        "        include \"leds(groups)\"\n"
        "        interpret ISO_Group_Shift { action= SetGroup(group=+1); };\n"
        "        interpret ISO_Group_Latch { action= LatchGroup(group=+1); };\n"
        "        interpret ISO_Group_Lock  { action= LockGroup(group=+1); };\n"
        "    };\n"
        "    xkb_symbols {\n"
        "        key <AD01> { [ q, Q ], [w, W], [e, E] };\n"
        "        key <LFSH> { [ ISO_Group_Shift ] };\n"
        "        key <MENU> { [ ISO_Group_Latch ] };\n"
        "        key <CAPS> { [ ISO_Group_Lock ] };\n"
        "    };\n"
        "};";

    struct xkb_keymap *keymap =
        test_compile_buffer(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                            buf, ARRAY_SIZE(buf));
    assert(keymap);

    const xkb_led_index_t caps_idx = _xkb_keymap_led_get_index(keymap, XKB_LED_NAME_CAPS);
    const xkb_led_index_t num_idx = _xkb_keymap_led_get_index(keymap, XKB_LED_NAME_NUM);
    const xkb_led_index_t scroll_idx = _xkb_keymap_led_get_index(keymap, XKB_LED_NAME_SCROLL);
    const xkb_led_index_t compose_idx = _xkb_keymap_led_get_index(keymap, XKB_LED_NAME_COMPOSE);
    const xkb_led_index_t sleep_idx = _xkb_keymap_led_get_index(keymap, "Sleep");
    const xkb_led_index_t mute_idx = _xkb_keymap_led_get_index(keymap, "Mute");
    const xkb_led_index_t misc_idx = _xkb_keymap_led_get_index(keymap, "Misc");
    const xkb_led_index_t mail_idx = _xkb_keymap_led_get_index(keymap, "Mail");
    const xkb_led_index_t charging_idx = _xkb_keymap_led_get_index(keymap, "Charging");

    const xkb_led_mask_t caps = UINT32_C(1) << caps_idx;
    const xkb_led_mask_t num = UINT32_C(1) << num_idx;
    const xkb_led_mask_t scroll = UINT32_C(1) << scroll_idx;
    const xkb_led_mask_t compose = UINT32_C(1) << compose_idx;
    const xkb_led_mask_t sleep = UINT32_C(1) << sleep_idx;
    const xkb_led_mask_t mute = UINT32_C(1) << mute_idx;
    const xkb_led_mask_t misc = UINT32_C(1) << misc_idx;
    const xkb_led_mask_t mail = UINT32_C(1) << mail_idx;
    const xkb_led_mask_t charging = UINT32_C(1) << charging_idx;

    struct xkb_state *state = xkb_state_new(keymap);
    assert(state);

    xkb_state_update_key(state, KEY_Q + EVDEV_OFFSET, XKB_KEY_UP);
    assert(test_active_leds(state, (caps | scroll)));

    /* SetGroup */
    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
    assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE) == 0x1);
    assert(test_active_leds(state, (num | scroll | mute | misc)));
    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);

    /* LatchGroup */
    xkb_state_update_key(state, KEY_COMPOSE + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_COMPOSE + EVDEV_OFFSET, XKB_KEY_UP);
    assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE) == 0x1);
    assert(test_active_leds(state, (caps | compose | mute | misc | charging)));
    xkb_state_update_key(state, KEY_Q + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_Q + EVDEV_OFFSET, XKB_KEY_UP);

    /* LockGroup 2 */
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_UP);
    assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE) == 0x1);
    assert(test_active_leds(state, (caps | scroll | sleep | mute | mail)));

    /* LockGroup 2 + SetGroup */
    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
    assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE) == 0x2);
    assert(test_active_leds(state, (num | scroll | sleep | mute | misc | mail | charging)));
    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);

    /* LockGroup 3 */
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_UP);
    assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE) == 0x2);
    assert(test_active_leds(state, (caps | scroll | sleep | mute | charging)));

    /* LockGroup 3 + SetGroup */
    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
    assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE) == 0x0);
    assert(test_active_leds(state, (num | scroll | sleep | misc | charging)));
    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);

    xkb_state_unref(state);
    xkb_keymap_unref(keymap);
}

static void
test_multiple_actions(struct xkb_context *ctx)
{
    /* Check that we can trigger 2 actions on the same levels, with both
     * explicit (defined via the key statement) and explicit (defined via
     * interpret). The actions set the Control modifier and may change the
     * group. The idea is to enable keyboard shortcuts to always target the
     * same layout. Because SetGroup() does not work well with absolute values,
     * we define the modifiers on each 2 groups. */
    const char keymap_str[] =
        "xkb_keymap {\n"
        "  xkb_keycodes {\n"
        "    <AD01> = 24;\n"
	    "    <LCTL> = 37;\n"
	    "    <RCTL> = 105;\n"
        "  };\n"
        "  xkb_compat {\n"
        /* Right Control has its actions set implicitly via interpret */
        "    interpret 1 {\n"
        "      action = {SetMods(modifiers=Control)};\n"
        "    };\n"
        "    interpret 2 {\n"
        "      action = {SetMods(modifiers=Control), SetGroup(group=-1)};\n"
        "    };\n"
        "    interpret 3 {\n"
        "      action = {SetMods(modifiers=Control), SetGroup(group=-2)};\n"
        "    };\n"
        "    interpret 4 {\n"
        "      action = {SetMods(modifiers=Control), SetGroup(group=-3)};\n"
        "    };\n"
        "  };\n"
        "  xkb_symbols {\n"
        "    key <AD01> { [q], [Arabic_dad], [c_h], [Thai_maiyamok] };\n"
        /* Left Control has its actions set explicitly */
        "    key <LCTL> {\n"
        "      symbols[1] = [Control_L],\n"
        "      actions[1] = [{SetMods(modifiers=Control)}],\n"
        "      actions[2] = [{SetMods(modifiers=Control), SetGroup(group=-1)}],\n"
        "      actions[3] = [{SetMods(modifiers=Control), SetGroup(group=-2)}],\n"
        "      actions[4] = [{SetMods(modifiers=Control), SetGroup(group=-3)}]\n"
        "    };\n"
        "    key <RCTL> { [1], [2], [3], [4] };\n"
        "  };\n"
        "};";
    struct xkb_keymap *keymap =
        test_compile_buffer(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                            keymap_str, sizeof(keymap_str));
    assert(keymap);
    struct xkb_state *state = xkb_state_new(keymap);
    assert(state);
    const xkb_mod_index_t ctrl_idx =
        _xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_CTRL);
    const xkb_mod_mask_t ctrl = UINT32_C(1) << ctrl_idx;
    const xkb_keycode_t lcontrol = KEY_LEFTCTRL + EVDEV_OFFSET;
    const xkb_keycode_t rcontrol = KEY_RIGHTCTRL + EVDEV_OFFSET;
    const xkb_keycode_t q = KEY_Q + EVDEV_OFFSET;
    xkb_mod_mask_t mods;

    const xkb_keycode_t mod_keys[] = {lcontrol, rcontrol};
    const xkb_keysym_t ad01[] = {
        XKB_KEY_q,
        XKB_KEY_Arabic_dad,
        XKB_KEY_c_h,
        XKB_KEY_Thai_maiyamok
    };

    for (xkb_layout_index_t layout = 0; layout < ARRAY_SIZE(ad01); layout++) {
        /* Lock layout */
        xkb_state_update_mask(state, 0, 0, 0, 0, 0, layout);
        assert(xkb_state_key_get_layout(state, q) == layout);
        assert(xkb_state_key_get_one_sym(state, q) == ad01[layout]);
        for (unsigned int k = 0; k < ARRAY_SIZE(mod_keys); k++) {
            /* Temporarily switch to first layout + set Control modifier */
            xkb_state_update_key(state, mod_keys[k], XKB_KEY_DOWN);
            assert(xkb_state_key_get_layout(state, q) == 0);
            mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
            assert(mods == ctrl);
            assert(xkb_state_key_get_one_sym(state, q) == XKB_KEY_q);
            /* Restore layout, unset Control */
            xkb_state_update_key(state, mod_keys[k], XKB_KEY_UP);
            assert(xkb_state_key_get_layout(state, q) == layout);
            mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
            assert(mods == 0);
            assert(xkb_state_key_get_one_sym(state, q) == ad01[layout]);
        }
    }

    xkb_state_unref(state);
    xkb_keymap_unref(keymap);
}

static void
test_void_action(struct xkb_context *ctx)
{
    const char keymap_str[] =
        "xkb_keymap {\n"
        "  xkb_keycodes { <> = 1; };\n"
        "  xkb_symbols  { key <> { [VoidAction()], [VoidAction()] }; };\n"
        "};";
    struct xkb_keymap *keymap =
        test_compile_buffer(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                            keymap_str, sizeof(keymap_str));
    assert(keymap);
    struct xkb_state *state = xkb_state_new(keymap);
    assert(state);

    xkb_state_update_latched_locked(state, 0x1, 0x1, true, 1, 0, 0, false, 0);
    assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE) == 1);
    assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE) == 0x1);
    xkb_state_update_key(state, 1, XKB_KEY_DOWN);
    assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE) == 0);
    assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE) == 0);

    xkb_state_unref(state);
    xkb_keymap_unref(keymap);
}

static void
test_extended_layout_indices(struct xkb_context *ctx)
{
    const char keymap_str[] =
        "xkb_keymap \"extended-layout-indices\" {\n"
        "  xkb_keycodes {\n"
        "    <> = 1;\n"
        "    <set-1>   = 10;\n"
        "    <set+1>   = 11;\n"
        "    <set32>   = 12;\n"
        "    <latch-1> = 20;\n"
        "    <latch+1> = 21;\n"
        "    <latch32> = 22;\n"
        "    <lock-1>  = 30;\n"
        "    <lock+1>  = 31;\n"
        "    <lock32>  = 32;\n"
        "  };\n"
        "  xkb_types { type \"ONE_LEVEL\" {}; };\n"
        "  xkb_symbols {\n"
        "    key <> {\n"
        "      [a], [b], [c], [d], [e], [f], [g], [h], [i], [j],\n"
        "      [k], [l], [m], [n], [o], [p], [q], [r], [s], [t],\n"
        "      [u], [v], [w], [x], [y], [z], [1], [2], [3], [4],\n"
        "      [5], [6]\n"
        "    };\n"
        "    key <set-1>   { [ISO_Group_Shift], [SetGroup(group=-1)] };\n"
        "    key <set+1>   { [ISO_Group_Shift], [SetGroup(group=+1)] };\n"
        "    key <set32>   { [ISO_Group_Shift], [SetGroup(group=32)] };\n"
        "    key <latch-1> { [ISO_Group_Latch], [LatchGroup(group=-1)] };\n"
        "    key <latch+1> { [ISO_Group_Latch], [LatchGroup(group=+1)] };\n"
        "    key <latch32> { [ISO_Group_Latch], [LatchGroup(group=32)] };\n"
        "    key <lock-1>  { [ISO_Group_Lock],  [LockGroup(group=-1)] };\n"
        "    key <lock+1>  { [ISO_Group_Lock],  [LockGroup(group=+1)] };\n"
        "    key <lock32>  { [ISO_Group_Lock],  [LockGroup(group=32)] };\n"
        "  };\n"
        "};";
    struct xkb_keymap *keymap = test_compile_buffer(
        ctx, XKB_KEYMAP_FORMAT_TEXT_V2, keymap_str, sizeof(keymap_str)
    );
    assert(keymap);
    const int32_t num_layouts = (int32_t) xkb_keymap_num_layouts(keymap);
    assert(num_layouts == XKB_MAX_GROUPS);
    struct xkb_state *state = xkb_state_new(keymap);
    assert(state);

    for (int32_t l = -num_layouts - 1; l < num_layouts + 1; l++) {
        const xkb_layout_index_t expected_layout = (xkb_layout_index_t)
                                                   group_wrap(l, num_layouts);
        /* Out-of-bounds latches update */
        enum xkb_state_component expected_changes =
            (XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE) |
            ((l == 0) ? 0 : XKB_STATE_LAYOUT_LATCHED) |
            ((expected_layout == 0) ? 0 : XKB_STATE_LAYOUT_EFFECTIVE);
        enum xkb_state_component got_changes = xkb_state_update_latched_locked(
            state, 0x1, 0x1, true, l, 0, 0, false, 0
        );
        assert(got_changes == expected_changes);
        assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LATCHED) ==
               (xkb_layout_index_t) l);
        assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE) ==
               expected_layout);
        assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_LATCHED) == 0x1);
        assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE) == 0x1);

        /* Break latches by key press */
        got_changes = xkb_state_update_key(state, 1, XKB_KEY_DOWN);
        assert(got_changes == expected_changes);
        assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LATCHED) == 0);
        assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE) == 0);
        assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_LATCHED) == 0);
        assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE) == 0);

        /* Check key release changes nothing */
        assert(xkb_state_update_key(state, 1, XKB_KEY_UP) == 0);
        assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LATCHED) == 0);
        assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE) == 0);
        assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_LATCHED) == 0);
        assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE) == 0);

        /* Out-of-bounds locks update */
        got_changes = xkb_state_update_latched_locked(
            state, 0, 0, false, 0, 0x2, 0x2, true, l
        );
        expected_changes =
            (XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE) |
            ((expected_layout == 0)
                ? 0
                : (XKB_STATE_LAYOUT_LOCKED | XKB_STATE_LAYOUT_EFFECTIVE));
        assert(got_changes == expected_changes);
        assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LOCKED) ==
               expected_layout);
        assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE) ==
               expected_layout);
        assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_LOCKED) == 0x2);
        assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE) == 0x2);

        /* Out-of-bounds locks reset */
        assert(got_changes == xkb_state_update_latched_locked(
            state, 0, 0, false, 0, 0x2, 0, true, 0
        ));
        assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LOCKED) == 0);
        assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE) == 0);
        assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_LOCKED) == 0);
        assert(xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE) == 0);
    }

    const xkb_layout_index_t last_layout = (xkb_layout_index_t) num_layouts - 1;

    const struct test_state_components tests[] = {
        /*
         * Set
         */
        {
            KEY_ENTRY(10 - EVDEV_OFFSET, DOWN, XKB_KEY_ISO_Group_Shift),
            .base_group=-1, .group=last_layout,
            .changes = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE
        },
        {
            KEY_ENTRY(1 - EVDEV_OFFSET, BOTH, XKB_KEY_6),
            .base_group=-1, .group=last_layout,
            .changes = 0
        },
        {
            KEY_ENTRY(10 - EVDEV_OFFSET, UP, XKB_KEY_ISO_Group_Shift),
            .base_group=0, .group=0,
            .changes = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE
        },
        {
            KEY_ENTRY(12 - EVDEV_OFFSET, DOWN, XKB_KEY_ISO_Group_Shift),
            .base_group=num_layouts - 1, .group=last_layout,
            .changes = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE
        },
        {
            KEY_ENTRY(11 - EVDEV_OFFSET, DOWN, XKB_KEY_ISO_Group_Shift),
            .base_group=num_layouts, .group=0,
            .changes = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE
        },
        {
            KEY_ENTRY(11 - EVDEV_OFFSET, UP, XKB_KEY_ISO_Group_Shift),
            .base_group=num_layouts - 1, .group=last_layout,
            .changes = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE
        },
        {
            KEY_ENTRY(12 - EVDEV_OFFSET, UP, XKB_KEY_ISO_Group_Shift),
            .base_group=0, .group=0,
            .changes = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE
        },

        /*
         * Lock
         */
        {
            KEY_ENTRY(30 - EVDEV_OFFSET, BOTH, XKB_KEY_ISO_Group_Lock),
            .locked_group=num_layouts - 1, .group=last_layout,
            .changes = 0
        },
        {
            KEY_ENTRY(31 - EVDEV_OFFSET, BOTH, XKB_KEY_ISO_Group_Lock),
            .locked_group=0, .group=0,
            .changes = 0
        },
        {
            KEY_ENTRY(32 - EVDEV_OFFSET, BOTH, XKB_KEY_ISO_Group_Lock),
            .locked_group=num_layouts-1, .group=last_layout,
            .changes = 0
        },

        /*
         * Latch
         */
        {
            KEY_ENTRY(21 - EVDEV_OFFSET, BOTH, XKB_KEY_ISO_Group_Latch),
            .latched_group=1, .locked_group=num_layouts-1, .group=0,
            .changes = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_LATCHED
        },
        {
            KEY_ENTRY(1 - EVDEV_OFFSET, BOTH, XKB_KEY_a),
            .latched_group=0, .locked_group=num_layouts-1, .group=0,
            .changes = 0
        },
        {
            KEY_ENTRY(31 - EVDEV_OFFSET, BOTH, XKB_KEY_ISO_Group_Lock),
            .locked_group=0, .group=0,
            .changes = 0
        },
        {
            KEY_ENTRY(20 - EVDEV_OFFSET, BOTH, XKB_KEY_ISO_Group_Latch),
            .latched_group=-1, .group=last_layout,
            .changes = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_LATCHED
        },
        {
            KEY_ENTRY(1 - EVDEV_OFFSET, BOTH, XKB_KEY_6),
            .latched_group=0, .group=0,
            .changes = 0
        },
        {
            KEY_ENTRY(22 - EVDEV_OFFSET, BOTH, XKB_KEY_ISO_Group_Latch),
            .latched_group=num_layouts-1, .group=last_layout,
            .changes = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_LATCHED
        },
        {
            KEY_ENTRY(1 - EVDEV_OFFSET, BOTH, XKB_KEY_6),
            .latched_group=0, .group=0,
            .changes = 0
        },
    };
    struct xkb_state *expected_state = xkb_state_new(keymap);
    assert(expected_state);
    for (size_t k = 0; k < ARRAY_SIZE(tests); k++) {
        assert_printf(
            run_state_update(keymap, &tests[k], &expected_state, &state),
            "%s #%zu: type: %d\n",
            __func__, k, tests[k].input_type
        );
    }

    xkb_state_unref(state);
    xkb_state_unref(expected_state);
    xkb_keymap_unref(keymap);
}

static enum xkb_state_component
update_key(struct xkb_state_machine *sm,
           struct xkb_event_iterator *events,
           struct xkb_state *state,
           bool use_machine, xkb_keycode_t key,
           enum xkb_key_direction direction)
{
    if (use_machine) {
        assert(xkb_state_machine_update_key(sm, events, key, direction) == 0);
        const struct xkb_event *event;
        enum xkb_state_component changed = 0;
        while ((event = xkb_event_iterator_next(events))) {
            changed |= xkb_state_update_from_event(state, event);
        }
        return changed;
    } else {
        return xkb_state_update_key(state, key, direction);
    }
}

static enum xkb_state_component
update_controls(struct xkb_state_machine *sm,
                struct xkb_event_iterator *events,
                struct xkb_state *state,
                bool use_machine,
                enum xkb_keyboard_controls affect,
                enum xkb_keyboard_controls controls)
{
    if (use_machine) {
        assert(xkb_state_machine_update_controls(sm, events, affect, controls)
               == 0);
        const struct xkb_event *event;
        enum xkb_state_component changed = 0;
        while ((event = xkb_event_iterator_next(events))) {
            changed |= xkb_state_update_from_event(state, event);
        }
        return changed;
    } else {
        return xkb_state_update_controls(state, affect, controls);
    }
}

static void
test_sticky_keys(struct xkb_context *ctx)
{
    struct xkb_keymap * const keymap = test_compile_rmlvo(
        ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
        "evdev", "pc104", "ca,cz,de", NULL, "controls,grp:lwin_switch,grp:menu_toggle"
    );
    assert(keymap);

    struct xkb_state_machine *sm = xkb_state_machine_new(keymap, NULL);
    assert(sm);
    struct xkb_event_iterator *events =
        xkb_event_iterator_new(ctx, XKB_EVENT_ITERATOR_NO_FLAGS);
    assert(events);
    struct xkb_state *state = xkb_state_new(keymap);
    assert(state);

    const xkb_mod_mask_t shift = xkb_keymap_mod_get_mask2(keymap,
                                                          XKB_MOD_INDEX_SHIFT);
    const xkb_mod_mask_t caps = xkb_keymap_mod_get_mask2(keymap,
                                                          XKB_MOD_INDEX_CAPS);
    const xkb_mod_mask_t ctrl = xkb_keymap_mod_get_mask2(keymap,
                                                         XKB_MOD_INDEX_CTRL);

    xkb_mod_mask_t mods = 0;
    enum xkb_keyboard_controls controls;
    enum xkb_state_component changed;

    controls = xkb_state_serialize_controls(state, XKB_STATE_CONTROLS);
    assert(controls == 0);

    enum sticky_key_activation {
        STICKY_KEY_ACTION_SETCONTROLS = 0,
        STICKY_KEY_ACTION_LOCKCONTROLS,
        STICKY_KEY_EVENTS_API,
        STICKY_KEY_LEGACY_API,
    };

    enum sticky_key_activation tests[] = {
        STICKY_KEY_ACTION_SETCONTROLS,
        STICKY_KEY_ACTION_LOCKCONTROLS,
        STICKY_KEY_EVENTS_API,
        STICKY_KEY_LEGACY_API,
    };

    for (uint8_t t = 0; t < (uint8_t) ARRAY_SIZE(tests); t++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, t);

        const bool use_events = tests[t] == STICKY_KEY_EVENTS_API;

        /* Enable controls */
        switch (tests[t]) {
        case STICKY_KEY_ACTION_SETCONTROLS:
            /* SetControls() */
            changed = update_key(sm, events, state, use_events,
                                 KEY_F1 + EVDEV_OFFSET, XKB_KEY_DOWN);
            assert(changed == XKB_STATE_CONTROLS);
            break;
        case STICKY_KEY_ACTION_LOCKCONTROLS:
            /* LockControls() */
            changed = update_key(sm, events, state, use_events,
                                 KEY_F2 + EVDEV_OFFSET, XKB_KEY_DOWN);
            assert(changed == XKB_STATE_CONTROLS);
            controls = xkb_state_serialize_controls(state, XKB_STATE_CONTROLS);
            assert(controls == XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS);
            changed = update_key(sm, events, state, use_events,
                                 KEY_F2 + EVDEV_OFFSET, XKB_KEY_UP);
            assert(changed == 0);
            break;
        case STICKY_KEY_EVENTS_API: {
            changed = update_controls(sm, events, state, true,
                                      XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS,
                                      XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS);
            assert(changed == XKB_STATE_CONTROLS);
            controls = xkb_state_serialize_controls(state, XKB_STATE_CONTROLS);
            assert(controls == XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS);
            break;
        }
        case STICKY_KEY_LEGACY_API:
            changed = xkb_state_update_controls(state,
                                                XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS,
                                                XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS);
            assert(changed == XKB_STATE_CONTROLS);
            controls = xkb_state_serialize_controls(state, XKB_STATE_CONTROLS);
            assert(controls == XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS);
            break;
        }
        controls = xkb_state_serialize_controls(state, XKB_STATE_CONTROLS);
        assert(controls == XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS);

        /* Latch shift (sticky) */
        changed = update_key(sm, events, state, use_events,
                             KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
        assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE));
        changed = update_key(sm, events, state, use_events,
                             KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);
        assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED));
        mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
        assert(mods == shift);

        /* No latch-to-lock */
        changed = update_key(sm, events, state, use_events,
                             KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
        assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED));
        changed = update_key(sm, events, state, use_events,
                             KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);
        assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE));
        mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
        assert(mods == 0);

        /* Latch shift (sticky) and control */
        changed = update_key(sm, events, state, use_events,
                             KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
        assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE));
        changed = update_key(sm, events, state, use_events,
                             KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);
        assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED));
        mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
        assert(mods == shift);
        changed = update_key(sm, events, state, use_events,
                             KEY_LEFTCTRL + EVDEV_OFFSET, XKB_KEY_DOWN);
        assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE));
        changed = update_key(sm, events, state, use_events,
                             KEY_LEFTCTRL + EVDEV_OFFSET, XKB_KEY_UP);
        assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED));
        mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
        assert(mods == (shift | ctrl));
        changed = update_key(sm, events, state, use_events,
                             KEY_Q + EVDEV_OFFSET, XKB_KEY_DOWN);
        assert(changed == (XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE));
        changed = update_key(sm, events, state, use_events,
                             KEY_Q + EVDEV_OFFSET, XKB_KEY_UP);
        assert(changed == 0);

        /* Latch (sticky) & lock groups */
        changed = update_key(sm, events, state, use_events,
                             KEY_COMPOSE + EVDEV_OFFSET, XKB_KEY_DOWN);
        assert(changed == (XKB_STATE_LAYOUT_LOCKED | XKB_STATE_LAYOUT_EFFECTIVE |
                           XKB_STATE_LEDS));
        changed = update_key(sm, events, state, use_events,
                             KEY_COMPOSE + EVDEV_OFFSET, XKB_KEY_UP);
        assert(changed == 0);
        assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LOCKED) == 1);
        assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE) == 1);
        changed = update_key(sm, events, state, use_events,
                             KEY_LEFTMETA + EVDEV_OFFSET, XKB_KEY_DOWN);
        assert(changed == (XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE));
        changed = update_key(sm, events, state, use_events,
                             KEY_LEFTMETA + EVDEV_OFFSET, XKB_KEY_UP);
        assert(changed == (XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_LATCHED));
        assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LATCHED) == 1);
        assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LOCKED) == 1);
        assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE) == 2);

        /* Latch shift (sticky) and lock Caps */
        changed = update_key(sm, events, state, use_events,
                             KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
        assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE));
        changed = update_key(sm, events, state, use_events,
                             KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);
        assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED));
        mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
        assert(mods == shift);
        changed = update_key(sm, events, state, use_events,
                             KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_DOWN);
        assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LOCKED |
                           XKB_STATE_MODS_EFFECTIVE | XKB_STATE_LEDS));
        changed = update_key(sm, events, state, use_events,
                             KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_UP);
        assert(changed == (XKB_STATE_MODS_DEPRESSED));
        mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
        assert(mods == (shift | caps));

        /* Disable controls */
        switch (tests[t]) {
        case STICKY_KEY_ACTION_SETCONTROLS:
            /* SetControls() */
            changed = update_key(sm, events, state, use_events,
                                 KEY_F1 + EVDEV_OFFSET, XKB_KEY_UP);
            assert(changed == (XKB_STATE_CONTROLS |
                               XKB_STATE_LAYOUT_LATCHED |
                               XKB_STATE_LAYOUT_LOCKED |
                               XKB_STATE_LAYOUT_EFFECTIVE |
                               XKB_STATE_MODS_LATCHED |
                               XKB_STATE_MODS_LOCKED |
                               XKB_STATE_MODS_EFFECTIVE |
                               XKB_STATE_LEDS));
            break;
        case STICKY_KEY_ACTION_LOCKCONTROLS:
            /* LockControls() */
            changed = update_key(sm, events, state, use_events,
                                 KEY_F2 + EVDEV_OFFSET, XKB_KEY_DOWN);
            assert(changed == (XKB_STATE_MODS_LATCHED |
                               XKB_STATE_MODS_EFFECTIVE |
                               XKB_STATE_LAYOUT_LATCHED |
                               XKB_STATE_LAYOUT_EFFECTIVE));
            mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
            assert(mods == caps);
            changed = update_key(sm, events, state, use_events,
                                 KEY_F2 + EVDEV_OFFSET, XKB_KEY_UP);
            assert(changed == (XKB_STATE_CONTROLS |
                               XKB_STATE_LAYOUT_LOCKED |
                               XKB_STATE_LAYOUT_EFFECTIVE |
                               XKB_STATE_MODS_LOCKED |
                               XKB_STATE_MODS_EFFECTIVE |
                               XKB_STATE_LEDS));
            break;
        case STICKY_KEY_EVENTS_API: {
            changed = update_controls(sm, events, state, true,
                                      XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS, 0);
            assert(changed == (XKB_STATE_CONTROLS |
                               XKB_STATE_LAYOUT_LATCHED |
                               XKB_STATE_LAYOUT_LOCKED |
                               XKB_STATE_LAYOUT_EFFECTIVE |
                               XKB_STATE_MODS_LATCHED |
                               XKB_STATE_MODS_LOCKED |
                               XKB_STATE_MODS_EFFECTIVE |
                               XKB_STATE_LEDS));
            break;
        }
        case STICKY_KEY_LEGACY_API:
            changed = xkb_state_update_controls(state,
                                                XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS,
                                                0);
            assert(changed == (XKB_STATE_CONTROLS |
                               XKB_STATE_LAYOUT_LATCHED |
                               XKB_STATE_LAYOUT_LOCKED |
                               XKB_STATE_LAYOUT_EFFECTIVE |
                               XKB_STATE_MODS_LATCHED |
                               XKB_STATE_MODS_LOCKED |
                               XKB_STATE_MODS_EFFECTIVE |
                               XKB_STATE_LEDS));
            break;
        }
        controls = xkb_state_serialize_controls(state, XKB_STATE_CONTROLS);
        assert(controls == 0);
        mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
        assert(mods == 0);
        assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE) == 0);

        /* Mods are not latched anymore */
        changed = update_key(sm, events, state, use_events,
                             KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
        assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE));
        changed = update_key(sm, events, state, use_events,
                             KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);
        assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE));
        mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
        assert(mods == 0);

        controls = xkb_state_serialize_controls(state, XKB_STATE_CONTROLS);
        assert(controls == 0);
    }

    xkb_state_unref(state);
    xkb_event_iterator_destroy(events);
    xkb_state_machine_unref(sm);

    /*
     * Test latch-to-lock
     */

    struct xkb_state_machine_options * const sm_options =
        xkb_state_machine_options_new(ctx);
    assert(sm_options);
    assert(xkb_state_machine_options_update_a11y_flags(
                sm_options,
                XKB_STATE_A11Y_LATCH_TO_LOCK,
                XKB_STATE_A11Y_LATCH_TO_LOCK) == 0);
    sm = xkb_state_machine_new(keymap, sm_options);
    assert(sm);
    xkb_state_machine_options_destroy(sm_options);
    events = xkb_event_iterator_new(ctx, XKB_EVENT_ITERATOR_NO_FLAGS);
    assert(events);
    state = xkb_state_new(keymap);
    assert(state);
    update_controls(sm, events, state, true,
                    XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS,
                    XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS);

    /* Latch shift */
    changed = update_key(sm, events, state, true,
                         KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
    assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE));
    changed = update_key(sm, events, state, true,
                         KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);
    assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED));
    mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_LATCHED);
    assert(mods == shift);
    mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
    assert(mods == shift);
    /* Lock shift */
    changed = update_key(sm, events, state, true,
                         KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
    assert(changed == ( XKB_STATE_MODS_DEPRESSED |
                        XKB_STATE_MODS_LATCHED |
                        XKB_STATE_MODS_LOCKED |
                        XKB_STATE_LEDS /* shift-lock */));
    mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_DEPRESSED);
    assert(mods == shift);
    mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_LATCHED);
    assert(mods == 0);
    mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_LOCKED);
    assert(mods == shift);
    changed = update_key(sm, events, state, true,
                         KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);
    assert(changed == XKB_STATE_MODS_DEPRESSED);
    mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
    assert(mods == shift);
    /* Unlock shift */
    changed = update_key(sm, events, state, true,
                         KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
    assert(changed == XKB_STATE_MODS_DEPRESSED);
    changed = update_key(sm, events, state, true,
                         KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);
    assert(changed == ( XKB_STATE_MODS_DEPRESSED |
                        XKB_STATE_MODS_LOCKED |
                        XKB_STATE_MODS_EFFECTIVE |
                        XKB_STATE_LEDS /* shift-lock */));
    mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
    assert(mods == 0);

    xkb_state_unref(state);
    xkb_event_iterator_destroy(events);
    xkb_state_machine_unref(sm);

    xkb_keymap_unref(keymap);
}

static void
test_layout_index_named_bounds(struct xkb_context *ctx)
{
    struct xkb_keymap * keymap = test_compile_rules(
        ctx, XKB_KEYMAP_FORMAT_TEXT_V2, "evdev-modern", "pc104",
        "us,in,ch,cz,de", NULL, "grp:shift_caps_switch,grp:group_bounds"
    );
    assert(keymap);
    struct xkb_state * state = xkb_state_new(keymap);
    assert(state);

    assert(xkb_state_led_name_is_active(state, XKB_LED_NAME_SCROLL));

    /* Last layout */
    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_UP);
    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);
    assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE) == 4);
    assert(!xkb_state_led_name_is_active(state, XKB_LED_NAME_SCROLL));

    xkb_state_update_latched_locked(state, 0, 0, false, 0, 0, 0, true, 2);
    assert(xkb_state_led_name_is_active(state, XKB_LED_NAME_SCROLL));

    /* First layout */
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_UP);
    assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE) == 0);
    assert(xkb_state_led_name_is_active(state, XKB_LED_NAME_SCROLL));

    xkb_state_unref(state);
    xkb_keymap_unref(keymap);

    /*
     * Ensure that the limitation of one group per key in a section holds when
     * using the RMLVO API and is consistent with the `Last` group constant value.
     */

    keymap = test_compile_rules(
        ctx, XKB_KEYMAP_FORMAT_TEXT_V2, "evdev-modern", "pc104",
        "multiple-groups,de,cz", NULL, "grp:shift_caps_switch,grp:group_bounds"
    );
    assert(keymap);
    state = xkb_state_new(keymap);
    assert(state);

    assert(xkb_keymap_num_layouts_for_key(keymap, KEY_RIGHTALT + EVDEV_OFFSET) == 3);
    assert(xkb_keymap_num_layouts(keymap) == 3);
    assert(xkb_state_led_name_is_active(state, XKB_LED_NAME_SCROLL));

    /* Last layout */
    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_UP);
    xkb_state_update_key(state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);
    assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE) == 2);
    assert(!xkb_state_led_name_is_active(state, XKB_LED_NAME_SCROLL));

    xkb_state_unref(state);
    xkb_keymap_unref(keymap);
}

static void
test_redirect_key(struct xkb_context *ctx)
{
    struct xkb_keymap * const keymap = test_compile_file(
        ctx, XKB_KEYMAP_FORMAT_TEXT_V1, "keymaps/redirect-key-1.xkb"
    );
    assert(keymap);

    struct xkb_state_machine *sm = xkb_state_machine_new(keymap, NULL);
    assert(sm);

    static const xkb_mod_mask_t shift = UINT32_C(1) << XKB_MOD_INDEX_SHIFT;
    static const xkb_mod_mask_t ctrl = UINT32_C(1) << XKB_MOD_INDEX_CTRL;

    struct xkb_event_iterator *events =
        xkb_event_iterator_new(ctx, XKB_EVENT_ITERATOR_NO_FLAGS);
    assert(events);

    xkb_state_machine_update_latched_locked(sm, events, 0, 0, false, 0,
                                            ctrl, ctrl, false, 0);

    const struct {
        xkb_keycode_t keycode;
        struct test_events {
            struct xkb_event events[3];
            unsigned int events_count;
        } down;
        struct test_events up;
    } tests[] = {
        {
            .keycode = EVDEV_OFFSET + KEY_A,
            .down = {
                .events = {
                    {
                        .type = XKB_EVENT_TYPE_KEY_DOWN,
                        .keycode = EVDEV_OFFSET + KEY_A
                    }
                },
                .events_count = 1
            },
            .up = {
                .events = {
                    {
                        .type = XKB_EVENT_TYPE_KEY_UP,
                        .keycode = EVDEV_OFFSET + KEY_A
                    }
                },
                .events_count = 1
            }
        },
        {
            .keycode = EVDEV_OFFSET + KEY_S,
            .down = {
                .events = {
                    {
                        .type = XKB_EVENT_TYPE_KEY_DOWN,
                        .keycode = EVDEV_OFFSET + KEY_A
                    }
                },
                .events_count = 1
            },
            .up = {
                .events = {
                    {
                        .type = XKB_EVENT_TYPE_KEY_UP,
                        .keycode = EVDEV_OFFSET + KEY_A
                    }
                },
                .events_count = 1
            }
        },
        {
            .keycode = EVDEV_OFFSET + KEY_D,
            .down = {
                .events = {
                    {
                        .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
                        .components = {
                            .components = {
                                .base_mods = shift,
                                .latched_mods = shift,
                                .locked_mods = shift,
                                .mods = shift,
                            },
                            .changed = XKB_STATE_MODS_DEPRESSED
                                     | XKB_STATE_MODS_LATCHED
                                     | XKB_STATE_MODS_LOCKED
                                     | XKB_STATE_MODS_EFFECTIVE
                        }
                    },
                    {
                        .type = XKB_EVENT_TYPE_KEY_DOWN,
                        .keycode = EVDEV_OFFSET + KEY_S
                    },
                    {
                        .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
                        .components = {
                            .components = {
                                .base_mods = 0,
                                .latched_mods = 0,
                                .locked_mods = ctrl,
                                .mods = ctrl,
                            },
                            .changed = XKB_STATE_MODS_DEPRESSED
                                     | XKB_STATE_MODS_LATCHED
                                     | XKB_STATE_MODS_LOCKED
                                     | XKB_STATE_MODS_EFFECTIVE
                        }
                    },
                },
                .events_count = 3
            },
            .up = {
                .events = {
                    {
                        .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
                        .components = {
                            .components = {
                                .base_mods = shift,
                                .latched_mods = shift,
                                .locked_mods = shift,
                                .mods = shift,
                            },
                            .changed = XKB_STATE_MODS_DEPRESSED
                                     | XKB_STATE_MODS_LATCHED
                                     | XKB_STATE_MODS_LOCKED
                                     | XKB_STATE_MODS_EFFECTIVE
                        }
                    },
                    {
                        .type = XKB_EVENT_TYPE_KEY_UP,
                        .keycode = EVDEV_OFFSET + KEY_S
                    },
                    {
                        .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
                        .components = {
                            .components = {
                                .base_mods = 0,
                                .latched_mods = 0,
                                .locked_mods = ctrl,
                                .mods = ctrl,
                            },
                            .changed = XKB_STATE_MODS_DEPRESSED
                                     | XKB_STATE_MODS_LATCHED
                                     | XKB_STATE_MODS_LOCKED
                                     | XKB_STATE_MODS_EFFECTIVE
                        }
                    },
                },
                .events_count = 3
            }
        },
    };

    for (uint8_t t = 0; t < (uint8_t) ARRAY_SIZE(tests); t++) {
        fprintf(stderr, "------\n*** %s: #%u, keycode: %"PRIu32" ***\n",
                __func__, t, tests[t].keycode);
        assert(0 == xkb_state_machine_update_key(sm, events, tests[t].keycode,
                                                 XKB_KEY_DOWN));
        assert(check_events(events, tests[t].down.events,
                            tests[t].down.events_count));
        assert(0 == xkb_state_machine_update_key(sm, events, tests[t].keycode,
                                                 XKB_KEY_UP));
        assert(check_events(events, tests[t].up.events,
                            tests[t].up.events_count));
    }

    xkb_event_iterator_destroy(events);
    xkb_state_machine_unref(sm);
    xkb_keymap_unref(keymap);
}

static void
test_shortcuts_tweak(struct xkb_context *context)
{
    struct xkb_state_machine_options * const options =
        xkb_state_machine_options_new(context);
    assert(options);

    const xkb_mod_mask_t ctrl = UINT32_C(1) << XKB_MOD_INDEX_CTRL;

    assert(xkb_state_machine_options_shortcuts_update_mods(options, ctrl, ctrl)
           == 0);
    assert(xkb_state_machine_options_shortcuts_set_mapping(options, 1, 2) == 0);
    assert(xkb_state_machine_options_shortcuts_set_mapping(options, 3, 0) == 0);

    struct xkb_keymap * const keymap =
        test_compile_rules(context, XKB_KEYMAP_FORMAT_TEXT_V1,
                           "evdev", "pc104", "us,il,de,ru", NULL,
                           "grp:menu_toggle,grp:win_switch");
    assert(keymap);

    struct xkb_state_machine * const sm = xkb_state_machine_new(keymap, options);
    assert(sm);
    xkb_state_machine_options_destroy(options);

    struct xkb_event_iterator * const events =
        xkb_event_iterator_new(context, XKB_EVENT_ITERATOR_NO_FLAGS);
    assert(events);

    /*
     * xkb_state_machine_update_key
     */

    assert(test_key_seq2(
        keymap, sm, events,
        KEY_Q       , BOTH, XKB_KEY_q             , NEXT,
        KEY_Z       , BOTH, XKB_KEY_z             , NEXT,
        KEY_LEFTCTRL, DOWN, XKB_KEY_Control_L     , NEXT,
        KEY_Q       , BOTH, XKB_KEY_q             , NEXT,
        KEY_Z       , BOTH, XKB_KEY_z             , NEXT,
        KEY_LEFTCTRL, UP  , XKB_KEY_Control_L     , NEXT,
        KEY_LEFTALT , DOWN, XKB_KEY_Alt_L         , NEXT,
        KEY_Q       , BOTH, XKB_KEY_q             , NEXT,
        KEY_Z       , BOTH, XKB_KEY_z             , NEXT,
        KEY_LEFTALT , UP  , XKB_KEY_Alt_L         , NEXT,

        /* Layout 2: set */

        KEY_LEFTMETA, DOWN, XKB_KEY_ISO_Group_Shift, NEXT,
        KEY_Q       , BOTH, XKB_KEY_slash          , NEXT, /* Layout 2 */
        KEY_Z       , BOTH, XKB_KEY_hebrew_zain    , NEXT,
        KEY_LEFTMETA, UP  , XKB_KEY_ISO_Group_Shift, NEXT,

        KEY_LEFTCTRL, DOWN, XKB_KEY_Control_L      , NEXT,
        KEY_Q       , BOTH, XKB_KEY_q              , NEXT, /* Layout 1 (unchanged) */
        KEY_Z       , BOTH, XKB_KEY_z              , NEXT,
        KEY_LEFTMETA, DOWN, XKB_KEY_ISO_Group_Shift, NEXT, /* Layout 2 */
        KEY_Q       , BOTH, XKB_KEY_q              , NEXT, /* Redirect to layout 3 */
        KEY_Z       , BOTH, XKB_KEY_y              , NEXT,
        KEY_LEFTCTRL, UP  , XKB_KEY_Control_L      , NEXT,
        KEY_Q       , BOTH, XKB_KEY_slash          , NEXT, /* Layout 2 */
        KEY_Z       , BOTH, XKB_KEY_hebrew_zain    , NEXT,
        KEY_LEFTALT , DOWN, XKB_KEY_Alt_L          , NEXT,
        KEY_Q       , BOTH, XKB_KEY_slash          , NEXT, /* No redirection with Alt */
        KEY_Z       , BOTH, XKB_KEY_hebrew_zain    , NEXT,
        KEY_LEFTALT , UP  , XKB_KEY_Alt_L          , NEXT,
        KEY_LEFTMETA, UP  , XKB_KEY_ISO_Group_Shift, NEXT, /* Layout 1 */

        /* Layout 2: lock */

        KEY_COMPOSE , BOTH, XKB_KEY_ISO_Next_Group, NEXT,

        KEY_Q       , BOTH, XKB_KEY_slash         , NEXT,
        KEY_Z       , BOTH, XKB_KEY_hebrew_zain   , NEXT,
        /* Match mask: redirect to layout 1 */
        KEY_LEFTCTRL, DOWN, XKB_KEY_Control_L     , NEXT,
        KEY_Q       , BOTH, XKB_KEY_q             , NEXT,
        KEY_Z       , BOTH, XKB_KEY_y             , NEXT,
        KEY_LEFTCTRL, UP  , XKB_KEY_Control_L     , NEXT,
        KEY_Q       , BOTH, XKB_KEY_slash         , NEXT,
        KEY_Z       , BOTH, XKB_KEY_hebrew_zain   , NEXT,
        /* No match: no redirect */
        KEY_LEFTALT , DOWN, XKB_KEY_Alt_L         , NEXT,
        KEY_Q       , BOTH, XKB_KEY_slash         , NEXT,
        KEY_Z       , BOTH, XKB_KEY_hebrew_zain   , NEXT,
        /* Match mask: redirect to layout 1 */
        KEY_LEFTCTRL, DOWN, XKB_KEY_Control_L     , NEXT,
        KEY_Q       , BOTH, XKB_KEY_q             , NEXT,
        KEY_Z       , BOTH, XKB_KEY_y             , NEXT,
        KEY_LEFTCTRL, UP  , XKB_KEY_Control_L     , NEXT,
        KEY_Q       , BOTH, XKB_KEY_slash         , NEXT,
        KEY_Z       , BOTH, XKB_KEY_hebrew_zain   , NEXT,
        KEY_LEFTALT , UP  , XKB_KEY_Alt_L         , NEXT,
        KEY_COMPOSE , BOTH, XKB_KEY_ISO_Next_Group, NEXT,

        /* Layout 3 */

        KEY_Q       , BOTH, XKB_KEY_q             , NEXT,
        KEY_Z       , BOTH, XKB_KEY_y             , NEXT,
        KEY_LEFTCTRL, DOWN, XKB_KEY_Control_L     , NEXT,
        KEY_Q       , BOTH, XKB_KEY_q             , NEXT,
        KEY_Z       , BOTH, XKB_KEY_y             , NEXT,
        KEY_LEFTCTRL, UP  , XKB_KEY_Control_L     , NEXT,
        KEY_LEFTALT , DOWN, XKB_KEY_Alt_L         , NEXT,
        KEY_Q       , BOTH, XKB_KEY_q             , NEXT,
        KEY_Z       , BOTH, XKB_KEY_y             , NEXT,
        KEY_LEFTCTRL, DOWN, XKB_KEY_Control_L     , NEXT,
        KEY_Q       , BOTH, XKB_KEY_q             , NEXT,
        KEY_Z       , BOTH, XKB_KEY_y             , NEXT,
        KEY_LEFTCTRL, UP  , XKB_KEY_Control_L     , NEXT,
        KEY_LEFTALT , UP  , XKB_KEY_Alt_L         , NEXT,
        KEY_COMPOSE , BOTH, XKB_KEY_ISO_Next_Group, NEXT,

        /* Layout 4 */

        KEY_Q       , BOTH, XKB_KEY_Cyrillic_shorti, NEXT,
        KEY_Z       , BOTH, XKB_KEY_Cyrillic_ya    , NEXT,
        KEY_LEFTCTRL, DOWN, XKB_KEY_Control_L      , NEXT,
        KEY_Q       , BOTH, XKB_KEY_q              , NEXT,
        KEY_Z       , BOTH, XKB_KEY_z              , NEXT,
        KEY_LEFTCTRL, UP  , XKB_KEY_Control_L      , NEXT,
        KEY_LEFTALT , DOWN, XKB_KEY_Alt_L          , NEXT,
        KEY_Q       , BOTH, XKB_KEY_Cyrillic_shorti, NEXT,
        KEY_Z       , BOTH, XKB_KEY_Cyrillic_ya    , NEXT,
        KEY_LEFTCTRL, DOWN, XKB_KEY_Control_L      , NEXT,
        KEY_Q       , BOTH, XKB_KEY_q              , NEXT,
        KEY_Z       , BOTH, XKB_KEY_z              , NEXT,
        KEY_LEFTCTRL, UP  , XKB_KEY_Control_L      , NEXT,
        KEY_LEFTALT , UP  , XKB_KEY_Alt_L          , NEXT,
        KEY_COMPOSE , BOTH, XKB_KEY_ISO_Next_Group , NEXT,

        /* Layout 1 */

        KEY_Q       , BOTH, XKB_KEY_q             , NEXT,
        KEY_Z       , BOTH, XKB_KEY_z             , FINISH
    ));

    /*
     * xkb_state_machine_update_latched_locked
     */

    /* Layout 1 locked, Ctrl latched */
    const xkb_led_mask_t group2 = UINT16_C(1) << xkb_keymap_led_get_index(keymap, "Group 2");
    assert(xkb_state_machine_update_latched_locked(sm, events,
                                                   ctrl, ctrl, false, 0,
                                                   0, 0, true, 1) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
            .components = {
                .changed = XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE
                         | XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_LOCKED
                         | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_LEDS,
                .components = {
                    .latched_mods = ctrl,
                    .locked_mods = 0,
                    .mods = ctrl,
                    .base_group = 1,
                    .latched_group = 0,
                    .locked_group = 1,
                    .group = 2,
                    .leds = group2,
                }
            }
        }
    );

    /* Layout 1 locked, Ctrl locked */
    assert(xkb_state_machine_update_latched_locked(sm, events,
                                                   ctrl, 0, false, 0,
                                                   ctrl, ctrl, false, 0) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
            .components = {
                .changed = XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_LOCKED,
                .components = {
                    .latched_mods = 0,
                    .locked_mods = ctrl,
                    .mods = ctrl,
                    .base_group = 1,
                    .latched_group = 0,
                    .locked_group = 1,
                    .group = 2,
                    .leds = group2,
                }
            }
        }
    );

    /* Layout 1 latched, layout 2 locked, Ctrl locked */
    assert(xkb_state_machine_update_latched_locked(sm, events,
                                                   0, 0, true, 1,
                                                   0, 0, true, 2) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
            .components = {
                .changed = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_LATCHED
                         | XKB_STATE_LAYOUT_LOCKED | XKB_STATE_LAYOUT_EFFECTIVE
                         | XKB_STATE_LEDS,
                .components = {
                    .latched_mods = 0,
                    .locked_mods = ctrl,
                    .mods = ctrl,
                    .base_group = -3,
                    .latched_group = 1,
                    .locked_group = 2,
                    .group = 0,
                    .leds = 0,
                }
            }
        }
    );

    /* Layout 1 latched, layout 2 locked, Ctrl disabled */
    assert(xkb_state_machine_update_latched_locked(sm, events,
                                                   0, 0, false, 0,
                                                   ctrl, 0, false, 0) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
            .components = {
                .changed = XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE
                         | XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE
                         | XKB_STATE_LEDS,
                .components = {
                    .latched_mods = 0,
                    .locked_mods = 0,
                    .mods = 0,
                    .base_group = 0,
                    .latched_group = 1,
                    .locked_group = 2,
                    .group = 3,
                    .leds = group2,
                }
            }
        }
    );

    /* Layout 1 latched, layout 2 locked, Ctrl latched */
    assert(xkb_state_machine_update_latched_locked(sm, events,
                                                   ctrl, ctrl, false, 0,
                                                   0, 0, false, 0) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
            .components = {
                .changed = XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE
                         | XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE
                         | XKB_STATE_LEDS,
                .components = {
                    .latched_mods = ctrl,
                    .locked_mods = 0,
                    .mods = ctrl,
                    .base_group = -3,
                    .latched_group = 1,
                    .locked_group = 2,
                    .group = 0,
                    .leds = 0,
                }
            }
        }
    );

    /*
     * xkb_state_machine_update_controls
     */

    const enum xkb_keyboard_controls controls =
        XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS;

    /* Disable already disabled sticky keys: no change */
    assert(xkb_state_machine_update_controls(sm, events, controls, 0) == 0);
    check_events_(events, { .type = XKB_EVENT_TYPE_NONE });

    /* Enable disabled sticky keys: no change */
    assert(xkb_state_machine_update_controls(sm, events, controls, controls) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
            .components = {
                .changed = XKB_STATE_CONTROLS,
                .components = {
                    .latched_mods = ctrl,
                    .locked_mods = 0,
                    .mods = ctrl,
                    .base_group = -3,
                    .latched_group = 1,
                    .locked_group = 2,
                    .group = 0,
                    .leds = 0,
                    .controls = CONTROL_STICKY_KEYS,
                }
            }
        }
    );

    /* Enable already enabled sticky keys: no change */
    assert(xkb_state_machine_update_controls(sm, events, controls, controls) == 0);
    check_events_(events, { .type = XKB_EVENT_TYPE_NONE });

    /* Disable sticky keys: clear latches & locks */
    assert(xkb_state_machine_update_controls(sm, events, controls, 0) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
            .components = {
                .changed = XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE
                         | XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_LATCHED
                         | XKB_STATE_LAYOUT_LOCKED | XKB_STATE_CONTROLS,
                .components = {
                    .latched_mods = 0,
                    .locked_mods = 0,
                    .mods = 0,
                    .base_group = 0,
                    .latched_group = 0,
                    .locked_group = 0,
                    .group = 0,
                    .leds = 0,
                    .controls = 0,
                }
            }
        }
    );

    assert(xkb_state_machine_update_controls(sm, events, controls, 0) == 0);

    xkb_event_iterator_destroy(events);
    xkb_state_machine_unref(sm);
    xkb_keymap_unref(keymap);
}

#undef check_events_

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
    xkb_state_machine_unref(NULL);
    xkb_state_machine_options_destroy(NULL);
    xkb_event_iterator_destroy(NULL);

    test_state_machine_options(context);
    test_group_wrap(context);
    test_initial_derived_values(context);

    assert(!xkb_event_iterator_new(context, -1));

    const char* rules[] = {"evdev", "evdev-pure-virtual-mods"};
    for (size_t r = 0; r < ARRAY_SIZE(rules); r++) {
        fprintf(stderr, "=== Rules set: %s ===\n", rules[r]);
        keymap = test_compile_rules(context, XKB_KEYMAP_FORMAT_TEXT_V1, rules[r],
                                    "pc104", "us,ru", NULL,
                                    "grp:menu_toggle,grp:lwin_latch,"
                                    "grp:rwin_latch_lock_clear,"
                                    "grp:sclk_latch_no_lock,"
                                    "lv3:lsgt_latch,lv3:bksl_latch_no_lock");
        assert(keymap);
        const bool pure_vmods = !!r;

        test_update_key(context, keymap, pure_vmods);
        test_update_latched_locked(keymap);
        test_serialisation(keymap, pure_vmods);
        test_update_mask_mods(keymap, pure_vmods);
        test_repeat(keymap);
        test_consume(keymap, pure_vmods);
        test_keycode_range(keymap);
        test_get_utf8_utf32(keymap);
        test_ctrl_string_transformation(keymap);

        xkb_keymap_unref(keymap);
    }

    test_inactive_key_type_entry(context);
    test_overlapping_mods(context);
    test_caps_keysym_transformation(context);
    test_leds(context);
    test_multiple_actions(context);
    test_void_action(context);
    test_extended_layout_indices(context);
    test_sticky_keys(context);
    test_layout_index_named_bounds(context);
    test_redirect_key(context);
    test_shortcuts_tweak(context);

    xkb_context_unref(context);
    return EXIT_SUCCESS;
}
