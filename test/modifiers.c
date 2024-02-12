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

#include "test.h"
#include "keymap.h"
#include "evdev-scancodes.h"

// Standard real modifier masks
#define ShiftMask   (1 << 0)
#define LockMask    (1 << 1)
#define ControlMask (1 << 2)
#define Mod1Mask    (1 << 3)
#define Mod2Mask    (1 << 4)
#define Mod3Mask    (1 << 5)
#define Mod4Mask    (1 << 6)
#define Mod5Mask    (1 << 7)
#define Mod6Mask    (1 << 8)
#define Mod7Mask    (1 << 9)
#define Mod8Mask    (1 << 10)
#define Mod9Mask    (1 << 11)
#define Mod10Mask   (1 << 12)
#define HighestRealMask Mod10Mask
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
test_real_mods(void)
{
    struct xkb_context *context = test_get_context(0);
    struct xkb_keymap *keymap;
    const struct xkb_key *key;
    xkb_keycode_t keycode;

    keymap = test_compile_file(context, "keymaps/real-modifiers.xkb");
    assert(keymap);

    keycode = xkb_keymap_key_by_name(keymap, "AD01");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == Mod5Mask);
    /* Test that VMod5 is the first defined virtual modifier */
    assert(key->vmodmap == HighestRealMask << 1);

    keycode = xkb_keymap_key_by_name(keymap, "AD02");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == Mod6Mask);
    /* Test that VMod5 is the second defined virtual modifier */
    assert(key->vmodmap == HighestRealMask << 2);

    /* Test that we get the expected keysym for each level of KEY_O (<AD09>).
     * Most levels require support of the real modifiers Mod6-Mod10. */
    assert(test_key_seq(keymap,
                        /* Level 1 */
                        KEY_O, BOTH, XKB_KEY_1, NEXT,
                        /* Level 2 */
                        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
                        KEY_O, BOTH, XKB_KEY_2, NEXT,
                        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
                        /* Level 3 (VMod5 = Mod5) */
                        KEY_Q, DOWN, XKB_KEY_Q, NEXT,
                        KEY_O, BOTH, XKB_KEY_3, NEXT,
                        KEY_Q, UP,   XKB_KEY_Q, NEXT,
                        /* Level 4 (VMod6 = Mod6) */
                        KEY_W, DOWN, XKB_KEY_W, NEXT,
                        KEY_O, BOTH, XKB_KEY_4, NEXT,
                        /* Level 5 (VMod6 + Shift = Mod6 + Shift) */
                        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
                        KEY_O, BOTH, XKB_KEY_5, NEXT,
                        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
                        KEY_W, UP,   XKB_KEY_W, NEXT,
                        /* Level 6 (VMod7 = Mod7) */
                        KEY_E, DOWN, XKB_KEY_E, NEXT,
                        KEY_O, BOTH, XKB_KEY_6, NEXT,
                        KEY_E, UP,   XKB_KEY_E, NEXT,
                        /* Level 7 (VMod8 = Mod8) */
                        KEY_R, DOWN, XKB_KEY_R, NEXT,
                        KEY_O, BOTH, XKB_KEY_7, NEXT,
                        KEY_R, UP,   XKB_KEY_R, NEXT,
                        /* Level 8 (VMod9 = Mod9) */
                        KEY_T, DOWN, XKB_KEY_T, NEXT,
                        KEY_O, BOTH, XKB_KEY_8, NEXT,
                        KEY_T, UP,   XKB_KEY_T, NEXT,
                        /* Level 9 (VMod10 = Mod10) */
                        KEY_Y, DOWN, XKB_KEY_Y, NEXT,
                        KEY_O, BOTH, XKB_KEY_9, NEXT,
                        KEY_Y, UP,   XKB_KEY_Y, NEXT,
                        /* Level 10 using KEY_U (VMod11 = Mod5 + Mod6) */
                        KEY_U, DOWN, XKB_KEY_U, NEXT,
                        KEY_O, BOTH, XKB_KEY_A, NEXT,
                        KEY_U, UP,   XKB_KEY_U, NEXT,
                        /* Level 10 using KEY_I (VMod11 = Mod5 + Mod6) */
                        KEY_I, DOWN, XKB_KEY_I, NEXT,
                        KEY_O, BOTH, XKB_KEY_A, NEXT,
                        KEY_I, UP,   XKB_KEY_I, NEXT,
                        /* Level 10 using KEY_Q + KEY_W
                         * VMod5 + VMod6 = Mod5 + Mod6 = VMod11 */
                        KEY_Q, DOWN, XKB_KEY_Q, NEXT,
                        KEY_W, DOWN, XKB_KEY_W, NEXT,
                        KEY_O, BOTH, XKB_KEY_A, NEXT,
                        KEY_W, UP,   XKB_KEY_W, NEXT,
                        KEY_Q, UP,   XKB_KEY_Q, NEXT,
                        /* Invalid level, fallback to base level */
                        KEY_W, DOWN, XKB_KEY_W, NEXT,
                        KEY_E, DOWN, XKB_KEY_E, NEXT,
                        KEY_O, BOTH, XKB_KEY_1, NEXT,
                        KEY_E, UP,   XKB_KEY_E, NEXT,
                        KEY_W, UP,   XKB_KEY_W, FINISH));

    xkb_keymap_unref(keymap);
    xkb_context_unref(context);
}

int
main(void)
{
    test_modmap_none();
    test_real_mods();

    return 0;
}
