/*
 * Copyright © 2025 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <limits.h>
#include <string.h>

#include "xkbcommon/xkbcommon.h"
#include "context.h"
#include "darray.h"
#include "messages-codes.h"
#include "utils.h"
#include "utils-paths.h"
#include "keymap-file-iterator.h"
#include "xkbcomp-priv.h"
#include "include.h"
#include "xkbcomp/ast.h"

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

/* Stateful lookup of map flags names */
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

FILE *
xkb_resolve_file(struct xkb_context *ctx,
                 enum xkb_file_type file_type,
                 const char *path, const char *map,
                 char *resolved_path, size_t resolved_path_size,
                 char *resolved_map, size_t resolved_map_size)
{
    unsigned int offset = 0;
    FILE *file = NULL; /* Exact match */
    FILE *candidate = NULL; /* Weak match */
    const size_t path_len = strlen(path);

    const bool absolute_path = is_absolute_path(path);
    if (absolute_path) {
        /* Absolute path: no need for lookup in XKB paths */
        file = fopen(path, "rb");
    } else {
        /* Relative path: lookup the first XKB path */
        file = FindFileInXkbPath(ctx, "(unknown)", path, path_len, file_type,
                                 resolved_path, resolved_path_size, &offset,
                                 true);
    }

    while (file) {
        XkbFile * const xkb_file = XkbParseFile(ctx, file, path, map);
        if (xkb_file) {
            if (file_type < _FILE_TYPE_NUM_ENTRIES &&
                xkb_file->file_type != file_type) {
                log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                        "File of wrong type (expected %s, got %s); "
                        "file \"%s\" ignored\n",
                        xkb_file_type_to_string(file_type),
                        xkb_file_type_to_string(xkb_file->file_type),
                        (absolute_path ? path : resolved_path));
                goto invalid_file;
            } else if (map || (xkb_file->flags && MAP_IS_DEFAULT)) {
                /*
                 * Exact match: explicit map name or explicit default map.
                 * Lookup stops in this iteration.
                 */
                if (!strcpy_safe(resolved_map, resolved_map_size,
                                 (xkb_file->name ? xkb_file->name : ""))) {
                    FreeXkbFile(xkb_file);
                    goto error;
                }
            } else if (!candidate) {
                /*
                 * Weak match: looking for an explicit default map, but found
                 * only the first *implicit* default map (e.g. first map of the
                 * file). Use as fallback, but keep looking for an exact match.
                 */
                candidate = file;
                if (!strcpy_safe(resolved_map, resolved_map_size,
                                 (xkb_file->name ? xkb_file->name : ""))) {
                    FreeXkbFile(xkb_file);
                    goto error;
                }
            } else {
                /*
                 * Weak match, but we already have a previous candidate.
                 * Keep looking for an exact match.
                 */
                goto invalid_file;
            }
        } else {
invalid_file:
            fclose(file);
            file = NULL;
        }

        FreeXkbFile(xkb_file);

        if (file != NULL || absolute_path) {
            /* Exact match or absolute path with no further file to search */
            break;
        }

        offset++;
        file = FindFileInXkbPath(ctx, "(unknown)",
                                 path, path_len, file_type,
                                 resolved_path, resolved_path_size,
                                 &offset, true);
    }

    if (!file) {
        /* No exact match: use weak match, if any */
        file = candidate;
    } else if (candidate) {
        /* Found exact match: discard weak match, if any */
        fclose(candidate);
    }

    if (absolute_path && file &&
        !strcpy_safe(resolved_path, resolved_path_size, path)) {
        goto error;
    }

    return file;

error:
    log_err(ctx, XKB_ERROR_INSUFFICIENT_BUFFER_SIZE,
            "Cannot copy resolved path or section\n");
    fclose(file);
    return NULL;
}

void
xkb_file_section_init(struct xkb_file_section *section)
{
    darray_init(section->include_groups);
    darray_init(section->includes);
    darray_init(section->buffer);
    darray_append(section->buffer, '\0');
}

