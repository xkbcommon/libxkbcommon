/*
 * Copyright © 2022 Ran Benita <ran@unusedvar.com>
 * SPDX-License-Identifier: MIT
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
                uint32_t p)
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
