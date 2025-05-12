/*
 * For HPND:
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 *
 * For MIT:
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 *
 * SPDX-License-Identifier: HPND AND MIT
 */

#include "config.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>

#include "xkbcomp-priv.h"
#include "include.h"

/**
 * Parse an include statement. Each call returns a file name, along with
 * (possibly) a specific map in the file, an explicit group designator, and
 * the separator from the next file, used to determine the merge mode.
 *
 * @param str_inout Input statement, modified in-place. Should be passed in
 * repeatedly. If str_inout is NULL, the parsing has completed.
 *
 * @param file_rtrn Set to the name of the include file to be used. Combined
 * with an enum xkb_file_type, this determines which file to look for in the
 * include path.
 *
 * @param map_rtrn Set to the string between '(' and ')', if any. This will
 * result in the compilation of a specific named map within the file (e.g.
 * xkb_symbols "basic" { ... }) , as opposed to the default map of the file.
 *
 * @param nextop_rtrn Set to the next operation in the complete statement,
 * which is '\0' if it's the last file or '+' or '|' or '^' if there are more.
 * Separating the files with '+' sets the merge mode to `MERGE_MODE_OVERRIDE`,
 * while '|' sets the merge mode to `MERGE_MODE_AUGMENT` and '^' sets the merge
 * mode to `MERGE_REPLACE`.
 *
 * @param extra_data Set to the string after ':', if any. Currently the
 * extra data is only used for setting an explicit group index for a symbols
 * file.
 *
 * @return true if parsing was successful, false for an illegal string.
 *
 * Example: "evdev+aliases(qwerty):2"
 *      str_inout = "aliases(qwerty):2"
 *      file_rtrn = "evdev"
 *      map_rtrn = NULL
 *      nextop_retrn = "+"
 *      extra_data = NULL
 *
 * 2nd run with "aliases(qwerty):2"
 *      str_inout = NULL
 *      file_rtrn = "aliases"
 *      map_rtrn = "qwerty"
 *      nextop_retrn = ""
 *      extra_data = "2"
 *
 */
bool
ParseIncludeMap(char **str_inout, char **file_rtrn, char **map_rtrn,
                char *nextop_rtrn, char **extra_data)
{
    char *tmp, *str, *next;

    str = *str_inout;

    /*
     * Find the position in the string where the next file is included,
     * if there is more than one left in the statement.
     */
    next = strpbrk(str, MERGE_MODE_PREFIXES);
    if (next) {
        /* Got more files, this function will be called again. */
        *nextop_rtrn = *next;
        /* Separate the string, for strchr etc. to work on this file only. */
        *next++ = '\0';
    }
    else {
        /* This is the last file in this statement, won't be called again. */
        *nextop_rtrn = '\0';
        next = NULL;
    }

    /*
     * Search for the explicit group designator, if any. If it's there,
     * it goes after the file name and map.
     */
    tmp = strchr(str, ':');
    if (tmp != NULL) {
        *tmp++ = '\0';
        *extra_data = strdup(tmp);
    }
    else {
        *extra_data = NULL;
    }

    /* Look for a map, if any. */
    tmp = strchr(str, '(');
    if (tmp == NULL) {
        /* No map. */
        *file_rtrn = strdup(str);
        *map_rtrn = NULL;
    }
    else if (str[0] == '(') {
        /* Map without file - invalid. */
        free(*extra_data);
        return false;
    }
    else {
        /* Got a map; separate the file and the map for the strdup's. */
        *tmp++ = '\0';
        *file_rtrn = strdup(str);
        str = tmp;
        tmp = strchr(str, ')');
        if (tmp == NULL || tmp[1] != '\0') {
            free(*file_rtrn);
            free(*extra_data);
            return false;
        }
        *tmp++ = '\0';
        *map_rtrn = strdup(str);
    }

    /* Set up the next file for the next call, if any. */
    if (*nextop_rtrn == '\0')
        *str_inout = NULL;
    else if (is_merge_mode_prefix(*nextop_rtrn))
        *str_inout = next;
    else
        return false;

    return true;
}

static const char *xkb_file_type_include_dirs[_FILE_TYPE_NUM_ENTRIES] = {
    [FILE_TYPE_KEYCODES] = "keycodes",
    [FILE_TYPE_TYPES] = "types",
    [FILE_TYPE_COMPAT] = "compat",
    [FILE_TYPE_SYMBOLS] = "symbols",
    [FILE_TYPE_GEOMETRY] = "geometry",
    [FILE_TYPE_KEYMAP] = "keymap",
    [FILE_TYPE_RULES] = "rules",
};

/**
 * Return the xkb directory based on the type.
 */
static const char *
DirectoryForInclude(enum xkb_file_type type)
{
    if (type >= _FILE_TYPE_NUM_ENTRIES)
        return "";
    return xkb_file_type_include_dirs[type];
}

static void
LogIncludePaths(struct xkb_context *ctx)
{
    if (xkb_context_num_include_paths(ctx) > 0) {
        log_err(ctx, XKB_ERROR_INCLUDED_FILE_NOT_FOUND,
                "%d include paths searched:\n",
                xkb_context_num_include_paths(ctx));
        for (unsigned int i = 0; i < xkb_context_num_include_paths(ctx); i++)
            log_err(ctx, XKB_ERROR_INCLUDED_FILE_NOT_FOUND,
                    "\t%s\n",
                    xkb_context_include_path_get(ctx, i));
    }
    else {
        log_err(ctx, XKB_ERROR_INCLUDED_FILE_NOT_FOUND,
                "There are no include paths to search\n");
    }

    if (xkb_context_num_failed_include_paths(ctx) > 0) {
        log_err(ctx, XKB_ERROR_INCLUDED_FILE_NOT_FOUND,
                "%d include paths could not be added:\n",
                xkb_context_num_failed_include_paths(ctx));
        for (darray_size_t i = 0;
             i < xkb_context_num_failed_include_paths(ctx);
             i++)
            log_err(ctx, XKB_ERROR_INCLUDED_FILE_NOT_FOUND,
                    "\t%s\n",
                    xkb_context_failed_include_path_get(ctx, i));
    }
}

