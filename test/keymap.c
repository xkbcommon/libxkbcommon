/*
 * Copyright © 2016 Intel Corporation
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
 * Author: Mike Blumenkrantz <zmike@osg.samsung.com>
 */

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "evdev-scancodes.h"
#include "test.h"
#include "keymap.h"

#define KEY_LVL3 84
#define KEY_LVL5 195

static void
test_garbage_key(void)
{
    struct xkb_context *context = test_get_context(CONTEXT_NO_FLAG);
    struct xkb_keymap *keymap;
    xkb_keycode_t kc;
    int nsyms;
    const xkb_keysym_t *syms;
    const xkb_layout_index_t first_layout = 0;
    xkb_level_index_t nlevels;

    assert(context);

    keymap = test_compile_rules(context, NULL, NULL, "garbage", NULL, NULL);
    assert(keymap);

    /* TLDE uses the 'us' sym on the first level and is thus [grave, exclam] */
    kc = xkb_keymap_key_by_name(keymap, "TLDE");
    assert(kc != XKB_KEYCODE_INVALID);
    nlevels = xkb_keymap_num_levels_for_key(keymap, kc, first_layout);
    assert(nlevels == 2);
    nsyms = xkb_keymap_key_get_syms_by_level(keymap, kc, first_layout, 0, &syms);
    assert(nsyms == 1);
    assert(*syms == XKB_KEY_grave); /* fallback from 'us' */
    nsyms = xkb_keymap_key_get_syms_by_level(keymap, kc, first_layout, 1, &syms);
    assert(nsyms == 1);
    assert(*syms == XKB_KEY_exclam);

    /* AE13 has no 'us' fallback and ends up as [NoSymbol, asciitilde] */
    kc = xkb_keymap_key_by_name(keymap, "AE13");
    assert(kc != XKB_KEYCODE_INVALID);
    nlevels = xkb_keymap_num_levels_for_key(keymap, kc, first_layout);
    assert(nlevels == 2);
    nsyms = xkb_keymap_key_get_syms_by_level(keymap, kc, first_layout, 0, &syms);
    assert(nsyms == 0);
    nsyms = xkb_keymap_key_get_syms_by_level(keymap, kc, first_layout, 1, &syms);
    assert(nsyms == 1);
    assert(*syms == XKB_KEY_asciitilde);

    xkb_keymap_unref(keymap);
    xkb_context_unref(context);
}

