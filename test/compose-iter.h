/*
 * Copyright © 2022 Ran Benita <ran@unusedvar.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef COMPOSE_LEGACY_ITER_H
#define COMPOSE_LEGACY_ITER_H

#include "config.h"
#include "src/compose/table.h"

/**
 * The iterator function type used by xkb_compose_table_for_each().
 */
typedef void
(*xkb_compose_table_iter_t)(struct xkb_compose_table_entry *entry,
                            void *data);

/**
 * Run a specified function for every valid entry in the table.
 *
 * The entries are returned in lexicographic order of the left-hand
 * side of entries. This does not correspond to the order in which
 * the entries appear in the Compose file.
 */
void
xkb_compose_table_for_each(struct xkb_compose_table *table,
                           xkb_compose_table_iter_t iter,
                           void *data);

#endif
