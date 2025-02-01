/*
 * Copyright © 2021 Ran Benita <ran@unusedvar.com>
 * Copyright © 2023 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#ifndef COMPOSE_DUMP_H
#define COMPOSE_DUMP_H

#include "config.h"

#include "table.h"

bool
print_compose_table_entry(FILE *file, struct xkb_compose_table_entry *entry);

bool
xkb_compose_table_dump(FILE *file, struct xkb_compose_table *table);

#endif
