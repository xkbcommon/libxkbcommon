/*
 * Copyright © 2013,2021 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <assert.h>

#include "xkbcommon/xkbcommon.h"
#include "messages-codes.h"
#include "utils.h"
#include "constants.h"
#include "table.h"
#include "parser.h"
#include "paths.h"

static struct xkb_compose_table *
xkb_compose_table_new(struct xkb_context *ctx, const char *func,
                      const char *locale,
                      enum xkb_compose_format format,
                      enum xkb_compose_compile_flags flags)
{
    static const enum xkb_compose_compile_flags XKB_COMPOSE_COMPILE_FLAGS =
        XKB_COMPOSE_COMPILE_NO_FLAGS;

    if (flags & ~XKB_COMPOSE_COMPILE_FLAGS) {
        log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                "%s: unrecognized flags: %#x\n", func,
                (flags & ~XKB_COMPOSE_COMPILE_FLAGS));
        return NULL;
    }

    if (format != XKB_COMPOSE_FORMAT_TEXT_V1) {
        log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                "%s: unsupported compose format: %d\n", func, format);
        return NULL;
    }

    char *resolved_locale;
    struct xkb_compose_table *table;
    struct compose_node dummy = {0};

    resolved_locale = resolve_locale(ctx, locale);
    if (!resolved_locale)
        return NULL;

    table = calloc(1, sizeof(*table));
    if (!table) {
        free(resolved_locale);
        return NULL;
    }

    table->refcnt = 1;
    table->ctx = xkb_context_ref(ctx);

    table->locale = resolved_locale;
    table->format = format;
    table->flags = flags;

    darray_init(table->nodes);
    darray_init(table->utf8);

    dummy.keysym = XKB_KEY_NoSymbol;
    dummy.leaf.is_leaf = true;
    dummy.leaf.utf8 = 0;
    dummy.leaf.keysym = XKB_KEY_NoSymbol;
    darray_append(table->nodes, dummy);

    darray_append(table->utf8, '\0');

    return table;
}

struct xkb_compose_table *
xkb_compose_table_ref(struct xkb_compose_table *table)
{
    assert(table->refcnt > 0);
    table->refcnt++;
    return table;
}

void
xkb_compose_table_unref(struct xkb_compose_table *table)
{
    assert(!table || table->refcnt > 0);
    if (!table || --table->refcnt > 0)
        return;
    free(table->locale);
    darray_free(table->nodes);
    darray_free(table->utf8);
    xkb_context_unref(table->ctx);
    free(table);
}

struct xkb_compose_table *
xkb_compose_table_new_from_file(struct xkb_context *ctx,
                                FILE *file,
                                const char *locale,
                                enum xkb_compose_format format,
                                enum xkb_compose_compile_flags flags)
{
    struct xkb_compose_table * const table =
        xkb_compose_table_new(ctx, __func__, locale, format, flags);
    if (!table)
        return NULL;

    if (!parse_file(table, file, "(unknown file)")) {
        xkb_compose_table_unref(table);
        return NULL;
    }

    return table;
}

struct xkb_compose_table *
xkb_compose_table_new_from_buffer(struct xkb_context *ctx,
                                  const char *buffer, size_t length,
                                  const char *locale,
                                  enum xkb_compose_format format,
                                  enum xkb_compose_compile_flags flags)
{
    struct xkb_compose_table * const table =
        xkb_compose_table_new(ctx, __func__, locale, format, flags);
    if (!table)
        return NULL;

    if (!parse_string(table, buffer, length, "(input string)")) {
        xkb_compose_table_unref(table);
        return NULL;
    }

    return table;
}

struct xkb_compose_table *
xkb_compose_table_new_from_locale(struct xkb_context *ctx,
                                  const char *locale,
                                  enum xkb_compose_compile_flags flags)
{
    static const enum xkb_compose_format format = XKB_COMPOSE_FORMAT_TEXT_V1;
    struct xkb_compose_table * const table =
        xkb_compose_table_new(ctx, __func__, locale, format, flags);
    if (!table)
        return NULL;

    char *path = get_xcomposefile_path(ctx);
    FILE *file = open_file(path);
    if (file)
        goto found_path;
    free(path);

    path = get_xdg_xcompose_file_path(ctx);
    file = open_file(path);
    if (file)
        goto found_path;
    free(path);

    path = get_home_xcompose_file_path(ctx);
    file = open_file(path);
    if (file)
        goto found_path;
    free(path);

    path = get_locale_compose_file_path(ctx, table->locale);
    file = open_file(path);
    if (file)
        goto found_path;
    free(path);

    log_err(ctx, XKB_ERROR_INVALID_COMPOSE_LOCALE,
            "couldn't find a Compose file for locale \"%s\" (mapped to \"%s\")\n",
            locale, table->locale);
    xkb_compose_table_unref(table);
    return NULL;

found_path:
    {} /* Label followed by a declaration is a C23 extension */
    const bool ok = parse_file(table, file, path);
    fclose(file);
    if (!ok) {
        free(path);
        xkb_compose_table_unref(table);
        return NULL;
    }

    log_dbg(ctx, XKB_LOG_MESSAGE_NO_ID,
            "created compose table from locale %s with path %s\n",
            table->locale, path);

    free(path);
    return table;
}

const xkb_keysym_t *
xkb_compose_table_entry_sequence(struct xkb_compose_table_entry *entry,
                                 size_t *sequence_length)
{
    *sequence_length = entry->sequence_length;
    return entry->sequence;
}

xkb_keysym_t
xkb_compose_table_entry_keysym(struct xkb_compose_table_entry *entry)
{
    return entry->keysym;
}

