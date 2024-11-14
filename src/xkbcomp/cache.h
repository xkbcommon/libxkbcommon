/*
 * Copyright © 2024 Pierre Le Marre <dev@wismill.eu>
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

#ifndef XKBCOMP_CACHE_H
#define XKBCOMP_CACHE_H

#include <search.h>
#include <stdbool.h>

#include "xkbcommon/xkbcommon.h"
#include "src/darray.h"
#include "src/atom.h"
#include "ast.h"

#define XKB_KEYMAP_CACHE_SIZE 100000

struct xkb_keymap_cache_entry {
    char *key;
    XkbFile *data;
};

struct xkb_keymap_cache {
    struct hsearch_data index;
    darray(struct xkb_keymap_cache_entry) data;
};

struct xkb_keymap_cache*
xkb_keymap_cache_new(void);

void
xkb_keymap_cache_free(struct xkb_keymap_cache *cache);

bool
xkb_keymap_cache_add(struct xkb_keymap_cache *cache, char *key, XkbFile *data);

XkbFile*
xkb_keymap_cache_search(struct xkb_keymap_cache *cache, char *key);

#endif
