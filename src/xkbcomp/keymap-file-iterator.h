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

XKB_EXPORT_PRIVATE FILE *
xkb_resolve_file(struct xkb_context *ctx,
                 enum xkb_file_type file_type,
                 const char *path, const char *map,
                 char *resolved_path, size_t resolved_path_size,
                 char *resolved_map, size_t resolved_map_size);

/** An atomic include entry */
struct xkb_file_include {
    bool valid:1;
    bool explicit_section:1;
    enum merge_mode merge;
    /*
     * The following are indices from `xkb_file_section::buffer`
     *
     * Use `xkb_file_section_get_string()` to retrieve them.
     */
    darray_size_t path;
    darray_size_t file;
    darray_size_t section;
    darray_size_t modifier;
    /* Flags of the included section, only set if valid = true */
    enum xkb_map_flags flags;
};

struct xkb_file_include_group {
    darray_size_t start;
    darray_size_t end;
};

/** A file section: `xkb_{keycodes,compat,geometry,symbols,types} */
struct xkb_file_section {
    darray_size_t name;
    enum xkb_file_type file_type;
    enum xkb_map_flags flags;
    darray(struct xkb_file_include_group) include_groups;
    darray(struct xkb_file_include) includes;
    /**
     * Char array to store all the strings
     *
     * Use `xkb_file_section_get_string()` to retrieve them.
     */
    darray_char buffer;
};

XKB_EXPORT_PRIVATE void
xkb_file_section_init(struct xkb_file_section *section);

XKB_EXPORT_PRIVATE void
xkb_file_section_free(struct xkb_file_section *section);

enum xkb_file_iterator_flags {
    XKB_FILE_ITERATOR_NO_FLAG = 0,
    /** Include errors are fatal */
    XKB_FILE_ITERATOR_FAIL_ON_INCLUDE_ERROR = (1 << 0),
    /** Do not list includes */
    XKB_FILE_ITERATOR_NO_INCLUDES = (1 << 1)
};

/**
 * Parse a single keymap file section
 *
 * @param[in]    ctx            The XKB context
 * @param[in]    iterator_flags The flags used to parse the section
 * @param[in]    format         The keymap format used to parse the keymap file
 * @param[in]    compile_flags  The flags used to parse the keymap file
 * @param[in]    path           The path of the keymap file to parse
 * @param[in]    map            The name of a specific section in the file
 * @param[inout] section        The resulting section
 *
 * @returns `true` on success, else `false`
 */
XKB_EXPORT_PRIVATE bool
xkb_file_section_parse(struct xkb_context *ctx,
                       enum xkb_file_iterator_flags iterator_flags,
                       enum xkb_keymap_format format,
                       enum xkb_keymap_compile_flags compile_flags,
                       unsigned int include_depth,
                       const char *path, const char *map,
                       struct xkb_file_section *section);

XKB_EXPORT_PRIVATE const char *
xkb_file_section_get_string(const struct xkb_file_section *section,
                            darray_size_t idx);

/** Iterator over the sections of a keymap file */
struct xkb_file_iterator {
    enum xkb_file_iterator_flags flags;
    bool finished;
    /** File to analyze */
    const char *path;
    /** Section to analyze; if NULL then iter over all the sections */
    const char *map;
    /** File type to check/filter */
    enum xkb_file_type type;
    /** Scanner of the keymap file */
    struct scanner scanner;
    /** Current section */
    struct xkb_file_section section;
    /** Pending XKB file */
    XkbFile *pending_xkb_file;
    /** Pending component of the XKB file */
    XkbFile *pending_section;
    struct xkb_context *ctx;
};

/**
 * Create a keymap file section iterator from a buffer
 *
 * @param ctx            The XKB context
 * @param iterator_flags The flags used to parse the section
 * @param format         The keymap format used to parse the keymap file
 * @param compile_flags  The flags used to parse the keymap file
 * @param path           The path of the keymap file parsed (informative)
 * @param map            The name of a specific section in the file, else NULL
 * to iterate over all the sections of the file.
 * @param file_type      If the input is a keymap, iterate only over the
 * corresponding component; else check that the parsed file type matches.
 * @param string         The buffer to parse
 * @param length         The length of the buffer to parse
 *
 * @returns A `malloc`ed file iterator on success, else `NULL`.
 */
XKB_EXPORT_PRIVATE struct xkb_file_iterator *
xkb_file_iterator_new_from_buffer(struct xkb_context *ctx,
                                  enum xkb_file_iterator_flags iterator_flags,
                                  enum xkb_keymap_format format,
                                  enum xkb_keymap_compile_flags compile_flags,
                                  const char *path, const char *map,
                                  enum xkb_file_type file_type,
                                  const char *string, size_t length);

XKB_EXPORT_PRIVATE void
xkb_file_iterator_free(struct xkb_file_iterator *iter);

XKB_EXPORT_PRIVATE bool
xkb_file_iterator_next(struct xkb_file_iterator *iter,
                       const struct xkb_file_section **section);
