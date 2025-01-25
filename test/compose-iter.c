/*
 * Copyright © 2022 Ran Benita <ran@unusedvar.com>
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

#include "src/darray.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "xkbcommon/xkbcommon-compose.h"
#include "src/compose/escape.h"
#include "src/compose/parser.h"
#include "src/keysym.h"
#include "src/utils.h"
#include "test/compose-iter.h"

/* Reference implentation of Compose table traversal */
static void
for_each_helper(struct xkb_compose_table *table,
                xkb_compose_table_iter_t iter,
                void *data,
                xkb_keysym_t *syms,
                size_t nsyms,
                uint16_t p)
{
    if (!p) {
        return;
    }
    const struct compose_node *node = &darray_item(table->nodes, p);
    for_each_helper(table, iter, data, syms, nsyms, node->lokid);
    syms[nsyms++] = node->keysym;
    if (node->is_leaf) {
        struct xkb_compose_table_entry entry = {
            .sequence = syms,
            .sequence_length = nsyms,
            .keysym = node->leaf.keysym,
            .utf8 = &darray_item(table->utf8, node->leaf.utf8),
        };
        iter(&entry, data);
    } else {
        for_each_helper(table, iter, data, syms, nsyms, node->internal.eqkid);
    }
    nsyms--;
    for_each_helper(table, iter, data, syms, nsyms, node->hikid);
}

void
xkb_compose_table_for_each(struct xkb_compose_table *table,
                           xkb_compose_table_iter_t iter,
                           void *data)
{
    if (darray_size(table->nodes) <= 1) {
        return;
    }
    xkb_keysym_t syms[MAX_LHS_LEN];
    for_each_helper(table, iter, data, syms, 0, 1);
}
