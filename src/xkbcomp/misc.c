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

#include "xkbcomp-priv.h"
#include "path.h"
#include "parseutils.h"

/***====================================================================***/

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

    file = XkbFindFileInPath(ctx, stmt->file, file_type, &stmt->path);
    if (file == NULL) {
        ERROR("Can't find file \"%s\" for %s include\n", stmt->file,
              XkbDirectoryForInclude(file_type));
        return false;
    }

    if (!XKBParseFile(ctx, file, stmt->file, &rtrn)) {
        ERROR("Error interpreting include file \"%s\"\n", stmt->file);
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
            if (strcmp(mapToUse->name, stmt->map) == 0 &&
                mapToUse->type == file_type) {
                FreeXKBFile(next);
                break;
            }
            else {
                FreeXKBFile(mapToUse);
            }
            mapToUse = next;
        }
        if (!mapToUse) {
            ERROR("No %s named \"%s\" in the include file \"%s\"\n",
                  XkbcFileTypeText(file_type), stmt->map, stmt->file);
            return false;
        }
    }
    else if ((rtrn->common.next != NULL) && (warningLevel > 5)) {
        WARN("No map in include statement, but \"%s\" contains several\n",
             stmt->file);
        ACTION("Using first defined map, \"%s\"\n", rtrn->name);
    }
    if (mapToUse->type != file_type) {
        ERROR("Include file wrong type (expected %s, got %s)\n",
              XkbcFileTypeText(file_type), XkbcFileTypeText(mapToUse->type));
        ACTION("Include file \"%s\" ignored\n", stmt->file);
        return false;
    }
    /* FIXME: we have to check recursive includes here (or somewhere) */

    *file_rtrn = mapToUse;
    *merge_rtrn = stmt->merge;
    return true;
}

bool
UseNewField(unsigned field, short old_defined, unsigned old_file_id,
            short new_defined, unsigned new_file_id,
            enum merge_mode new_merge, unsigned *collide)
{
    if (!(old_defined & field))
        return true;

    if (new_defined & field) {
        if ((old_file_id == new_file_id && warningLevel > 0) ||
            warningLevel > 9)
            *collide |= field;

        if (new_merge != MERGE_AUGMENT)
            return true;
    }

    return false;
}

/**
 * Find the key with the given name.
 *
 * @param keymap The keymap to search in.
 * @param name The 4-letter name of the key as a long.
 * @param use_aliases true if the key aliases should be searched too.
 * @param create If true and the key is not found, it is added to the
 *        keymap->names at the first free keycode.
 * @param start_from Keycode to start searching from.
 *
 * @return the key if it is found, NULL otherwise.
 */
struct xkb_key *
FindNamedKey(struct xkb_keymap *keymap, unsigned long name,
             bool use_aliases, bool create, xkb_keycode_t start_from)
{
    struct xkb_key *key;

    if (start_from < keymap->min_key_code)
        start_from = keymap->min_key_code;
    else if (start_from > keymap->max_key_code)
        return NULL;

    xkb_foreach_key_from(key, keymap, start_from)
        if (KeyNameToLong(key->name) == name)
            return key;

    if (use_aliases) {
        unsigned long new_name;
        if (FindKeyNameForAlias(keymap, name, &new_name))
            return FindNamedKey(keymap, new_name, false, create, 0);
    }

    if (create) {
        /* Find first unused key and store our key here */
        xkb_foreach_key(key, keymap) {
            if (key->name[0] == '\0') {
                char buf[XkbKeyNameLength + 1];
                LongToKeyName(name, buf);
                memcpy(key->name, buf, XkbKeyNameLength);
                return key;
            }
        }
    }

    return NULL;
}

bool
FindKeyNameForAlias(struct xkb_keymap *keymap, unsigned long lname,
                    unsigned long *real_name)
{
    char name[XkbKeyNameLength + 1];
    struct xkb_key_alias *a;

    LongToKeyName(lname, name);
    name[XkbKeyNameLength] = '\0';
    darray_foreach(a, keymap->key_aliases) {
        if (strncmp(name, a->alias, XkbKeyNameLength) == 0) {
            *real_name = KeyNameToLong(a->real);
            return true;
        }
    }

    return false;
}
