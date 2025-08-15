/*
 * Copyright © 2025 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <limits.h>

#include "context.h"
#include "darray.h"
#include "messages-codes.h"
#include "utils.h"
#include "xkbcommon/xkbcommon.h"
#include "file_iterator.h"
#include "xkbcomp-priv.h"
#include "include.h"

/* Same as `darray_append_string` but do count the final '\0' in the size */
#define darray_append_string0(arr, string) \
    darray_append_items((arr), (string), strlen(string) + 1)


const char *
xkb_file_type_name(enum xkb_file_type type)
{
    if (type > FILE_TYPE_KEYMAP)
        return "unknown";

    static const char *xkb_file_type_strings[_FILE_TYPE_NUM_ENTRIES] = {
        [FILE_TYPE_KEYCODES] = "keycodes",
        [FILE_TYPE_TYPES] = "types",
        [FILE_TYPE_COMPAT] = "compatibility",
        [FILE_TYPE_SYMBOLS] = "symbols",
        [FILE_TYPE_GEOMETRY] = "geometry",
        [FILE_TYPE_KEYMAP] = "keymap",
    };
    return xkb_file_type_strings[type];
}


const char *
xkb_merge_mode_name(enum merge_mode merge)
{
    if (merge >= _MERGE_MODE_NUM_ENTRIES)
        return "unknown";

    static const char *merge_mode_strings[_MERGE_MODE_NUM_ENTRIES] = {
        [MERGE_DEFAULT] = "default",
        [MERGE_AUGMENT] = "augment",
        [MERGE_OVERRIDE] = "override",
        [MERGE_REPLACE] = "replace",
    };
    return merge_mode_strings[merge];
}

const char *
xkb_map_flags_string_iter(unsigned int *index, enum xkb_map_flags flags)
{
    if (!flags)
        return NULL;

    static const struct {
        enum xkb_map_flags flag;
        const char *name;
    } names[] = {
        { MAP_IS_DEFAULT, "default" },
        { MAP_IS_PARTIAL, "partial" },
        { MAP_IS_HIDDEN, "hidden" },
        { MAP_HAS_ALPHANUMERIC, "alphanumeric" },
        { MAP_HAS_MODIFIER, "modifiers" },
        { MAP_HAS_KEYPAD, "keypad" },
        { MAP_HAS_FN, "fn" },
        { MAP_IS_ALTGR, "altgr" },
    };

    while (*index < ARRAY_SIZE(names)) {
        if (flags & names[*index].flag)
            return names[(*index)++].name;
        (*index)++;
    }

    return NULL;
};

void
xkb_file_section_init(struct xkb_file_section *section)
{
    darray_init(section->includes);
    darray_init(section->buf);
    darray_append(section->buf, '\0');
}

void
xkb_file_section_free(struct xkb_file_section *section)
{
    if (!section)
        return;

    darray_free(section->includes);
    darray_free(section->buf);
}

static bool
xkb_file_section_set_meta_data(struct xkb_context *ctx,
                               struct xkb_file_section *section,
                               const XkbFile *xkb_file)
{
    section->file_type = xkb_file->file_type;
    section->flags = xkb_file->flags;
    if (xkb_file->name) {
        darray_size_t idx = darray_size(section->buf);
        darray_append_string0(section->buf, xkb_file->name);
        section->name = idx;
    } else {
        section->name = 0;
    }
    return true;
}

/* Process a list of include statements */
static bool
xkb_file_section_append_includes(struct xkb_context *ctx,
                                 struct xkb_file_section *section,
                                 enum xkb_file_type file_type,
                                 IncludeStmt * include)
{
    for (IncludeStmt *stmt = include; stmt; stmt = stmt->next_incl) {
        char buf[PATH_MAX];
        XkbFile *xkb_file = ProcessIncludeFile(ctx, stmt, file_type, buf, sizeof(buf));
        if (xkb_file) {
            FreeXkbFile(xkb_file);
            const darray_size_t path = darray_size(section->buf);
            darray_append_string0(section->buf, buf);
            const darray_size_t file = darray_size(section->buf);
            darray_append_string0(section->buf, stmt->file);
            const darray_size_t section_name = (stmt->map)
                ? darray_size(section->buf)
                : 0;
            if (stmt->map) {
                darray_append_string0(section->buf, stmt->map);
            }
            const darray_size_t modifier = (stmt->modifier)
                ? darray_size(section->buf)
                : 0;
            if (stmt->modifier) {
                darray_append_string0(section->buf, stmt->map);
            }
            const struct xkb_file_include inc = {
                .merge = stmt->merge,
                .path = path,
                .file = file,
                .section = section_name,
                .modifier = modifier
            };
            darray_append(section->includes, inc);
        } else {
            log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                    "Cannot resolbe include: %s\n", stmt->stmt);
            return false;
        }
    };
    return true;
}

