/*
 * Copyright © 2023 Pierre Le Marre <dev@wismill.eu>
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
 */

#include "config.h"

#include <assert.h>
#include <stdlib.h>

#include "evdev-scancodes.h"
#include "test.h"
#include "keymap.h"

/* Standard real modifiers indexes */
#define ShiftIndex   0
#define LockIndex    1
#define ControlIndex 2
#define Mod1Index    3
#define Mod2Index    4
#define Mod3Index    5
#define Mod4Index    6
#define Mod5Index    7

/* Standard real modifier masks */
#define ShiftMask   (1 << ShiftIndex)
#define LockMask    (1 << LockIndex)
#define ControlMask (1 << ControlIndex)
#define Mod1Mask    (1 << Mod1Index)
#define Mod2Mask    (1 << Mod2Index)
#define Mod3Mask    (1 << Mod3Index)
#define Mod4Mask    (1 << Mod4Index)
#define Mod5Mask    (1 << Mod5Index)
#define NoModifier  0

static bool
test_real_mod(struct xkb_keymap *keymap, const char* name,
              xkb_mod_index_t idx, xkb_mod_mask_t mapping)
{
    return xkb_keymap_mod_get_index(keymap, name) == idx &&
           (keymap->mods.mods[idx].type == MOD_REAL) &&
           mapping == (1u << idx);
}

static bool
test_virtual_mod(struct xkb_keymap *keymap, const char* name,
                 xkb_mod_index_t idx, xkb_mod_mask_t mapping)
{
    return xkb_keymap_mod_get_index(keymap, name) == idx &&
           (keymap->mods.mods[idx].type == MOD_VIRT) &&
           mapping == keymap->mods.mods[idx].mapping;
}

/* Check that the provided modifier names work */
static void
test_modifiers_names(struct xkb_context *context)
{
    struct xkb_keymap *keymap;

    keymap = test_compile_rules(context, "evdev", NULL, NULL, NULL, NULL);
    assert(keymap);

    /* Real modifiers
     * The indexes and masks are fixed and always valid */
    assert(test_real_mod(keymap, XKB_MOD_NAME_SHIFT, ShiftIndex, ShiftMask));
    assert(test_real_mod(keymap, XKB_MOD_NAME_CAPS, LockIndex, LockMask));
    assert(test_real_mod(keymap, XKB_MOD_NAME_CTRL, ControlIndex, ControlMask));
    assert(test_real_mod(keymap, XKB_MOD_NAME_MOD1, Mod1Index, Mod1Mask));
    assert(test_real_mod(keymap, XKB_MOD_NAME_MOD2, Mod2Index, Mod2Mask));
    assert(test_real_mod(keymap, XKB_MOD_NAME_MOD3, Mod3Index, Mod3Mask));
    assert(test_real_mod(keymap, XKB_MOD_NAME_MOD4, Mod4Index, Mod4Mask));
    assert(test_real_mod(keymap, XKB_MOD_NAME_MOD5, Mod5Index, Mod5Mask));

    /* Usual virtual mods mappings */
    assert(test_real_mod(keymap, XKB_MOD_NAME_ALT,  Mod1Index, Mod1Mask));
    assert(test_real_mod(keymap, XKB_MOD_NAME_NUM,  Mod2Index, Mod2Mask));
    assert(test_real_mod(keymap, XKB_MOD_NAME_LOGO, Mod4Index, Mod4Mask));

    /* Virtual modifiers
     * The indexes depends on the keymap files */
    assert(test_virtual_mod(keymap, XKB_VMOD_NAME_ALT,    Mod5Index + 2,  Mod1Mask));
    assert(test_virtual_mod(keymap, XKB_VMOD_NAME_META,   Mod5Index + 11, Mod1Mask));
    assert(test_virtual_mod(keymap, XKB_VMOD_NAME_NUM,    Mod5Index + 1,  Mod2Mask));
    assert(test_virtual_mod(keymap, XKB_VMOD_NAME_SUPER,  Mod5Index + 12, Mod4Mask));
    assert(test_virtual_mod(keymap, XKB_VMOD_NAME_HYPER,  Mod5Index + 13, Mod4Mask));
    assert(test_virtual_mod(keymap, XKB_VMOD_NAME_LEVEL3, Mod5Index + 3,  Mod5Mask));
    assert(test_virtual_mod(keymap, XKB_VMOD_NAME_SCROLL, Mod5Index + 8,  0));
    /* TODO: current xkeyboard-config maps LevelFive to Nod3 by default */
    assert(test_virtual_mod(keymap, XKB_VMOD_NAME_LEVEL5, Mod5Index + 9,  0));
    /* Legacy stuff, removed from xkeyboard-config */
    assert(keymap->mods.num_mods == 21);
    assert(test_virtual_mod(keymap, "LAlt",     Mod5Index + 4,  0));
    assert(test_virtual_mod(keymap, "RAlt",     Mod5Index + 5,  0));
    assert(test_virtual_mod(keymap, "LControl", Mod5Index + 7,  0));
    assert(test_virtual_mod(keymap, "RControl", Mod5Index + 6,  0));
    assert(test_virtual_mod(keymap, "AltGr",    Mod5Index + 10, Mod5Mask));

    xkb_keymap_unref(keymap);
}

