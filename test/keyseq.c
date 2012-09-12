/*
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
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
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include <linux/input.h>

#include "xkbcommon/xkbcommon.h"
#include "test.h"

enum {
    DOWN,
    UP,
    BOTH,
    NEXT,
    FINISH,
};

#define EVDEV_OFFSET 8

/*
 * Test a sequence of keysyms, resulting from a sequence of key presses,
 * against the keysyms they're supposed to generate.
 *
 * - Each test runs with a clean state.
 * - Each line in the test is made up of:
 *   + A keycode, given as a KEY_* from linux/input.h.
 *   + A direction - DOWN for press, UP for release, BOTH for
 *     immediate press + release.
 *   + A sequence of keysyms that should result from this keypress.
 *
 * The vararg format is:
 * <KEY_*>  <DOWN | UP | BOTH>  <XKB_KEY_* (zero or more)>  <NEXT | FINISH>
 *
 * See below for examples.
 */
static int
test_key_seq(struct xkb_keymap *keymap, ...)
{
    struct xkb_state *state;

    va_list ap;
    xkb_keycode_t kc;
    int op;
    xkb_keysym_t keysym;

    const xkb_keysym_t *syms;
    unsigned int nsyms, i;
    char ksbuf[64];

    state = xkb_state_new(keymap);
    assert(state);

    va_start(ap, keymap);

    for (;;) {
        kc = va_arg(ap, int) + EVDEV_OFFSET;
        op = va_arg(ap, int);

        nsyms = xkb_key_get_syms(state, kc, &syms);
        fprintf(stderr, "got %d syms for key 0x%x: [", nsyms, kc);

        if (op == DOWN || op == BOTH)
            xkb_state_update_key(state, kc, XKB_KEY_DOWN);
        if (op == UP || op == BOTH)
            xkb_state_update_key(state, kc, XKB_KEY_UP);

        for (i = 0; i < nsyms; i++) {
            keysym = va_arg(ap, int);
            xkb_keysym_get_name(syms[i], ksbuf, sizeof(ksbuf));
            fprintf(stderr, "%s%s", (i != 0) ? ", " : "", ksbuf);

            if (keysym == FINISH || keysym == NEXT) {
                xkb_keysym_get_name(syms[i], ksbuf, sizeof(ksbuf));
                fprintf(stderr, "Did not expect keysym: %s.\n", ksbuf);
                goto fail;
            }

            if (keysym != syms[i]) {
                xkb_keysym_get_name(keysym, ksbuf, sizeof(ksbuf));
                fprintf(stderr, "Expected keysym: %s. ", ksbuf);;
                xkb_keysym_get_name(syms[i], ksbuf, sizeof(ksbuf));
                fprintf(stderr, "Got keysym: %s.\n", ksbuf);;
                goto fail;
            }
        }

        fprintf(stderr, "]\n");

        keysym = va_arg(ap, int);
        if (keysym == NEXT)
            continue;
        if (keysym == FINISH)
            break;

        xkb_keysym_get_name(keysym, ksbuf, sizeof(ksbuf));
        fprintf(stderr, "Expected keysym: %s. Didn't get it.\n", ksbuf);
        goto fail;
    }

    va_end(ap);
    xkb_state_unref(state);
    return 1;

fail:
    va_end(ap);
    xkb_state_unref(state);
    return 0;
}

