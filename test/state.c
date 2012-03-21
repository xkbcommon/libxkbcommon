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
#include <X11/X.h>
#include <X11/Xdefs.h>
#include <X11/keysym.h>
#include <linux/input.h>
#include "xkbcommon/xkbcommon.h"
#include "xkbcomp/utils.h"
#include "XKBcommonint.h"

/* Offset between evdev keycodes (where KEY_ESCAPE is 1), and the evdev XKB
 * keycode set (where ESC is 9). */
#define EVDEV_OFFSET 8

int main(int argc, char *argv[])
{
    struct xkb_rule_names rmlvo;
    struct xkb_desc *xkb;
    struct xkb_state *state;
    int num_syms;
    xkb_keysym_t *syms;

    rmlvo.rules = "evdev";
    rmlvo.model = "pc104";
    rmlvo.layout = "us";
    rmlvo.variant = NULL;
    rmlvo.options = NULL;

    xkb = xkb_compile_keymap_from_rules(&rmlvo);

    if (!xkb) {
        fprintf(stderr, "Failed to compile keymap\n");
        exit(1);
    }

    state = xkb_state_new(xkb);
    assert(state->mods == 0);
    assert(state->group == 0);

    /* LCtrl down */
    xkb_state_update_key(state, KEY_LEFTCTRL + EVDEV_OFFSET, 1);
    assert(state->mods & ControlMask);

    /* LCtrl + RAlt down */
    xkb_state_update_key(state, KEY_RIGHTALT + EVDEV_OFFSET, 1);
    assert(state->mods & Mod1Mask);
    assert(state->locked_mods == 0);
    assert(state->latched_mods == 0);

    /* RAlt down */
    xkb_state_update_key(state, KEY_LEFTCTRL + EVDEV_OFFSET, 0);
    assert(!(state->mods & ControlMask) && (state->mods & Mod1Mask));

    /* none down */
    xkb_state_update_key(state, KEY_RIGHTALT + EVDEV_OFFSET, 0);
    assert(state->mods == 0);
    assert(state->group == 0);

    /* Caps locked */
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, 1);
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, 0);
    assert(state->mods & LockMask);
    assert(state->mods == state->locked_mods);
    num_syms = xkb_key_get_syms(state, KEY_Q + EVDEV_OFFSET, &syms);
    assert(num_syms == 1 && syms[0] == XK_Q);

    /* Caps unlocked */
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, 1);
    xkb_state_update_key(state, KEY_CAPSLOCK + EVDEV_OFFSET, 0);
    assert(state->mods == 0);
    num_syms = xkb_key_get_syms(state, KEY_Q + EVDEV_OFFSET, &syms);
    assert(num_syms == 1 && syms[0] == XK_q);

    xkb_state_unref(state);
    xkb_free_keymap(xkb);

    return 0;
}