static void
xkb_file_section_reset(struct xkb_file_section *section)
{
    darray_size(section->include_groups) = 0;
    darray_size(section->includes) = 0;
    darray_size(section->buffer) = 1; /* keep the dummy string */
}

void
xkb_file_section_free(struct xkb_file_section *section)
{
    if (!section)
        return;

    darray_free(section->include_groups);
    darray_free(section->includes);
    darray_free(section->buffer);
}

static bool
xkb_file_section_set_meta_data(struct xkb_context *ctx,
                               struct xkb_file_section *section,
                               const XkbFile *xkb_file)
{
    section->file_type = xkb_file->file_type;
    section->flags = xkb_file->flags;
    if (xkb_file->name) {
        darray_size_t idx = darray_size(section->buffer);
        darray_append_string0(section->buffer, xkb_file->name);
        section->name = idx;
    } else {
        section->name = 0;
    }
    return true;
}

/** Process a list of include statements */
static bool
xkb_file_section_append_includes(struct xkb_context *ctx,
                                 enum xkb_file_iterator_flags flags,
                                 const char *section_path,
                                 struct xkb_file_section *section,
                                 enum xkb_file_type file_type,
                                 IncludeStmt * include)
{
    struct xkb_file_include_group *group = NULL;
    /* Note: this will flatten statements such as `include "pc+de"` */
    for (IncludeStmt *stmt = include; stmt; stmt = stmt->next_incl) {
        char buf[PATH_MAX];
        /* Parse the included file to check the include validity */
        XkbFile *xkb_file = ProcessIncludeFile(ctx, stmt, file_type, buf, sizeof(buf));
        const bool valid = (xkb_file != NULL);
        if (valid || !(flags & XKB_FILE_ITERATOR_FAIL_ON_INCLUDE_ERROR)) {
            /* Collect the strings of the statement properties */

            const darray_size_t path = darray_size(section->buffer);
            darray_append_string0(section->buffer, buf);

            const darray_size_t file = darray_size(section->buffer);
            darray_append_string0(section->buffer, stmt->file);

            const darray_size_t section_name =
                (stmt->map != NULL || (valid && xkb_file->name))
                    ? darray_size(section->buffer)
                    : 0;
            if (section_name) {
                darray_append_string0(section->buffer,
                                      (stmt->map) ? stmt->map : xkb_file->name);
            }

            const darray_size_t modifier = (stmt->modifier)
                ? darray_size(section->buffer)
                : 0;
            if (modifier) {
                darray_append_string0(section->buffer, stmt->modifier);
            }

            const enum xkb_map_flags section_flags = (valid)
                ? xkb_file->flags
                : 0;

            /* Create and append the include statement */
            const struct xkb_file_include inc = {
                .valid = valid,
                .explicit_section = (stmt->map != NULL),
                .merge = stmt->merge,
                .path = path,
                .file = file,
                .section = section_name,
                .modifier = modifier,
                .flags = section_flags
            };
            const darray_size_t idx = darray_size(section->includes);
            darray_append(section->includes, inc);

            /* Update include group */
            if (group == NULL) {
                const darray_size_t group_idx =
                    darray_size(section->include_groups);
                darray_append(
                    section->include_groups,
                    (struct xkb_file_include_group) {.start = idx, .end = idx}
                );
                group = &darray_item(section->include_groups, group_idx);
            } else {
                group->end = idx;
            }

            FreeXkbFile(xkb_file);
        } else {
            const char * const name =
                xkb_file_section_get_string(section, section->name);
            log_err(ctx, XKB_ERROR_INCLUDED_FILE_NOT_FOUND,
                    "%s include failure in: %s%s%s%s\n",
                    xkb_file_type_name(file_type), section_path,
                    (section->name ? " (section: \"": ""), name,
                    (section->name ? "\")": ""));
            FreeXkbFile(xkb_file);
            return false;
        }
    };
    return true;
}

