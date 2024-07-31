#ifndef COMPOSE_LEGACY_ITER_H
#define COMPOSE_LEGACY_ITER_H

#include "config.h"
#include "src/compose/table.h"

/**
 * The iterator function type used by xkb_compose_table_for_each().
 *
 * @sa xkb_compose_table_for_each
 * @memberof xkb_compose
 * @since 1.6.0
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
 *
 * @memberof xkb_compose_table
 * @since 1.6.0
 */
void
xkb_compose_table_for_each(struct xkb_compose_table *table,
                           xkb_compose_table_iter_t iter,
                           void *data);

bool
print_compose_table_entry(FILE *file, struct xkb_compose_table_entry *entry);

#endif
