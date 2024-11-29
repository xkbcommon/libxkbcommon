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
#if HAVE_ICU
#include <unicode/uchar.h>
#endif

#include "xkbcommon/xkbcommon.h"
#include "src/keysym.h"

static void
print_char_name(uint32_t cp)
{
#if HAVE_ICU
    char char_name[256];
    UErrorCode pErrorCode = 0;
    int32_t count = u_charName((UChar32)cp, U_CHAR_NAME_ALIAS, char_name,
                                sizeof(char_name), &pErrorCode);
    if (count <= 0) {
        count = u_charName((UChar32)cp, U_EXTENDED_CHAR_NAME,
                            char_name, sizeof(char_name), &pErrorCode);
    }
    if (count > 0) {
        printf(" %s", char_name);
    }
#endif
}

int
main(int argc, char **argv)
{
    bool explicit = !(argc > 1 && strcmp("all", argv[1]) == 0);
    int idx = 1 + !explicit;
    bool char_names = argc > idx && strcmp("names", argv[idx]) == 0;
#if !HAVE_ICU
    if (char_names)
        fprintf(stderr, "ERROR: names argument requires ICU.\n");
#endif
    struct xkb_keysym_iterator *iter = xkb_keysym_iterator_new(explicit);
    while (xkb_keysym_iterator_next(iter)) {
        xkb_keysym_t ks = xkb_keysym_iterator_get_keysym(iter);
        printf("0x%04x:\n", ks);
        char name[XKB_KEYSYM_NAME_MAX_SIZE];
        xkb_keysym_iterator_get_name(iter, name, sizeof(name));
        printf("  name: %s\n", name);
        // char utf8[7];
        // int count = xkb_keysym_to_utf8(ks, utf8, sizeof(utf8));
        uint32_t cp = xkb_keysym_to_utf32(ks);
        if (cp) {
            printf("  code point: 0x%04X", cp);
            if (char_names) {
                printf(" #");
                print_char_name(cp);
            }
            printf("\n");
        }
        xkb_keysym_t ks2;
        if ((ks2 = xkb_keysym_to_lower(ks)) != ks) {
            xkb_keysym_get_name(ks2, name, sizeof(name));
            printf("  lower: 0x%04x # %s", ks2, name);
            if (char_names) {
                cp = xkb_keysym_to_utf32(ks2);
                print_char_name(cp);
            }
            printf("\n");
        }
        if ((ks2 = xkb_keysym_to_upper(ks)) != ks) {
            xkb_keysym_get_name(ks2, name, sizeof(name));
            printf("  upper: 0x%04x # %s", ks2, name);
            if (char_names) {
                cp = xkb_keysym_to_utf32(ks2);
                print_char_name(cp);
            }
            printf("\n");
        }
    };
    xkb_keysym_iterator_unref(iter);
}