/* Process the AST of a section */
static bool
xkb_file_section_process(struct xkb_context *ctx,
                         struct xkb_file_section *section,
                         const XkbFile *xkb_file)
{
    bool ok = true;
    for (ParseCommon *stmt = xkb_file->defs; stmt; stmt = stmt->next) {
        if (stmt->type == STMT_INCLUDE) {
            ok = xkb_file_section_append_includes(ctx, section,
                                                  xkb_file->file_type,
                                                  (IncludeStmt *) stmt);
            if (!ok)
                break;
        }
    }
    return ok;
}

bool
xkb_file_section_parse(struct xkb_context *ctx,
                       enum xkb_keymap_format format,
                       enum xkb_keymap_compile_flags flags,
                       const char *path, const char *map,
                       unsigned int include_depth,
                       struct xkb_file_section *section)
{
    if (ExceedsIncludeMaxDepth(ctx, include_depth))
        return false;

    FILE *file = fopen(path, "rb");
    if (!file) {
        log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                "Cannot open file: %s\n", path);
        return false;
    }

    XkbFile *xkb_file = XkbParseFile(ctx, file, path, map);
    fclose(file);
    if (!xkb_file) {
        log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                "Cannot parse map \"%s\" in file: %s\n",
                (map ? map : "(no map)"), path);
        return false;
    }

    const bool ok = (xkb_file_section_set_meta_data(ctx, section, xkb_file) &&
                     xkb_file_section_process(ctx, section, xkb_file));

    FreeXkbFile(xkb_file);
    return ok;
}

struct xkb_file_iterator *
xkb_file_iterator_new(struct xkb_context *ctx,
                      enum xkb_keymap_format format,
                      enum xkb_keymap_compile_flags flags,
                      const char *path, const char *map,
                      const char *string, size_t length)
{
    struct xkb_file_iterator * const iter = calloc(1, sizeof(*iter));
    if (!iter) {
        log_err(ctx, XKB_ERROR_ALLOCATION_ERROR,
                "Cannot allocate file iterator\n");
        return NULL;
    }

    iter->ctx = ctx;
    iter->path = path;
    iter->map = map;
    xkb_file_section_init(&iter->section);

    if (!XkbParseStringInit(ctx, &iter->scanner, string, length, path, NULL)) {
        xkb_file_iterator_free(iter);
        return NULL;
    }

    return iter;
}

void
xkb_file_iterator_free(struct xkb_file_iterator *iter)
{
    if (!iter)
        return;

    xkb_file_section_free(&iter->section);
    FreeXkbFile(iter->pending_xkb_file);
    free(iter);
}

bool
xkb_file_iterator_next(struct xkb_file_iterator *iter,
                       const struct xkb_file_section **section)
{
    if (iter->finished) {
        *section = NULL;
        return true;
    }

    /* Parse next section in the file */
    XkbFile *xkb_file = iter->pending_xkb_file;

    if (xkb_file) {
        /* We are parsing the components of a keymap */
        if (iter->pending_section) {
            /* Parse next component */
            xkb_file = iter->pending_section;
            goto parse_components;
        } else {
            /* No more components, try next keymap */
            FreeXkbFile(iter->pending_xkb_file);
            xkb_file = iter->pending_xkb_file = NULL;
        }
    }

    if (!XkbParseStringNext(iter->ctx, &iter->scanner, iter->map, &xkb_file)) {
        log_err(iter->ctx, XKB_LOG_MESSAGE_NO_ID,
                "Error while parsing section in file: %s\n", iter->path);
        return false;
    } else if (!xkb_file) {
        /* No more sections */
        iter->finished = true;
        *section = NULL;
        return true;
    }

parse_components:
    /* Reset section */
    darray_resize(iter->section.includes, 0);
    /* 1st index is reserved for empty strings */
    darray_resize(iter->section.buf, 1);

    /* Meta data */
    if (!xkb_file_section_set_meta_data(iter->ctx, &iter->section, xkb_file))
        goto error;

    /* Return current section */
    *section = &iter->section;

    if (xkb_file->file_type == FILE_TYPE_KEYMAP) {
        /*
         * If it’s a keymap, then stop here.
         * Next iteration will process its components.
         */
        iter->pending_xkb_file = xkb_file;
        iter->pending_section = (XkbFile *) xkb_file->defs;
        iter->map = NULL;
        return true;
    } else if (iter->map) {
        iter->finished = true;
    }

    /* Collect include statements of current section */
    if (!xkb_file_section_process(iter->ctx, &iter->section, xkb_file)) {
        goto error;
    } else if (iter->pending_section) {
        /* Next component */
        iter->pending_section = (XkbFile *) xkb_file->common.next;
    } else {
        /* No more component, free the file containing the components */
        FreeXkbFile(xkb_file);
    }

    return true;

error:
    FreeXkbFile(xkb_file);
    *section = NULL;
    return false;
}

const char *
xkb_file_section_get_string(const struct xkb_file_section *section, darray_size_t idx)
{
    return &darray_item(section->buf, idx);
}
