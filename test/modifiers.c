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

// Standard real modifier masks
#define ShiftMask   (1 << 0)
#define LockMask    (1 << 1)
#define ControlMask (1 << 2)
#define Mod1Mask    (1 << 3)
#define Mod2Mask    (1 << 4)
#define Mod3Mask    (1 << 5)
#define Mod4Mask    (1 << 6)
#define Mod5Mask    (1 << 7)
#define NoModifier  0

static void
test_modmap_none(void)
{
    struct xkb_context *context = test_get_context(0);
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
    xkb_context_unref(context);
}

static void
test_pure_virtual_modifiers(void)
{
    struct xkb_context *context = test_get_context(0);
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
    xkb_context_unref(context);
}

int
main(void)
{
    test_init();

    test_modmap_none();
    test_pure_virtual_modifiers();

    return 0;
}
