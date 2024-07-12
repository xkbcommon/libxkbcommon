/*
 * Copyright 1985, 1987, 1990, 1998  The Open Group
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the names of the authors or their
 * institutions shall not be used in advertising or otherwise to promote the
 * sale, use or other dealings in this Software without prior written
 * authorization from the authors.
 */

/*
 * Copyright © 2009 Dan Nicholson
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

#ifndef KEYSYM_H
#define KEYSYM_H

#include <stdbool.h>
#include "xkbcommon/xkbcommon.h"

/*
 * NOTE: this is not defined in xkbcommon.h, because if we did, it may add
 * overhead for library user: when handling keysyms they would also need to
 * check min keysym when previously there was no reason to.
 */
/** Minimum keysym value */
#define XKB_KEYSYM_MIN            0x00000000
/** Minimum keysym value assigned */
#define XKB_KEYSYM_MIN_ASSIGNED   ((xkb_keysym_t)0x00000000)
/** Maximum keysym value assigned */
#define XKB_KEYSYM_MAX_ASSIGNED   0x1008ffb8
/** Minimum keysym value with explicit name */
#define XKB_KEYSYM_MIN_EXPLICIT   0x00000000
/** Maximum keysym value with explicit name */
#define XKB_KEYSYM_MAX_EXPLICIT   0x1008ffb8
/** Count of keysym value with explicit name */
#define XKB_KEYSYM_COUNT_EXPLICIT 2446
/** Offset to use when converting a Unicode code point to a keysym */
#define XKB_KEYSYM_UNICODE_OFFSET 0x01000000
/** Minimum Unicode keysym. NOTE: code points in 0..0xff cannot be converted. */
#define XKB_KEYSYM_UNICODE_MIN    0x01000100
/** Maximum Unicode keysym, correspoding to the maximum Unicode code point */
#define XKB_KEYSYM_UNICODE_MAX    0x0110ffff
/** Maximum keysym name length */
#define XKB_KEYSYM_NAME_MAX_SIZE  27
/** Maximum bytes to encode the Unicode representation of a keysym in UTF-8:
 * 4 bytes + NULL-terminating byte */
#define XKB_KEYSYM_UTF8_MAX_SIZE  5

bool
xkb_keysym_is_assigned(xkb_keysym_t ks);

struct xkb_keysym_iterator;

struct xkb_keysym_iterator*
xkb_keysym_iterator_new(bool explicit);

struct xkb_keysym_iterator*
xkb_keysym_iterator_unref(struct xkb_keysym_iterator *iter);

bool
xkb_keysym_iterator_next(struct xkb_keysym_iterator *iter);

xkb_keysym_t
xkb_keysym_iterator_get_keysym(struct xkb_keysym_iterator *iter);

int
xkb_keysym_iterator_get_name(struct xkb_keysym_iterator *iter,
                             char *buffer, size_t size);

bool
xkb_keysym_iterator_is_explicitly_named(struct xkb_keysym_iterator *iter);

bool
xkb_keysym_is_lower(xkb_keysym_t keysym);

bool
xkb_keysym_is_upper(xkb_keysym_t keysym);

bool
xkb_keysym_is_keypad(xkb_keysym_t keysym);

bool
xkb_keysym_is_modifier(xkb_keysym_t keysym);

#endif
