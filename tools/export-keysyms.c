/*
 * Copyright © 2024 Pierre Le Marre
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

#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "xkbcommon/xkbcommon.h"
#include "src/keysym.h"

int
main(int argc, char **argv)
{
    struct xkb_keysym_iterator *iter = xkb_keysym_iterator_new(true);
    while (xkb_keysym_iterator_next(iter)) {
        xkb_keysym_t ks = xkb_keysym_iterator_get_keysym(iter);
        printf("0x%04x:\n", ks);
        char name[XKB_KEYSYM_NAME_MAX_SIZE];
        xkb_keysym_iterator_get_name(iter, name, sizeof(name));
        printf("  name: %s\n", name);
        // char utf8[7];
        // count = xkb_keysym_to_utf8(ks, utf8, sizeof(utf8));
        uint32_t cp = xkb_keysym_to_utf32(ks);
        if (cp)
            printf("  code point: 0x%04X\n", cp);
        xkb_keysym_t ks2;
        if ((ks2 = xkb_keysym_to_lower(ks)) != ks) {
            xkb_keysym_get_name(ks2, name, sizeof(name));
            printf("  lower: 0x%04x # %s\n", ks2, name);
        }
        if ((ks2 = xkb_keysym_to_upper(ks)) != ks) {
            xkb_keysym_get_name(ks2, name, sizeof(name));
            printf("  upper: 0x%04x # %s\n", ks2, name);
        }
    };
    xkb_keysym_iterator_unref(iter);
}