int
main(void)
{
    struct xkb_context *ctx = test_get_context();
    struct xkb_keymap *keymap;

    assert(ctx);
    keymap = test_compile_rules(ctx, "evdev", "evdev",
                                "us,il,ru", ",,phonetic",
                                "grp:alt_shift_toggle,grp:menu_toggle");
    assert(keymap);

    assert(test_key_seq(keymap,
                        KEY_H,  BOTH,  XKB_KEY_h,  NEXT,
                        KEY_E,  BOTH,  XKB_KEY_e,  NEXT,
                        KEY_L,  BOTH,  XKB_KEY_l,  NEXT,
                        KEY_L,  BOTH,  XKB_KEY_l,  NEXT,
                        KEY_O,  BOTH,  XKB_KEY_o,  FINISH));

    assert(test_key_seq(keymap,
                        KEY_H,          BOTH,  XKB_KEY_h,        NEXT,
                        KEY_LEFTSHIFT,  DOWN,  XKB_KEY_Shift_L,  NEXT,
                        KEY_E,          BOTH,  XKB_KEY_E,        NEXT,
                        KEY_L,          BOTH,  XKB_KEY_L,        NEXT,
                        KEY_LEFTSHIFT,  UP,    XKB_KEY_Shift_L,  NEXT,
                        KEY_L,          BOTH,  XKB_KEY_l,        NEXT,
                        KEY_O,          BOTH,  XKB_KEY_o,        FINISH));

    /* Base modifier cleared on key release... */
    assert(test_key_seq(keymap,
                        KEY_H,          BOTH,  XKB_KEY_h,        NEXT,
                        KEY_LEFTSHIFT,  DOWN,  XKB_KEY_Shift_L,  NEXT,
                        KEY_E,          BOTH,  XKB_KEY_E,        NEXT,
                        KEY_L,          BOTH,  XKB_KEY_L,        NEXT,
                        KEY_LEFTSHIFT,  DOWN,  XKB_KEY_Shift_L,  NEXT,
                        KEY_L,          BOTH,  XKB_KEY_L,        NEXT,
                        KEY_O,          BOTH,  XKB_KEY_O,        FINISH));

    /* ... But only by the keycode that set it. */
    assert(test_key_seq(keymap,
                        KEY_H,           BOTH,  XKB_KEY_h,        NEXT,
                        KEY_LEFTSHIFT,   DOWN,  XKB_KEY_Shift_L,  NEXT,
                        KEY_E,           BOTH,  XKB_KEY_E,        NEXT,
                        KEY_L,           BOTH,  XKB_KEY_L,        NEXT,
                        KEY_RIGHTSHIFT,  UP,    XKB_KEY_Shift_R,  NEXT,
                        KEY_L,           BOTH,  XKB_KEY_L,        NEXT,
                        KEY_O,           BOTH,  XKB_KEY_O,        FINISH));

    /*
     * A base modifier should only be cleared when no other key affecting
     * the modifier is down.
     */
    assert(test_key_seq(keymap,
                        KEY_H,           BOTH,  XKB_KEY_h,        NEXT,
                        KEY_LEFTSHIFT,   DOWN,  XKB_KEY_Shift_L,  NEXT,
                        KEY_E,           BOTH,  XKB_KEY_E,        NEXT,
                        KEY_RIGHTSHIFT,  DOWN,  XKB_KEY_Shift_R,  NEXT,
                        KEY_L,           BOTH,  XKB_KEY_L,        NEXT,
                        KEY_RIGHTSHIFT,  UP,    XKB_KEY_Shift_R,  NEXT,
                        KEY_L,           BOTH,  XKB_KEY_L,        NEXT,
                        KEY_LEFTSHIFT,   UP,    XKB_KEY_Shift_L,  NEXT,
                        KEY_O,           BOTH,  XKB_KEY_o,        FINISH));

    /* Group switching / locking. */
    assert(test_key_seq(keymap,
                        KEY_H,        BOTH,  XKB_KEY_h,               NEXT,
                        KEY_E,        BOTH,  XKB_KEY_e,               NEXT,
                        KEY_COMPOSE,  BOTH,  XKB_KEY_ISO_Next_Group,  NEXT,
                        KEY_K,        BOTH,  XKB_KEY_hebrew_lamed,    NEXT,
                        KEY_F,        BOTH,  XKB_KEY_hebrew_kaph,     NEXT,
                        KEY_COMPOSE,  BOTH,  XKB_KEY_ISO_Next_Group,  NEXT,
                        KEY_COMPOSE,  BOTH,  XKB_KEY_ISO_Next_Group,  NEXT,
                        KEY_O,        BOTH,  XKB_KEY_o,               FINISH));

    assert(test_key_seq(keymap,
                        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L,        NEXT,
                        KEY_LEFTALT,   DOWN, XKB_KEY_ISO_Next_Group, NEXT,
                        KEY_LEFTALT,   UP,   XKB_KEY_ISO_Next_Group, NEXT,
                        KEY_LEFTSHIFT, UP,   XKB_KEY_Shift_L,        FINISH));

    assert(test_key_seq(keymap,
                        KEY_LEFTALT,   DOWN, XKB_KEY_Alt_L,          NEXT,
                        KEY_LEFTSHIFT, DOWN, XKB_KEY_ISO_Next_Group, NEXT,
                        KEY_LEFTSHIFT, UP,   XKB_KEY_ISO_Next_Group, NEXT,
                        KEY_LEFTALT,   UP,   XKB_KEY_Alt_L,          FINISH));

    /* Locked modifiers. */
    assert(test_key_seq(keymap,
                        KEY_CAPSLOCK,  BOTH,  XKB_KEY_Caps_Lock,  NEXT,
                        KEY_H,         BOTH,  XKB_KEY_H,          NEXT,
                        KEY_E,         BOTH,  XKB_KEY_E,          NEXT,
                        KEY_L,         BOTH,  XKB_KEY_L,          NEXT,
                        KEY_L,         BOTH,  XKB_KEY_L,          NEXT,
                        KEY_O,         BOTH,  XKB_KEY_O,          FINISH));

    assert(test_key_seq(keymap,
                        KEY_H,         BOTH,  XKB_KEY_h,          NEXT,
                        KEY_E,         BOTH,  XKB_KEY_e,          NEXT,
                        KEY_CAPSLOCK,  BOTH,  XKB_KEY_Caps_Lock,  NEXT,
                        KEY_L,         BOTH,  XKB_KEY_L,          NEXT,
                        KEY_L,         BOTH,  XKB_KEY_L,          NEXT,
                        KEY_CAPSLOCK,  BOTH,  XKB_KEY_Caps_Lock,  NEXT,
                        KEY_O,         BOTH,  XKB_KEY_o,          FINISH));

    assert(test_key_seq(keymap,
                        KEY_H,         BOTH,  XKB_KEY_h,          NEXT,
                        KEY_CAPSLOCK,  DOWN,  XKB_KEY_Caps_Lock,  NEXT,
                        KEY_E,         BOTH,  XKB_KEY_E,          NEXT,
                        KEY_L,         BOTH,  XKB_KEY_L,          NEXT,
                        KEY_L,         BOTH,  XKB_KEY_L,          NEXT,
                        KEY_CAPSLOCK,  UP,    XKB_KEY_Caps_Lock,  NEXT,
                        KEY_O,         BOTH,  XKB_KEY_O,          FINISH));

    assert(test_key_seq(keymap,
                        KEY_H,         BOTH,  XKB_KEY_h,          NEXT,
                        KEY_E,         BOTH,  XKB_KEY_e,          NEXT,
                        KEY_CAPSLOCK,  UP,    XKB_KEY_Caps_Lock,  NEXT,
                        KEY_L,         BOTH,  XKB_KEY_l,          NEXT,
                        KEY_L,         BOTH,  XKB_KEY_l,          NEXT,
                        KEY_O,         BOTH,  XKB_KEY_o,          FINISH));

    /*
     * A key release affecting a locked modifier should clear it
     * regardless of the key press.
     */
    /* assert(test_key_seq(keymap, */
    /*                     KEY_H,         BOTH,  XKB_KEY_h,          NEXT, */
    /*                     KEY_CAPSLOCK,  DOWN,  XKB_KEY_Caps_Lock,  NEXT, */
    /*                     KEY_E,         BOTH,  XKB_KEY_E,          NEXT, */
    /*                     KEY_L,         BOTH,  XKB_KEY_L,          NEXT, */
    /*                     KEY_CAPSLOCK,  UP,    XKB_KEY_Caps_Lock,  NEXT, */
    /*                     KEY_L,         BOTH,  XKB_KEY_L,          NEXT, */
    /*                     KEY_CAPSLOCK,  UP,    XKB_KEY_Caps_Lock,  NEXT, */
    /*                     KEY_O,         BOTH,  XKB_KEY_o,          FINISH)); */

    /* Simple Num Lock sanity check. */
    assert(test_key_seq(keymap,
                        KEY_KP1,      BOTH,  XKB_KEY_KP_End,    NEXT,
                        KEY_NUMLOCK,  BOTH,  XKB_KEY_Num_Lock,  NEXT,
                        KEY_KP1,      BOTH,  XKB_KEY_KP_1,      NEXT,
                        KEY_KP2,      BOTH,  XKB_KEY_KP_2,      NEXT,
                        KEY_NUMLOCK,  BOTH,  XKB_KEY_Num_Lock,  NEXT,
                        KEY_KP2,      BOTH,  XKB_KEY_KP_Down,   FINISH));

    /* Test that the aliases in the ru(phonetic) symbols map work. */
    assert(test_key_seq(keymap,
                        KEY_COMPOSE,     BOTH,  XKB_KEY_ISO_Next_Group,  NEXT,
                        KEY_COMPOSE,     BOTH,  XKB_KEY_ISO_Next_Group,  NEXT,
                        KEY_1,           BOTH,  XKB_KEY_1,               NEXT,
                        KEY_Q,           BOTH,  XKB_KEY_Cyrillic_ya,     NEXT,
                        KEY_LEFTSHIFT,   DOWN,  XKB_KEY_Shift_L,         NEXT,
                        KEY_1,           BOTH,  XKB_KEY_exclam,          NEXT,
                        KEY_Q,           BOTH,  XKB_KEY_Cyrillic_YA,     NEXT,
                        KEY_LEFTSHIFT,   UP,    XKB_KEY_Shift_L,         NEXT,
                        KEY_V,           BOTH,  XKB_KEY_Cyrillic_zhe,    NEXT,
                        KEY_CAPSLOCK,    BOTH,  XKB_KEY_Caps_Lock,       NEXT,
                        KEY_1,           BOTH,  XKB_KEY_1,               NEXT,
                        KEY_V,           BOTH,  XKB_KEY_Cyrillic_ZHE,    NEXT,
                        KEY_RIGHTSHIFT,  DOWN,  XKB_KEY_Shift_R,         NEXT,
                        KEY_V,           BOTH,  XKB_KEY_Cyrillic_zhe,    NEXT,
                        KEY_RIGHTSHIFT,  UP,    XKB_KEY_Shift_R,         NEXT,
                        KEY_V,           BOTH,  XKB_KEY_Cyrillic_ZHE,    FINISH));

    xkb_map_unref(keymap);
    xkb_context_unref(ctx);
    return 0;
}
