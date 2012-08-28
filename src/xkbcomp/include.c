/************************************************************
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting
 * documentation, and that the name of Silicon Graphics not be
 * used in advertising or publicity pertaining to distribution
 * of the software without specific prior written permission.
 * Silicon Graphics makes no representation about the suitability
 * of this software for any purpose. It is provided "as is"
 * without any express or implied warranty.
 *
 * SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 * GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 * THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 ********************************************************/

#include <errno.h>
#include <limits.h>
#include <stdio.h>

#include "xkbcomp-priv.h"
#include "include.h"

/**
 * Extract the first token from an include statement.
 * @param str_inout Input statement, modified in-place. Can be passed in
 * repeatedly. If str_inout is NULL, the parsing has completed.
 * @param file_rtrn Set to the include file to be used.
 * @param map_rtrn Set to whatever comes after ), if any.
 * @param nextop_rtrn Set to the next operation in the complete statement.
 * @param extra_data Set to the string between ( and ), if any.
 *
 * @return true if parsing was succcessful, false for an illegal string.
 *
 * Example: "evdev+aliases(qwerty)"
 *      str_inout = aliases(qwerty)
 *      nextop_retrn = +
 *      extra_data = NULL
 *      file_rtrn = evdev
 *      map_rtrn = NULL
 *
 * 2nd run with "aliases(qwerty)"
 *      str_inout = NULL
 *      file_rtrn = aliases
 *      map_rtrn = qwerty
 *      extra_data = NULL
 *      nextop_retrn = ""
 *
 */
bool
ParseIncludeMap(char **str_inout, char **file_rtrn, char **map_rtrn,
                char *nextop_rtrn, char **extra_data)
{
    char *tmp, *str, *next;

    str = *str_inout;

    /* search for tokens inside the string */
    next = strpbrk(str, "|+");
    if (next) {
        /* set nextop_rtrn to \0, next to next character */
        *nextop_rtrn = *next;
        *next++ = '\0';
    }
    else {
        *nextop_rtrn = '\0';
        next = NULL;
    }

    /* search for :, store result in extra_data */
    tmp = strchr(str, ':');
    if (tmp != NULL) {
        *tmp++ = '\0';
        *extra_data = strdup(tmp);
    }
    else {
        *extra_data = NULL;
    }

    tmp = strchr(str, '(');
    if (tmp == NULL) {
        *file_rtrn = strdup(str);
        *map_rtrn = NULL;
    }
    else if (str[0] == '(') {
        free(*extra_data);
        return false;
    }
    else {
        *tmp++ = '\0';
        *file_rtrn = strdup(str);
        str = tmp;
        tmp = strchr(str, ')');
        if ((tmp == NULL) || (tmp[1] != '\0')) {
            free(*file_rtrn);
            free(*extra_data);
            return false;
        }
        *tmp++ = '\0';
        *map_rtrn = strdup(str);
    }

    if (*nextop_rtrn == '\0')
        *str_inout = NULL;
    else if ((*nextop_rtrn == '|') || (*nextop_rtrn == '+'))
        *str_inout = next;
    else
        return false;

    return true;
}

/***====================================================================***/

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

/***====================================================================***/

/**
 * Search for the given file name in the include directories.
 *
 * @param ctx the XKB ctx containing the include paths
 * @param type one of FILE_TYPE_TYPES, FILE_TYPE_COMPAT, ..., or
 *             FILE_TYPE_KEYMAP or FILE_TYPE_RULES
 * @param pathRtrn is set to the full path of the file if found.
 *
 * @return an FD to the file or NULL. If NULL is returned, the value of
 * pathRtrn is undefined.
 */
FILE *
FindFileInXkbPath(struct xkb_context *ctx, const char *name,
                  enum xkb_file_type type, char **pathRtrn)
{
    size_t i;
    int ret;
    FILE *file = NULL;
    char buf[PATH_MAX];
    const char *typeDir;

