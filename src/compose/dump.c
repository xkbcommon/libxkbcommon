/*
 * Copyright © 2021 Ran Benita <ran@unusedvar.com>
 * Copyright © 2023 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include "src/darray.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "xkbcommon/xkbcommon-compose.h"
#include "parser.h"
#include "escape.h"
#include "dump.h"
#include "src/keysym.h"

bool
print_compose_table_entry(FILE *file, struct xkb_compose_table_entry *entry)
{
    size_t nsyms;
    const xkb_keysym_t *syms = xkb_compose_table_entry_sequence(entry, &nsyms);
    char buf[XKB_KEYSYM_NAME_MAX_SIZE];
    for (size_t i = 0; i < nsyms; i++) {
        xkb_keysym_get_name(syms[i], buf, sizeof(buf));
        fprintf(file, "<%s>", buf);
        if (i + 1 < nsyms) {
            fprintf(file, " ");
        }
    }
    fprintf(file, " : ");
    const char *utf8 = xkb_compose_table_entry_utf8(entry);
    if (*utf8 != '\0') {
        char *escaped = escape_utf8_string_literal(utf8);
        if (!escaped) {
            fprintf(stderr, "ERROR: Cannot escape the string: allocation error\n");
            return false;
        } else {
            fprintf(file, " \"%s\"", escaped);
            free(escaped);
        }
    }
    const xkb_keysym_t keysym = xkb_compose_table_entry_keysym(entry);
    if (keysym != XKB_KEY_NoSymbol) {
        xkb_keysym_get_name(keysym, buf, sizeof(buf));
        fprintf(file, " %s", buf);
    }
    fprintf(file, "\n");
    return true;
}

bool
xkb_compose_table_dump(FILE *file, struct xkb_compose_table *table)
{
    struct xkb_compose_table_entry *entry;
    struct xkb_compose_table_iterator *iter = xkb_compose_table_iterator_new(table);

    if (!iter)
        return false;

    bool ok = true;
    while ((entry = xkb_compose_table_iterator_next(iter))) {
        if (!print_compose_table_entry(file, entry)) {
            ok = false;
            break;
        }
    }

    xkb_compose_table_iterator_free(iter);
    return ok;
}