static void
test_keymap(void)
{
    struct xkb_context *context = test_get_context(CONTEXT_NO_FLAG);
    struct xkb_keymap *keymap;
    xkb_keycode_t kc;
    const char *keyname;
    xkb_mod_mask_t masks_out[4] = { 0, 0, 0, 0 };
    size_t mask_count;
    xkb_mod_mask_t shift_mask;
    xkb_mod_mask_t lock_mask;
    xkb_mod_mask_t mod2_mask;

    assert(context);

    keymap = test_compile_rules(context, "evdev", "pc104", "us,ru", NULL, "grp:menu_toggle");
    assert(keymap);

    kc = xkb_keymap_key_by_name(keymap, "AE09");
    assert(kc != XKB_KEYCODE_INVALID);
    keyname = xkb_keymap_key_get_name(keymap, kc);
    assert(streq(keyname, "AE09"));

    kc = xkb_keymap_key_by_name(keymap, "COMP");
    assert(kc != XKB_KEYCODE_INVALID);
    keyname = xkb_keymap_key_get_name(keymap, kc);
    assert(streq(keyname, "COMP"));

    kc = xkb_keymap_key_by_name(keymap, "MENU");
    assert(kc != XKB_KEYCODE_INVALID);
    keyname = xkb_keymap_key_get_name(keymap, kc);
    assert(streq(keyname, "COMP"));

    kc = xkb_keymap_key_by_name(keymap, "AC01");
    assert(kc != XKB_KEYCODE_INVALID);

    // AC01 level 0 ('a') requires no modifiers on us-pc104
    mask_count = xkb_keymap_key_get_mods_for_level(keymap, kc, 0, 0, masks_out, 4);
    assert(mask_count == 1);
    assert(masks_out[0] == 0);

    shift_mask = UINT32_C(1) << xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_SHIFT);
    lock_mask = UINT32_C(1) << xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_CAPS);
    mod2_mask = UINT32_C(1) << xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_MOD2);

    // AC01 level 1 ('A') requires either Shift or Lock modifiers on us-pc104
    mask_count = xkb_keymap_key_get_mods_for_level(keymap, kc, 0, 1, masks_out, 4);
    assert(mask_count == 2);
    assert(masks_out[0] == shift_mask);
    assert(masks_out[1] == lock_mask);

    kc = xkb_keymap_key_by_name(keymap, "KP1");

    // KP1 level 0 ('End') requires no modifiers or Shift+Mod2 on us-pc104
    mask_count = xkb_keymap_key_get_mods_for_level(keymap, kc, 0, 0, masks_out, 4);
    assert(mask_count == 2);
    assert(masks_out[0] == 0);
    assert(masks_out[1] == (shift_mask | mod2_mask));

    // KP1 level 1 ('1') requires either Shift or Mod2 modifiers on us-pc104
    mask_count = xkb_keymap_key_get_mods_for_level(keymap, kc, 0, 1, masks_out, 4);
    assert(mask_count == 2);
    assert(masks_out[0] == shift_mask);
    assert(masks_out[1] == mod2_mask);

    // Return key is not affected by modifiers on us-pc104
    kc = xkb_keymap_key_by_name(keymap, "RTRN");
    mask_count = xkb_keymap_key_get_mods_for_level(keymap, kc, 0, 0, masks_out, 4);
    assert(mask_count == 1);
    assert(masks_out[0] == 0);

    xkb_keymap_unref(keymap);
    xkb_context_unref(context);
}

