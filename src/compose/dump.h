#ifndef COMPOSE_DUMP_H
#define COMPOSE_DUMP_H

#include "config.h"

#include "table.h"

bool
print_compose_table_entry(FILE *file, struct xkb_compose_table_entry *entry);

bool
xkb_compose_table_dump(FILE *file, struct xkb_compose_table *table);

#endif
