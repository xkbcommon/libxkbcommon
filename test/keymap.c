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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "test.h"

int
main(void)
{
    struct xkb_context *context = test_get_context(0);
    struct xkb_keymap *keymap;
    xkb_keycode_t kc;
    const char *keyname;

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

    xkb_keymap_unref(keymap);
    xkb_context_unref(context);
}