/**
 * Return an open file handle to the first file (counting from offset) with the
 * given name in the include paths, starting at the offset.
 *
 * offset must be zero the first time this is called and is set to the index the
 * file was found. Call again with offset+1 to keep searching through the
 * include paths.
 *
 * If this function returns NULL, no more files are available.
 */
FILE *
FindFileInXkbPath(struct xkb_context *ctx, const char *name,
                  enum xkb_file_type type, char **pathRtrn,
                  unsigned int *offset)
{
    unsigned int i;
    FILE *file = NULL;
    char buf[PATH_MAX];
    const char *typeDir;

    typeDir = DirectoryForInclude(type);

    for (i = *offset; i < xkb_context_num_include_paths(ctx); i++) {
        if (!snprintf_safe(buf, sizeof(buf), "%s/%s/%s",
                           xkb_context_include_path_get(ctx, i),
                           typeDir, name)) {
            log_err(ctx, XKB_ERROR_INVALID_PATH,
                    "Path is too long: expected max length of %zu, "
                    "got: %s/%s/%s\n",
                    sizeof(buf), xkb_context_include_path_get(ctx, i),
                    typeDir, name);
            continue;
        }

        file = fopen(buf, "rb");
        if (file) {
            if (pathRtrn)
                *pathRtrn = strdup(buf);
            *offset = i;
            goto out;
        }
    }

    /* We only print warnings if we can't find the file on the first lookup */
    if (*offset == 0) {
        log_err(ctx, XKB_ERROR_INCLUDED_FILE_NOT_FOUND,
                "Couldn't find file \"%s/%s\" in include paths\n",
                typeDir, name);
        LogIncludePaths(ctx);
    }

out:
    return file;
}

bool
ExceedsIncludeMaxDepth(struct xkb_context *ctx, unsigned int include_depth)
{
    if (include_depth >= INCLUDE_MAX_DEPTH) {
        log_err(ctx, XKB_ERROR_RECURSIVE_INCLUDE,
                "Exceeded include depth threshold (%d)",
                INCLUDE_MAX_DEPTH);
        return true;
    } else {
        return false;
    }
}

XkbFile *
ProcessIncludeFile(struct xkb_context *ctx, IncludeStmt *stmt,
                   enum xkb_file_type file_type)
{
    /*
     * Resolve include statement:
     * - Explicit map name: look for an *exact match* only.
     * - Default map: look for an *explicit default* map (i.e. tagged with
     *   `default`: exact match), else fallback to the first *implicit* default
     *   map (weak match).
     */
    XkbFile *xkb_file = NULL;  /* Exact match */
    XkbFile *candidate = NULL; /* Weak match */
    unsigned int offset = 0;

    FILE* file = FindFileInXkbPath(ctx, stmt->file, file_type, NULL, &offset);
    if (!file)
        return NULL;

    while (file) {
        xkb_file = XkbParseFile(ctx, file, stmt->file, stmt->map);
        fclose(file);

        if (xkb_file) {
            if (xkb_file->file_type != file_type) {
                log_err(ctx, XKB_ERROR_INVALID_INCLUDED_FILE,
                        "Include file of wrong type (expected %s, got %s); "
                        "Include file \"%s\" ignored\n",
                        xkb_file_type_to_string(file_type),
                        xkb_file_type_to_string(xkb_file->file_type), stmt->file);
                FreeXkbFile(xkb_file);
                xkb_file = NULL;
            } else if (stmt->map || (xkb_file->flags && MAP_IS_DEFAULT)) {
                /*
                 * Exact match: explicit map name or explicit default map.
                 * Lookup stops here.
                 */
                break;
            } else if (!candidate) {
                /*
                 * Weak match: looking for an explicit default map, but found
                 * only the first *implicit* default map (e.g. first map of the
                 * file). Use as fallback, but keep looking for an exact match.
                 */
                candidate = xkb_file;
                xkb_file = NULL;
            } else {
                /*
                 * Weak match, but we already have a previous candidate.
                 * Keep looking for an exact match.
                 */
                FreeXkbFile(xkb_file);
                xkb_file = NULL;
            }
        }

        offset++;
        file = FindFileInXkbPath(ctx, stmt->file, file_type, NULL, &offset);
    }

    if (!xkb_file) {
        /* No exact match: use weak match, if any */
        xkb_file = candidate;
    } else {
        /* Found exact match: discard weak match, if any */
        FreeXkbFile(candidate);
    }

    if (!xkb_file) {
        if (stmt->map)
            log_err(ctx, XKB_ERROR_INVALID_INCLUDED_FILE,
                    "Couldn't process include statement for '%s(%s)'\n",
                    stmt->file, stmt->map);
        else
            log_err(ctx, XKB_ERROR_INVALID_INCLUDED_FILE,
                    "Couldn't process include statement for '%s'\n",
                    stmt->file);
    }

    return xkb_file;
}