const char *
xkb_compose_table_entry_utf8(struct xkb_compose_table_entry *entry)
{
    return entry->utf8;
}

#if MAX_COMPOSE_NODES_LOG2 > 31
#error "Cannot implement bit field xkb_compose_table_iterator_pending_node.offset"
#endif

struct xkb_compose_table_iterator_pending_node {
    uint32_t offset:31;
    bool processed:1;
};

struct xkb_compose_table_iterator {
    struct xkb_compose_table *table;
    /* Current entry */
    struct xkb_compose_table_entry entry;
    /* Stack of pending nodes to process */
    darray(struct xkb_compose_table_iterator_pending_node) pending_nodes;
};

struct xkb_compose_table_iterator *
xkb_compose_table_iterator_new(struct xkb_compose_table *table)
{
    struct xkb_compose_table_iterator *iter;
    xkb_keysym_t *sequence;

    iter = calloc(1, sizeof(*iter));
    if (!iter) {
        return NULL;
    }
    iter->table = xkb_compose_table_ref(table);
    sequence = calloc(COMPOSE_MAX_LHS_LEN, sizeof(xkb_keysym_t));
    if (!sequence) {
        free(iter);
        return NULL;
    }
    iter->entry.sequence = sequence;
    iter->entry.sequence_length = 0;

    darray_init(iter->pending_nodes);
    /* Short-circuit if table contains only the dummy entry */
    if (darray_size(table->nodes) == 1) {
        return iter;
    }
    /* Add root node */
    struct xkb_compose_table_iterator_pending_node pending = {
        .offset = 1,
        .processed = false
    };
    darray_append(iter->pending_nodes, pending);
    const struct compose_node *node = &darray_item(iter->table->nodes,
                                                   pending.offset);

    /* Find the first left-most node and store intermediate nodes as pending */
    while (node->lokid) {
        pending.offset = node->lokid;
        darray_append(iter->pending_nodes, pending);
        node = &darray_item(iter->table->nodes, pending.offset);
    };

    return iter;
}

void
xkb_compose_table_iterator_free(struct xkb_compose_table_iterator *iter)
{
    xkb_compose_table_unref(iter->table);
    darray_free(iter->pending_nodes);
    free(iter->entry.sequence);
    free(iter);
}

struct xkb_compose_table_entry *
xkb_compose_table_iterator_next(struct xkb_compose_table_iterator *iter)
{
    /* Traversal algorithm (simplified):
     * 1. Resume last pending node from the stack as the current pending node.
     * 2. If the node is not yet processed, go to 5.
     * 3. Remove node from the stack and remove last keysym from the entry.
     * 4. If there is a right arrow, set it as the current pending node
     *    (unprocessed) and go to 6; else go to 1.
     * 5. Follow down arrow: set the pending node as processed, then:
     *      a) if it is a leaf, complete the entry and return it.
     *      b) else append the child node to the stack and set it as the current
     *         pending node.
     * 6. Find the next left-most arrow and store intermediate pending nodes.
     * 7. Go to 5.
     */

    struct xkb_compose_table_iterator_pending_node *pending;
    const struct compose_node *node;

    /* Iterator is empty if there is no pending nodes */
    if (unlikely(darray_empty(iter->pending_nodes))) {
        return NULL;
    }

    /* Resume to the last element in the stack */
    pending = &darray_item(iter->pending_nodes,
                           darray_size(iter->pending_nodes) - 1);
    node = &darray_item(iter->table->nodes, pending->offset);

    struct xkb_compose_table_iterator_pending_node new = {
        .processed = false,
        .offset = 0
    };

    /* Remove processed leaves until an unprocessed right arrow or parent
     * is found, then update entry accordingly */
    while (pending->processed) {
        /* Remove last keysym */
        iter->entry.sequence_length--;
        if (node->hikid) {
            /* Follow right arrow: replace current pending node */
            pending->processed = false;
            pending->offset = node->hikid;
            /* Process child’s left arrow */
            goto node_left;
        } else {
            /* Remove processed node */
            darray_remove_last(iter->pending_nodes);
            if (unlikely(darray_empty(iter->pending_nodes))) {
                /* No more nodes to process */
                return NULL;
            }
            /* Get parent node */
            pending = &darray_item(iter->pending_nodes,
                                   darray_size(iter->pending_nodes) - 1);
            node = &darray_item(iter->table->nodes, pending->offset);
        }
    }

    while (1) {
        /* Follow down arrow */
        pending->processed = true;
        iter->entry.sequence[iter->entry.sequence_length] = node->keysym;
        iter->entry.sequence_length++;
        if (node->is_leaf) {
            /* Leaf: return entry */
            iter->entry.keysym = node->leaf.keysym;
            iter->entry.utf8 = &darray_item(iter->table->utf8, node->leaf.utf8);
            return &iter->entry;
        }
        /* Not a leaf: process child node */
        new.offset = node->internal.eqkid;
        darray_append(iter->pending_nodes, new);
        pending = &darray_item(iter->pending_nodes,
                               darray_size(iter->pending_nodes) - 1);
node_left:
        node = &darray_item(iter->table->nodes, pending->offset);
        /* Find the next left-most arrow and store intermediate pending nodes */
        while (node->lokid) {
            /* Follow left arrow */
            new.offset = node->lokid;
            darray_append(iter->pending_nodes, new);
            pending = &darray_item(iter->pending_nodes,
                                   darray_size(iter->pending_nodes) - 1);
            node = &darray_item(iter->table->nodes, new.offset);
        }
    }
}