static void
test_no_extra_groups(void)
{
    struct xkb_keymap *keymap;
    xkb_keycode_t kc;
    const xkb_keysym_t *syms;
    struct xkb_context *context = test_get_context(CONTEXT_NO_FLAG);
    assert(context);

    /* RMLVO: Legacy rules may add more layouts than the input RMLVO */
    keymap = test_compile_rules(context, "multiple-groups",
                                "old", "de", NULL, NULL);
    assert(keymap);
    kc = xkb_keymap_key_by_name(keymap, "AD01");
    assert(kc != XKB_KEYCODE_INVALID);
    /* 2 layouts, while only 1 was provided */
    assert(xkb_keymap_num_layouts_for_key(keymap, kc) == 2);
    assert(xkb_keymap_num_layouts(keymap) == 2);
    xkb_keymap_unref(keymap);

    /* RMLVO: “one group per key” in symbols sections */
    const char *layouts[] = {"us", "us,us", "us,us,us", "us,us,us,us"};
    for (xkb_layout_index_t k = 0; k < ARRAY_SIZE(layouts); k++) {
        /* `multiple-groups` option defines 4 groups for a key */
        keymap = test_compile_rules(context, "multiple-groups", NULL,
                                    layouts[k], NULL, "multiple-groups");
        assert(keymap);
        kc = xkb_keymap_key_by_name(keymap, "RALT");
        assert(kc != XKB_KEYCODE_INVALID);
        /* Groups after the first one should be dropped */
        assert(xkb_keymap_num_layouts_for_key(keymap, kc) == 1);
        /* Here we expect RMLVO layouts count = keymap layouts count */
        assert(xkb_keymap_num_layouts(keymap) == k + 1);
        for (xkb_layout_index_t l = 0; l <= k; l++) {
            assert(xkb_keymap_key_get_syms_by_level(keymap, kc, l, 0, &syms) == 1);
            assert(syms[0] == XKB_KEY_a);
            syms = NULL;
        }
        xkb_keymap_unref(keymap);
    }

    /* RMLVO: Ensure the rule “one group per key” in symbols sections works
     * for the 2nd layout */
    keymap = test_compile_rules(context, NULL, NULL,
                                "multiple-groups,multiple-groups", "1,2", NULL);
    assert(keymap);
    kc = xkb_keymap_key_by_name(keymap, "RALT");
    assert(kc != XKB_KEYCODE_INVALID);
    assert(xkb_keymap_num_layouts_for_key(keymap, kc) == 2);
    assert(xkb_keymap_num_layouts(keymap) == 2);
    for (xkb_layout_index_t l = 0; l < 2; l++) {
        assert(xkb_keymap_key_get_syms_by_level(keymap, kc, l, 0, &syms) == 1);
        assert(syms[0] == XKB_KEY_a);
        syms = NULL;
    }
    xkb_keymap_unref(keymap);

    /* Same configuration as previous, but without using RMLVO resolution:
     * We do accept more than one group per key in symbols sections, but only
     * when not using a modifier: the second layout (`:2`) will have its extra
     * groups dropped. */
    const char keymap_str[] =
        "xkb_keymap {"
        "  xkb_keycodes { include \"evdev+aliases(qwerty)\" };"
        "  xkb_types { include \"complete\" };"
        "  xkb_compat { include \"complete\" };"
        "  xkb_symbols { include \"pc+multiple-groups(1)+multiple-groups(2):2"
                                  "+inet(evdev)\" };"
        "};";
    keymap = test_compile_string(context, keymap_str);
    kc = xkb_keymap_key_by_name(keymap, "RALT");
    assert(kc != XKB_KEYCODE_INVALID);
    assert(xkb_keymap_num_layouts_for_key(keymap, kc) == 4);
    assert(xkb_keymap_num_layouts(keymap) == 4);
    xkb_keysym_t ref_syms[] = { XKB_KEY_a, XKB_KEY_a, XKB_KEY_c, XKB_KEY_d };
    for (xkb_layout_index_t l = 0; l < 4; l++) {
        assert(xkb_keymap_key_get_syms_by_level(keymap, kc, l, 0, &syms) == 1);
        assert(syms[0] == ref_syms[l]);
        syms = NULL;
    }
    xkb_keymap_unref(keymap);

    xkb_context_unref(context);
}

#define Mod1Mask (UINT32_C(1) << 3)
#define Mod2Mask (UINT32_C(1) << 4)
#define Mod3Mask (UINT32_C(1) << 5)

static void
test_numeric_keysyms(void)
{
    struct xkb_context *context = test_get_context(CONTEXT_NO_FLAG);
    struct xkb_keymap *keymap;
    const struct xkb_key *key;
    xkb_keycode_t kc;
    int keysyms_count;
    const xkb_layout_index_t first_layout = 0;
    const xkb_keysym_t *keysyms;

    assert(context);

    keymap = test_compile_rules(context, "evdev", "pc104", "numeric_keysyms", NULL, NULL);
    assert(keymap);

    kc = xkb_keymap_key_by_name(keymap, "AD01");
    keysyms_count = xkb_keymap_key_get_syms_by_level(keymap, kc, first_layout, 0, &keysyms);
    assert(keysyms_count == 1);
    assert(keysyms[0] == 0x1ffffffd);
    key = XkbKey(keymap, kc);
    assert(key->modmap == Mod1Mask);

    kc = xkb_keymap_key_by_name(keymap, "AD02");
    keysyms_count = xkb_keymap_key_get_syms_by_level(keymap, kc, first_layout, 0, &keysyms);
    assert(keysyms_count == 1);
    assert(keysyms[0] == 0x1ffffffe);
    key = XkbKey(keymap, kc);
    assert(key->modmap == Mod2Mask);

    kc = xkb_keymap_key_by_name(keymap, "AD03");
    keysyms_count = xkb_keymap_key_get_syms_by_level(keymap, kc, first_layout, 0, &keysyms);
    assert(keysyms_count == 1);
    assert(keysyms[0] == 0x1fffffff);
    /* Invalid numeric keysym */
    keysyms_count = xkb_keymap_key_get_syms_by_level(keymap, kc, first_layout, 1, &keysyms);
    assert(keysyms_count == 0);
    key = XkbKey(keymap, kc);
    assert(key->modmap == Mod3Mask);

    xkb_keymap_unref(keymap);
    xkb_context_unref(context);
}

