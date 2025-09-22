/*
 * For MIT-open-group:
 * Copyright 1987, 1998  The Open Group
 *
 * For LicenseRef-digital-equipment-corporation:
 * Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.
 *
 * For HPND:
 * Copyright 1994 by Silicon Graphics Computer Systems, Inc.
 *
 * SPDX-License-Identifier: MIT-open-group AND LicenseRef-digital-equipment-corporation AND HPND
 */

#include "config.h"

#include <assert.h>
#include <string.h>

#include "atom.h"
#include "darray.h"
#include "utils.h"

/* FNV-1a (http://www.isthe.com/chongo/tech/comp/fnv/). */
static inline uint32_t
hash_buf(const char *string, size_t len)
{
    uint32_t hash = UINT32_C(2166136261);
    for (size_t i = 0; i < (len + 1) / 2; i++) {
        hash ^= (uint8_t) string[i];
        hash *= 0x01000193;
        hash ^= (uint8_t) string[len - 1 - i];
        hash *= 0x01000193;
    }
    return hash;
}

/*
 * The atom table is an insert-only linear probing hash table
 * mapping strings to atoms. Another array maps the atoms to
 * strings. The atom value is the position in the strings array.
 */
struct atom_table {
    xkb_atom_t *index;
    size_t index_size;
    darray(char *) strings;
};

struct atom_table *
atom_table_new(void)
{
    struct atom_table *table = calloc(1, sizeof(*table));
    if (!table)
        return NULL;

    darray_init(table->strings);
    darray_append(table->strings, NULL);
    table->index_size = 4;
    table->index = calloc(table->index_size, sizeof(*table->index));

    return table;
}

void
atom_table_free(struct atom_table *table)
{
    if (!table)
        return;

    char **string;
    darray_foreach(string, table->strings)
        free(*string);
    darray_free(table->strings);
    free(table->index);
    free(table);
}

darray_size_t
atom_table_size(struct atom_table *table)
{
    return darray_size(table->strings);
}

const char *
atom_text(struct atom_table *table, xkb_atom_t atom)
{
    assert(atom < darray_size(table->strings));
    return darray_item(table->strings, atom);
}

xkb_atom_t
atom_intern(struct atom_table *table, const char *string, size_t len, bool add)
{
    /* len(string) > 0.8 * index_size */
    if (darray_size(table->strings) > (table->index_size / 5) * 4) {
        table->index_size *= 2;
        table->index = realloc(table->index, table->index_size * sizeof(*table->index));
        memset(table->index, 0, table->index_size * sizeof(*table->index));
        for (darray_size_t j = 1; j < darray_size(table->strings); j++) {
            const char *s = darray_item(table->strings, j);
            uint32_t hash = hash_buf(s, strlen(s));
            for (size_t i = 0; i < table->index_size; i++) {
                size_t index_pos = (hash + i) & (table->index_size - 1);
                if (index_pos == 0)
                    continue;

                xkb_atom_t atom = table->index[index_pos];
                if (atom == XKB_ATOM_NONE) {
                    table->index[index_pos] = j;
                    break;
                }
            }
        }
    }

    uint32_t hash = hash_buf(string, len);
    for (size_t i = 0; i < table->index_size; i++) {
        size_t index_pos = (hash + i) & (table->index_size - 1);
        if (index_pos == 0)
            continue;

        xkb_atom_t existing_atom = table->index[index_pos];
        if (existing_atom == XKB_ATOM_NONE) {
            if (add) {
                xkb_atom_t new_atom = darray_size(table->strings);
                darray_append(table->strings, strndup(string, len));
                table->index[index_pos] = new_atom;
                return new_atom;
            } else {
                return XKB_ATOM_NONE;
            }
        }

        const char *existing_value = darray_item(table->strings, existing_atom);
        if (strncmp(existing_value, string, len) == 0 && existing_value[len] == '\0')
            return existing_atom;
    }

    assert(!"couldn't find an empty slot during probing");
    return XKB_ATOM_NONE;
}