/* Process the AST of a section */
static bool
xkb_file_section_process(struct xkb_context *ctx,
                         enum xkb_file_iterator_flags flags,
                         const char *path,
                         struct xkb_file_section *section,
                         const XkbFile *xkb_file)
{
    bool ok = true;
    for (ParseCommon *stmt = xkb_file->defs; stmt; stmt = stmt->next) {
        if (stmt->type == STMT_INCLUDE) {
            ok = xkb_file_section_append_includes(ctx, flags, path, section,
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
                       enum xkb_file_iterator_flags iterator_flags,
                       enum xkb_keymap_format format,
                       enum xkb_keymap_compile_flags compile_flags,
                       unsigned int include_depth,
                       const char *path, const char *map,
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

    xkb_file_section_reset(section);
    const bool no_includes = iterator_flags & XKB_FILE_ITERATOR_NO_INCLUDES;
    const bool ok = (
        xkb_file_section_set_meta_data(ctx, section, xkb_file) &&
        (no_includes ||
         xkb_file_section_process(ctx, iterator_flags, path, section, xkb_file))
    );
    FreeXkbFile(xkb_file);
    return ok;
}

struct xkb_file_iterator *
xkb_file_iterator_new_from_buffer(struct xkb_context *ctx,
                                  enum xkb_file_iterator_flags iterator_flags,
                                  enum xkb_keymap_format format,
                                  enum xkb_keymap_compile_flags compile_flags,
                                  const char *path, const char *map,
                                  enum xkb_file_type file_type,
                                  const char *string, size_t length)
{
    struct xkb_file_iterator * const iter = calloc(1, sizeof(*iter));
    if (!iter) {
        log_err(ctx, XKB_ERROR_ALLOCATION_ERROR,
                "Cannot allocate file iterator\n");
        return NULL;
    }

    iter->flags = iterator_flags;
    iter->ctx = ctx;
    iter->path = path;
    iter->map = map;
    iter->type = file_type;
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

next:
    { /* C11 compatibility: label cannot be followed by a declaration */ }
    /* Parse next section in the file */
    XkbFile *xkb_file = NULL;

    if (iter->pending_xkb_file) {
        /* We are parsing the components of a keymap */
        if (iter->pending_section) {
            /* Parse next component */
            xkb_file = iter->pending_section;
            goto parse_components;
        } else {
            /* No more components, try next keymap */
            FreeXkbFile(iter->pending_xkb_file);
            iter->pending_xkb_file = NULL;
        }
    }

    if (!XkbParseStringNext(iter->ctx, &iter->scanner, iter->map, &xkb_file)) {
        log_err(iter->ctx, XKB_LOG_MESSAGE_NO_ID,
                "Error while parsing section in file: %s\n", iter->path);
        goto error;
    } else if (!xkb_file) {
        /* No more sections */
        iter->finished = true;
        *section = NULL;
        return true;
    }

parse_components:
    /* Reset section */
    xkb_file_section_reset(&iter->section);

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
    } else {
        if (iter->type != FILE_TYPE_INVALID &&
            xkb_file->file_type != iter->type) {
            if (iter->pending_xkb_file) {
                /* Within a keymap: filter out component */
                iter->pending_section = (XkbFile *) xkb_file->common.next;
                goto next;
            } else {
                /* Component-specific file: type mismatch */
                log_err(iter->ctx, XKB_LOG_MESSAGE_NO_ID,
                        "File type mismatch: %s, section: %s\n",
                        iter->path,
                        (xkb_file->name ? xkb_file->name : "(no name)"));
                goto error;
            }
        }
        if (iter->map) {
            iter->finished = true;
        }
    }

    /* Collect include statements of current section */
    const bool process_includes = !(iter->flags & XKB_FILE_ITERATOR_NO_INCLUDES);
    if (process_includes &&
        !xkb_file_section_process(iter->ctx, iter->flags, iter->path,
                                  &iter->section, xkb_file)) {
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

/* Lookup a string by its index */
const char *
xkb_file_section_get_string(const struct xkb_file_section *section, darray_size_t idx)
{
    return &darray_item(section->buffer, idx);
}