static void
test_modmap_none(struct xkb_context *context)
{
    struct xkb_keymap *keymap;
    const struct xkb_key *key;
    xkb_keycode_t keycode;

    keymap = test_compile_file(context, "keymaps/modmap-none.xkb");
    assert(keymap);

    keycode = xkb_keymap_key_by_name(keymap, "LVL3");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == NoModifier);

    keycode = xkb_keymap_key_by_name(keymap, "LFSH");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == NoModifier);

    keycode = xkb_keymap_key_by_name(keymap, "RTSH");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == NoModifier);

    keycode = xkb_keymap_key_by_name(keymap, "LWIN");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == Mod4Mask);

    keycode = xkb_keymap_key_by_name(keymap, "RWIN");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == Mod4Mask);

    keycode = xkb_keymap_key_by_name(keymap, "LCTL");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == ControlMask);

    keycode = xkb_keymap_key_by_name(keymap, "RCTL");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == ControlMask);

    keycode = xkb_keymap_key_by_name(keymap, "LALT");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == Mod1Mask);

    keycode = xkb_keymap_key_by_name(keymap, "RALT");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == (Mod2Mask | Mod5Mask));

    keycode = xkb_keymap_key_by_name(keymap, "CAPS");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == LockMask);

    keycode = xkb_keymap_key_by_name(keymap, "AD01");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == Mod1Mask);

    keycode = xkb_keymap_key_by_name(keymap, "AD02");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == NoModifier);

    keycode = xkb_keymap_key_by_name(keymap, "AD03");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == NoModifier);

    keycode = xkb_keymap_key_by_name(keymap, "AD04");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == Mod1Mask);

    keycode = xkb_keymap_key_by_name(keymap, "AD05");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == Mod2Mask);

    keycode = xkb_keymap_key_by_name(keymap, "AD06");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == Mod3Mask);

    keycode = xkb_keymap_key_by_name(keymap, "AD07");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == Mod1Mask);

    keycode = xkb_keymap_key_by_name(keymap, "AD08");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == Mod2Mask);

    keycode = xkb_keymap_key_by_name(keymap, "AD09");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == Mod3Mask);

    xkb_keymap_unref(keymap);
}

