#include "config.h"

#include "src/darray.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "xkbcommon/xkbcommon-compose.h"
#include "src/compose/dump.h"
#include "src/compose/parser.h"
#include "src/keysym.h"
#include "src/utils.h"
#include "test/compose-iter.h"

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

XKB_EXPORT void
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
            fprintf(file, "ERROR: Cannot escape the string: allocation error\n");
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