static void
test_multiple_keysyms_per_level(void)
{
    struct xkb_context *context = test_get_context(CONTEXT_NO_FLAG);
    struct xkb_keymap *keymap;
    xkb_keycode_t kc;
    int keysyms_count;
    const xkb_layout_index_t first_layout = 0;
    const xkb_keysym_t *keysyms;

    assert(context);

    keymap = test_compile_rules(context, "evdev", "pc104", "awesome", NULL, NULL);
    assert(keymap);

    kc = xkb_keymap_key_by_name(keymap, "AD01");
    keysyms_count = xkb_keymap_key_get_syms_by_level(keymap, kc, first_layout, 0, &keysyms);
    assert(keysyms_count == 3);
    assert(keysyms[0] == 'q');
    assert(keysyms[1] == 'a');
    assert(keysyms[2] == 'b');

    kc = xkb_keymap_key_by_name(keymap, "AD03");
    keysyms_count = xkb_keymap_key_get_syms_by_level(keymap, kc, first_layout, 1, &keysyms);
    assert(keysyms_count == 2);
    assert(keysyms[0] == 'E');
    assert(keysyms[1] == 'F');

    // Invalid keysyms
    kc = xkb_keymap_key_by_name(keymap, "AD06");
    // Only the invalid keysym is dropped, remaining one overrides previous entry
    keysyms_count = xkb_keymap_key_get_syms_by_level(keymap, kc, first_layout, 0, &keysyms);
    assert(keysyms_count == 1);
    assert(keysyms[0] == XKB_KEY_ydiaeresis);
    // All the keysyms are invalid and dropped, previous entry not overriden
    keysyms_count = xkb_keymap_key_get_syms_by_level(keymap, kc, first_layout, 1, &keysyms);
    assert(keysyms_count == 1);
    assert(keysyms[0] == 'Y');

    xkb_keymap_unref(keymap);
    xkb_context_unref(context);
}

