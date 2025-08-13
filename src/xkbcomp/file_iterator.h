/*
 * Copyright © 2025 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include "xkbcommon/xkbcommon.h"

#include "src/darray.h"
#include "src/scanner-utils.h"
#include "src/utils.h"
#include "ast.h"

XKB_EXPORT_PRIVATE const char *
xkb_file_type_name(enum xkb_file_type type);

XKB_EXPORT_PRIVATE const char *
xkb_merge_mode_name(enum merge_mode type);

XKB_EXPORT_PRIVATE const char *
xkb_map_flags_string_iter(unsigned int *index, enum xkb_map_flags flags);

struct xkb_file_include {
    enum merge_mode merge;
    darray_size_t path;
    darray_size_t file;
    darray_size_t section;
    darray_size_t modifier;
};

struct xkb_file_section {
    darray_size_t name;
    enum xkb_file_type file_type;
    enum xkb_map_flags flags;
    darray(struct xkb_file_include) includes;
    darray_char buf;
};

XKB_EXPORT_PRIVATE void
xkb_file_section_init(struct xkb_file_section *section);

XKB_EXPORT_PRIVATE void
xkb_file_section_free(struct xkb_file_section *section);

XKB_EXPORT_PRIVATE bool
xkb_file_section_parse(struct xkb_context *ctx,
                       enum xkb_keymap_format format,
                       enum xkb_keymap_compile_flags flags,
                       const char *path, const char *map,
                       unsigned int include_depth,
                       struct xkb_file_section *section);

XKB_EXPORT_PRIVATE const char *
xkb_file_section_get_string(const struct xkb_file_section *section,
                            darray_size_t idx);

struct xkb_file_iterator {
    bool finished;
    struct scanner scanner;
    /** Current section */
    struct xkb_file_section section;
    const char *path;
    const char *map;
    /** Pending XKB file */
    XkbFile *pending_xkb_file;
    /** Pending component of the XKB file */
    XkbFile *pending_section;
    struct xkb_context *ctx;
};

XKB_EXPORT_PRIVATE struct xkb_file_iterator *
xkb_file_iterator_new(struct xkb_context *ctx,
                      enum xkb_keymap_format format,
                      enum xkb_keymap_compile_flags flags,
                      const char *path, const char *map,
                      const char *string, size_t length);

XKB_EXPORT_PRIVATE void
xkb_file_iterator_free(struct xkb_file_iterator *iter);

XKB_EXPORT_PRIVATE bool
xkb_file_iterator_next(struct xkb_file_iterator *iter,
                       const struct xkb_file_section **section);
