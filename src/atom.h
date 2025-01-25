/*
 * Copyright © 2009 Dan Nicholson
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "utils.h"

typedef uint32_t xkb_atom_t;

#define XKB_ATOM_NONE 0

struct atom_table;

XKB_EXPORT_PRIVATE struct atom_table *
atom_table_new(void);

XKB_EXPORT_PRIVATE void
atom_table_free(struct atom_table *table);

XKB_EXPORT_PRIVATE xkb_atom_t
atom_intern(struct atom_table *table, const char *string, size_t len, bool add);

XKB_EXPORT_PRIVATE const char *
atom_text(struct atom_table *table, xkb_atom_t atom);
