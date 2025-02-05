/*
 * Copyright © 2009 Dan Nicholson
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef uint32_t xkb_atom_t;

#define XKB_ATOM_NONE 0

struct atom_table;

struct atom_table *
atom_table_new(void);

void
atom_table_free(struct atom_table *table);

xkb_atom_t
atom_intern(struct atom_table *table, const char *string, size_t len, bool add);

const char *
atom_text(struct atom_table *table, xkb_atom_t atom);