static void
test_multiple_actions_per_level(void)
{
    struct xkb_context *context = test_get_context(0);
    struct xkb_keymap *keymap;
    struct xkb_state *state;
    xkb_keycode_t kc;
    int keysyms_count;
    const xkb_layout_index_t first_layout = 0;
    const xkb_keysym_t *keysyms;
    xkb_mod_index_t ctrl, level3;
    xkb_layout_index_t layout;
    xkb_mod_mask_t base_mods;

    assert(context);

    /* Test various ways to set multiple actions */
    keymap = test_compile_rules(context, "evdev", "pc104",
                                "multiple_actions,cz", NULL, NULL);
    assert(keymap);

    kc = xkb_keymap_key_by_name(keymap, "LCTL");
    ctrl = xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_CTRL);
    level3 = xkb_keymap_mod_get_index(keymap, "Mod5");
    state = xkb_state_new(keymap);
    assert(state);
    layout = xkb_state_key_get_layout(state, KEY_LEFTCTRL + EVDEV_OFFSET);
    assert(layout == 0);
    xkb_state_update_key(state, KEY_LEFTCTRL + EVDEV_OFFSET, XKB_KEY_DOWN);
    base_mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_DEPRESSED);
    assert(base_mods == (UINT32_C(1) << ctrl));
    layout = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_DEPRESSED);
    assert(layout == 1);
    layout = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LATCHED);
    assert(layout == 0);
    layout = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LOCKED);
    assert(layout == 0);
    layout = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE);
    assert(layout == 1);
    xkb_state_update_key(state, KEY_LEFTCTRL + EVDEV_OFFSET, XKB_KEY_UP);
    base_mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_DEPRESSED);
    assert(base_mods == 0);
    layout = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_DEPRESSED);
    assert(layout == 0);
    layout = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LATCHED);
    assert(layout == 0);
    layout = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LOCKED);
    assert(layout == 0);
    layout = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE);
    assert(layout == 0);
    xkb_state_update_key(state, KEY_LVL3 + EVDEV_OFFSET, XKB_KEY_DOWN);
    base_mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_DEPRESSED);
    assert(base_mods == (UINT32_C(1) << level3));
    layout = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_DEPRESSED);
    assert(layout == 1);
    layout = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LATCHED);
    assert(layout == 0);
    layout = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LOCKED);
    assert(layout == 0);
    layout = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE);
    assert(layout == 1);
    xkb_state_update_key(state, KEY_LVL3 + EVDEV_OFFSET, XKB_KEY_UP);
    base_mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_DEPRESSED);
    assert(base_mods == 0);
    layout = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_DEPRESSED);
    assert(layout == 0);
    layout = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LATCHED);
    assert(layout == 0);
    layout = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LOCKED);
    assert(layout == 0);
    layout = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE);
    assert(layout == 0);
    xkb_state_unref(state);

    assert(test_key_seq(
        keymap,
        KEY_2,         BOTH, XKB_KEY_2,         NEXT,
        /* Control switch to the second group */
        KEY_LEFTCTRL,  DOWN, XKB_KEY_Control_L, XKB_KEY_ISO_Group_Shift, NEXT,
        KEY_2,         BOTH, XKB_KEY_ecaron,    NEXT,
        KEY_LEFTCTRL,  UP,   XKB_KEY_Control_L, XKB_KEY_ISO_Group_Shift, NEXT,
        KEY_2,         BOTH, XKB_KEY_2,         NEXT,
        KEY_RIGHTCTRL, DOWN, XKB_KEY_Control_R, NEXT,
        KEY_2,         BOTH, XKB_KEY_ecaron,    NEXT,
        KEY_RIGHTCTRL, UP,   XKB_KEY_Control_R, NEXT,
        KEY_2,         BOTH, XKB_KEY_2,         NEXT,
        /* Fake keys switch to the second group too */
        KEY_LVL3,      DOWN, XKB_KEY_ISO_Level3_Shift, XKB_KEY_ISO_Group_Shift, NEXT,
        KEY_2,         BOTH, XKB_KEY_at,        NEXT,
        KEY_LVL3,      UP,   XKB_KEY_ISO_Level3_Shift,
                             /* Only one keysym, group=2 + level3(ralt_switch):2 */
                             NEXT,
        KEY_2,         BOTH, XKB_KEY_2,         NEXT,
        KEY_LVL5,      DOWN, XKB_KEY_ISO_Level3_Shift, XKB_KEY_ISO_Group_Shift, NEXT,
        KEY_2,         BOTH, XKB_KEY_at,        NEXT,
        KEY_LVL5,      UP,   XKB_KEY_ISO_Level3_Shift, XKB_KEY_ISO_Group_Shift, NEXT,
        KEY_2,         BOTH, XKB_KEY_2,         NEXT,
        /* Alt have different keysyms & actions count */
        KEY_LEFTALT,   DOWN, XKB_KEY_Alt_L,     NEXT,
        KEY_2,         BOTH, XKB_KEY_ecaron,    NEXT,
        KEY_LEFTALT,   UP,   XKB_KEY_Alt_L,     NEXT,
        KEY_RIGHTALT,  DOWN, XKB_KEY_Alt_R, XKB_KEY_ISO_Group_Shift, NEXT,
        KEY_2,         BOTH, XKB_KEY_2,         NEXT,
        KEY_RIGHTALT,  UP,   XKB_KEY_Alt_R, XKB_KEY_ISO_Group_Shift, NEXT,
        /* Super have different keysyms & actions count */
        KEY_LEFTMETA,  DOWN, XKB_KEY_Super_L, XKB_KEY_ISO_Group_Shift, NEXT,
        KEY_2,         BOTH, XKB_KEY_ecaron,    NEXT,
        KEY_LEFTMETA,  UP,   XKB_KEY_Super_L, XKB_KEY_ISO_Group_Shift, NEXT,
        KEY_RIGHTMETA, DOWN, XKB_KEY_Super_R, XKB_KEY_ISO_Group_Shift, NEXT,
        KEY_2,         BOTH, XKB_KEY_ecaron,    NEXT,
        KEY_RIGHTMETA, UP,   XKB_KEY_Super_R, XKB_KEY_ISO_Group_Shift, NEXT,
        KEY_2,         BOTH, XKB_KEY_2,        NEXT,
        /* Incompatible actions categories */
        KEY_RO,        DOWN, XKB_KEY_Control_L, XKB_KEY_Shift_L, NEXT,
        KEY_2,         BOTH, XKB_KEY_2,        NEXT,
        KEY_RO,        UP,   XKB_KEY_Control_L, XKB_KEY_Shift_L, NEXT,
        KEY_YEN,       DOWN, XKB_KEY_Control_L, XKB_KEY_Shift_L, NEXT,
        KEY_2,         BOTH, XKB_KEY_2,        NEXT,
        KEY_YEN,       UP,   XKB_KEY_Control_L, XKB_KEY_Shift_L, NEXT,
        /* Test various overrides */
        KEY_Z,         DOWN, XKB_KEY_Control_L, XKB_KEY_ISO_Group_Shift, NEXT,
        KEY_2,         BOTH, XKB_KEY_ecaron,   NEXT,
        KEY_Z,         UP,   XKB_KEY_y,        NEXT,
        KEY_X,         BOTH, XKB_KEY_x,        NEXT,
        KEY_C,         DOWN, XKB_KEY_NoSymbol, NEXT,
        KEY_2,         BOTH, XKB_KEY_at,       NEXT,
        KEY_C,         UP,   XKB_KEY_ampersand, NEXT,
        KEY_V,         DOWN, XKB_KEY_NoSymbol, NEXT,
        KEY_2,         BOTH, XKB_KEY_2,        NEXT,
        KEY_V,         UP,   XKB_KEY_NoSymbol, NEXT,
        KEY_B,         DOWN, XKB_KEY_ISO_Level3_Shift, XKB_KEY_ISO_Group_Shift, NEXT,
        KEY_2,         BOTH, XKB_KEY_at,       NEXT,
        KEY_B,         UP,   XKB_KEY_braceleft,NEXT,
        KEY_N,         DOWN, XKB_KEY_Control_L, NEXT,
        KEY_2,         BOTH, XKB_KEY_2,        NEXT,
        KEY_N,         UP,   XKB_KEY_Control_L, NEXT,
        KEY_M,         DOWN, XKB_KEY_ISO_Level3_Shift, XKB_KEY_ISO_Group_Shift, NEXT,
        KEY_2,         BOTH, XKB_KEY_at,       NEXT,
        KEY_M,         UP,   XKB_KEY_asciicircum, NEXT,
        /* Modifier_Map */
        KEY_Q,         DOWN, XKB_KEY_a, XKB_KEY_b, NEXT,
        KEY_2,         BOTH, XKB_KEY_at,       NEXT,
        KEY_Q,         UP,   XKB_KEY_a, XKB_KEY_b, NEXT,
        KEY_2,         BOTH, XKB_KEY_2,        FINISH));

    xkb_keymap_unref(keymap);

    /* TODO: This example is intended to make keyboard shortcuts use the first
     *       layout. However, this requires to be able to configure group redirect
     *       at the *keymap* level, then use ISO_First_Group and SetGroup(group=-4).
     *       Change the symbols and this test once this is merged. */
    keymap = test_compile_rules(context, "evdev", "pc104",
                                "awesome,cz", NULL, "grp:menu_toggle");
    assert(keymap);

    ctrl = xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_CTRL);

    kc = xkb_keymap_key_by_name(keymap, "LCTL");
    keysyms_count = xkb_keymap_key_get_syms_by_level(keymap, kc, first_layout, 0, &keysyms);
    assert(keysyms_count == 2);
    assert(keysyms[0] == XKB_KEY_Control_L);
    assert(keysyms[1] == XKB_KEY_ISO_Next_Group);

    state = xkb_state_new(keymap);
    assert(state);
    layout = xkb_state_key_get_layout(state, KEY_LEFTCTRL + EVDEV_OFFSET);
    assert(layout == 0);
    xkb_state_update_key(state, KEY_LEFTCTRL + EVDEV_OFFSET, XKB_KEY_DOWN);
    base_mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_DEPRESSED);
    assert(base_mods == (UINT32_C(1) << ctrl));
    layout = xkb_state_key_get_layout(state, XKB_KEY_2 + EVDEV_OFFSET);
    assert(layout == 1);
    xkb_state_update_key(state, KEY_LEFTCTRL + EVDEV_OFFSET, XKB_KEY_UP);
    base_mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_DEPRESSED);
    assert(base_mods == 0);
    layout = xkb_state_key_get_layout(state, XKB_KEY_2 + EVDEV_OFFSET);
    assert(layout == 0);
    xkb_state_update_key(state, KEY_COMPOSE + EVDEV_OFFSET, XKB_KEY_DOWN);
    xkb_state_update_key(state, KEY_COMPOSE + EVDEV_OFFSET, XKB_KEY_UP);
    layout = xkb_state_key_get_layout(state, XKB_KEY_2 + EVDEV_OFFSET);
    assert(layout == 1);
    xkb_state_update_key(state, KEY_LEFTCTRL + EVDEV_OFFSET, XKB_KEY_DOWN);
    base_mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_DEPRESSED);
    assert(base_mods == (UINT32_C(1) << ctrl));
    layout = xkb_state_key_get_layout(state, XKB_KEY_2 + EVDEV_OFFSET);
    assert(layout == 0);
    xkb_state_update_key(state, KEY_LEFTCTRL + EVDEV_OFFSET, XKB_KEY_UP);
    base_mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_DEPRESSED);
    assert(base_mods == 0);
    layout = xkb_state_key_get_layout(state, XKB_KEY_2 + EVDEV_OFFSET);
    assert(layout == 1);
    xkb_state_unref(state);

    assert(test_key_seq(keymap,
                        KEY_2,        BOTH, XKB_KEY_2,         NEXT,
                        KEY_LEFTCTRL, DOWN, XKB_KEY_Control_L, XKB_KEY_ISO_Next_Group, NEXT,
                        KEY_2,        BOTH, XKB_KEY_ecaron,    NEXT,
                        KEY_LEFTCTRL, UP,   XKB_KEY_Control_L, XKB_KEY_ISO_Next_Group, NEXT,
                        KEY_COMPOSE,  BOTH, XKB_KEY_ISO_Next_Group, NEXT,
                        KEY_2,        BOTH, XKB_KEY_ecaron,    NEXT,
                        KEY_LEFTCTRL, DOWN, XKB_KEY_Control_L, XKB_KEY_ISO_Next_Group, NEXT,
                        KEY_2,        BOTH, XKB_KEY_2,         NEXT,
                        KEY_LEFTCTRL, UP,   XKB_KEY_Control_L, XKB_KEY_ISO_Next_Group, NEXT,
                        KEY_2,        BOTH, XKB_KEY_ecaron,    FINISH));

    xkb_keymap_unref(keymap);
    xkb_context_unref(context);
}

int
main(void)
{
    test_init();

    test_garbage_key();
    test_keymap();
    test_no_extra_groups();
    test_numeric_keysyms();
    test_multiple_keysyms_per_level();
    test_multiple_actions_per_level();

    return 0;
}