static void
test_pure_virtual_modifiers(struct xkb_context *context)
{
    struct xkb_keymap *keymap;

    /* Test definition of >20 pure virtual modifiers.
     * We superate the X11 limit of 16 virtual modifiers. */
    keymap = test_compile_file(context, "keymaps/pure-virtual-mods.xkb");
    assert(keymap);

    assert(test_key_seq(keymap,
                        KEY_W,          BOTH,  XKB_KEY_w,        NEXT,
                        KEY_A,          DOWN,  XKB_KEY_a,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_a,        NEXT,
                        KEY_A,          UP,    XKB_KEY_a,        NEXT,
                        KEY_B,          DOWN,  XKB_KEY_b,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_b,        NEXT,
                        KEY_B,          UP,    XKB_KEY_b,        NEXT,
                        KEY_C,          DOWN,  XKB_KEY_c,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_c,        NEXT,
                        KEY_C,          UP,    XKB_KEY_c,        NEXT,
                        KEY_D,          DOWN,  XKB_KEY_d,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_d,        NEXT,
                        KEY_D,          UP,    XKB_KEY_d,        NEXT,
                        KEY_E,          DOWN,  XKB_KEY_e,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_e,        NEXT,
                        KEY_E,          UP,    XKB_KEY_e,        NEXT,
                        KEY_F,          DOWN,  XKB_KEY_f,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_f,        NEXT,
                        KEY_F,          UP,    XKB_KEY_f,        NEXT,
                        KEY_G,          DOWN,  XKB_KEY_g,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_g,        NEXT,
                        KEY_G,          UP,    XKB_KEY_g,        NEXT,
                        KEY_H,          DOWN,  XKB_KEY_h,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_h,        NEXT,
                        KEY_H,          UP,    XKB_KEY_h,        NEXT,
                        KEY_I,          DOWN,  XKB_KEY_i,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_i,        NEXT,
                        KEY_I,          UP,    XKB_KEY_i,        NEXT,
                        KEY_J,          DOWN,  XKB_KEY_j,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_j,        NEXT,
                        KEY_J,          UP,    XKB_KEY_j,        NEXT,
                        KEY_K,          DOWN,  XKB_KEY_k,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_k,        NEXT,
                        KEY_K,          UP,    XKB_KEY_k,        NEXT,
                        KEY_L,          DOWN,  XKB_KEY_l,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_l,        NEXT,
                        KEY_L,          UP,    XKB_KEY_l,        NEXT,
                        KEY_M,          DOWN,  XKB_KEY_m,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_m,        NEXT,
                        KEY_M,          UP,    XKB_KEY_m,        NEXT,
                        KEY_N,          DOWN,  XKB_KEY_n,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_n,        NEXT,
                        KEY_N,          UP,    XKB_KEY_n,        NEXT,
                        KEY_O,          DOWN,  XKB_KEY_o,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_o,        NEXT,
                        KEY_O,          UP,    XKB_KEY_o,        NEXT,
                        KEY_P,          DOWN,  XKB_KEY_p,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_p,        NEXT,
                        KEY_P,          UP,    XKB_KEY_p,        NEXT,
                        KEY_Q,          DOWN,  XKB_KEY_q,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_q,        NEXT,
                        KEY_Q,          UP,    XKB_KEY_q,        NEXT,
                        KEY_R,          DOWN,  XKB_KEY_r,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_r,        NEXT,
                        KEY_R,          UP,    XKB_KEY_r,        NEXT,
                        KEY_S,          DOWN,  XKB_KEY_s,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_s,        NEXT,
                        KEY_S,          UP,    XKB_KEY_s,        NEXT,
                        KEY_T,          DOWN,  XKB_KEY_t,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_t,        NEXT,
                        KEY_T,          UP,    XKB_KEY_t,        NEXT,
                        KEY_U,          DOWN,  XKB_KEY_u,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_u,        NEXT,
                        KEY_U,          UP,    XKB_KEY_u,        NEXT,
                        KEY_V,          DOWN,  XKB_KEY_v,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_v,        NEXT,
                        KEY_LEFTSHIFT,  DOWN,  XKB_KEY_Shift_L,  NEXT,
                        KEY_W,          BOTH,  XKB_KEY_V,        NEXT,
                        KEY_LEFTSHIFT,  UP,    XKB_KEY_Shift_L,  NEXT,
                        KEY_V,          UP,    XKB_KEY_v,        NEXT,
                        KEY_A,          DOWN,  XKB_KEY_a,        NEXT,
                        KEY_S,          DOWN,  XKB_KEY_s,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_1,        NEXT,
                        KEY_RIGHTALT,   DOWN,  XKB_KEY_ISO_Level3_Shift, NEXT,
                        KEY_W,          BOTH,  XKB_KEY_4,        NEXT,
                        KEY_S,          UP,    XKB_KEY_s,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_3,        NEXT,
                        KEY_RIGHTALT,   UP,    XKB_KEY_ISO_Level3_Shift, NEXT,
                        KEY_Q,          DOWN,  XKB_KEY_q,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_2,        NEXT,
                        KEY_Q,          UP,    XKB_KEY_q,        NEXT,
                        KEY_B,          DOWN,  XKB_KEY_b,        NEXT,
                        KEY_C,          DOWN,  XKB_KEY_c,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_5,        NEXT,
                        KEY_C,          UP,    XKB_KEY_c,        NEXT,
                        KEY_B,          UP,    XKB_KEY_b,        NEXT,
                        KEY_A,          UP,    XKB_KEY_a,        NEXT,
                        KEY_Y,          BOTH,  XKB_KEY_y,        FINISH));
    xkb_keymap_unref(keymap);

    /* Test invalid interpret using a virtual modifier */
    const char keymap_str[] =
        "xkb_keymap {"
        "  xkb_keycodes { include \"evdev\" };"
        "  xkb_types { include \"complete\" };"
        "  xkb_compat { include \"complete+basic(invalid-pure-virtual-modifiers)\" };"
        "  xkb_symbols { include \"pc(pc105-pure-virtual-modifiers)\" };"
        "};";
    keymap = test_compile_string(context, keymap_str);
    assert(!keymap);
}

int
main(void)
{
    test_init();

    struct xkb_context *context = test_get_context(CONTEXT_NO_FLAG);
    assert(context);

    test_modmap_none(context);
    test_modifiers_names(context);
    test_pure_virtual_modifiers(context);

    xkb_context_unref(context);

    return 0;
}