    typeDir = DirectoryForInclude(type);
    for (i = 0; i < xkb_context_num_include_paths(ctx); i++) {
        ret = snprintf(buf, sizeof(buf), "%s/%s/%s",
                       xkb_context_include_path_get(ctx, i), typeDir, name);
        if (ret >= (ssize_t) sizeof(buf)) {
            log_err(ctx, "File name (%s/%s/%s) too long\n",
                    xkb_context_include_path_get(ctx, i), typeDir, name);
            continue;
        }
        file = fopen(buf, "r");
        if (file == NULL) {
            log_err(ctx, "Couldn't open file (%s/%s/%s): %s\n",
                    xkb_context_include_path_get(ctx, i), typeDir, name,
                    strerror(errno));
            continue;
        }
        break;
    }

    if ((file != NULL) && (pathRtrn != NULL))
        *pathRtrn = strdup(buf);
    return file;
}

/**
 * Open the file given in the include statement and parse it's content.
 * If the statement defines a specific map to use, this map is returned in
 * file_rtrn. Otherwise, the default map is returned.
 *
 * @param ctx The ctx containing include paths
 * @param stmt The include statement, specifying the file name to look for.
 * @param file_type Type of file (FILE_TYPE_KEYCODES, etc.)
 * @param file_rtrn Returns the key map to be used.
 * @param merge_rtrn Always returns stmt->merge.
 *
 * @return true on success or false otherwise.
 */
bool
ProcessIncludeFile(struct xkb_context *ctx,
                   IncludeStmt * stmt,
                   enum xkb_file_type file_type,
                   XkbFile ** file_rtrn, enum merge_mode *merge_rtrn)
{
    FILE *file;
    XkbFile *rtrn, *mapToUse, *next;

    file = FindFileInXkbPath(ctx, stmt->file, file_type, NULL);
    if (file == NULL) {
        log_err(ctx, "Can't find file \"%s\" for %s include\n", stmt->file,
                DirectoryForInclude(file_type));
        return false;
    }

    if (!XkbParseFile(ctx, file, stmt->file, &rtrn)) {
        log_err(ctx, "Error interpreting include file \"%s\"\n", stmt->file);
        fclose(file);
        return false;
    }
    fclose(file);

    mapToUse = rtrn;
    if (stmt->map != NULL) {
        while (mapToUse)
        {
            next = (XkbFile *) mapToUse->common.next;
            mapToUse->common.next = NULL;
            if (streq(mapToUse->name, stmt->map) &&
                mapToUse->file_type == file_type) {
                FreeXkbFile(next);
                break;
            }
            else {
                FreeXkbFile(mapToUse);
            }
            mapToUse = next;
        }
        if (!mapToUse) {
            log_err(ctx, "No %s named \"%s\" in the include file \"%s\"\n",
                    xkb_file_type_to_string(file_type), stmt->map,
                    stmt->file);
            return false;
        }
    }
    else if (rtrn->common.next) {
        for (; mapToUse; mapToUse = (XkbFile *) mapToUse->common.next)
            if (mapToUse->flags & XkbLC_Default)
                break;

        if (!mapToUse) {
            mapToUse = rtrn;
            log_vrb(ctx, 5,
                    "No map in include statement, but \"%s\" contains several; "
                    "Using first defined map, \"%s\"\n",
                    stmt->file, rtrn->name);
        }
    }

    if (mapToUse->file_type != file_type) {
        log_err(ctx,
                "Include file wrong type (expected %s, got %s); "
                "Include file \"%s\" ignored\n",
                xkb_file_type_to_string(file_type),
                xkb_file_type_to_string(mapToUse->file_type), stmt->file);
        return false;
    }

    /* FIXME: we have to check recursive includes here (or somewhere) */

    *file_rtrn = mapToUse;
    *merge_rtrn = stmt->merge;
    return true;
}
