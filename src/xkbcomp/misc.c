/************************************************************
 Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.

 Permission to use, copy, modify, and distribute this
 software and its documentation for any purpose and without
 fee is hereby granted, provided that the above copyright
 notice appear in all copies and that both that copyright
 notice and this permission notice appear in supporting
 documentation, and that the name of Silicon Graphics not be
 used in advertising or publicity pertaining to distribution
 of the software without specific prior written permission.
 Silicon Graphics makes no representation about the suitability
 of this software for any purpose. It is provided "as is"
 without any express or implied warranty.

 SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 THE USE OR PERFORMANCE OF THIS SOFTWARE.

 ********************************************************/

#include "xkbcomp-priv.h"
#include "path.h"
#include "parseutils.h"
#include "keycodes.h"

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
                   XkbFile ** file_rtrn, unsigned *merge_rtrn)
{
    FILE *file;
    XkbFile *rtrn, *mapToUse, *next;

    file = XkbFindFileInPath(ctx, stmt->file, file_type, &stmt->path);
    if (file == NULL)
    {
        ERROR("Can't find file \"%s\" for %s include\n", stmt->file,
                XkbDirectoryForInclude(file_type));
        return false;
    }


    if (!XKBParseFile(ctx, file, stmt->file, &rtrn))
    {
        ERROR("Error interpreting include file \"%s\"\n", stmt->file);
        fclose(file);
        return false;
    }
    fclose(file);

    mapToUse = rtrn;
    if (stmt->map != NULL)
    {
        while (mapToUse)
        {
            next = (XkbFile *)mapToUse->common.next;
            mapToUse->common.next = NULL;
            if (strcmp(mapToUse->name, stmt->map) == 0 &&
                mapToUse->type == file_type)
            {
                    FreeXKBFile(next);
                    break;
            }
            else
            {
                FreeXKBFile(mapToUse);
            }
            mapToUse = next;
        }
        if (!mapToUse)
        {
            ERROR("No %s named \"%s\" in the include file \"%s\"\n",
                   XkbcFileTypeText(file_type), stmt->map, stmt->file);
            return false;
        }
    }
    else if ((rtrn->common.next != NULL) && (warningLevel > 5))
    {
        WARN("No map in include statement, but \"%s\" contains several\n",
              stmt->file);
        ACTION("Using first defined map, \"%s\"\n", rtrn->name);
    }
    if (mapToUse->type != file_type)
    {
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

/***====================================================================***/

int
ReportNotArray(const char *type, const char *field, const char *name)
{
    ERROR("The %s %s field is not an array\n", type, field);
    ACTION("Ignoring illegal assignment in %s\n", name);
    return false;
}

int
ReportShouldBeArray(const char *type, const char *field, const char *name)
{
    ERROR("Missing subscript for %s %s\n", type, field);
    ACTION("Ignoring illegal assignment in %s\n", name);
    return false;
}

int
ReportBadType(const char *type, const char *field,
              const char *name, const char *wanted)
{
    ERROR("The %s %s field must be a %s\n", type, field, wanted);
    ACTION("Ignoring illegal assignment in %s\n", name);
    return false;
}

int
ReportBadField(const char *type, const char *field, const char *name)
{
    ERROR("Unknown %s field %s in %s\n", type, field, name);
    ACTION("Ignoring assignment to unknown field in %s\n", name);
    return false;
}

/***====================================================================***/

bool
UseNewField(unsigned field,
            CommonInfo * oldDefs, CommonInfo * newDefs, unsigned *pCollide)
{
    bool useNew;

    useNew = false;
    if (oldDefs->defined & field)
    {
        if (newDefs->defined & field)
        {
            if (((oldDefs->file_id == newDefs->file_id)
                 && (warningLevel > 0)) || (warningLevel > 9))
            {
                *pCollide |= field;
            }
            if (newDefs->merge != MERGE_AUGMENT)
                useNew = true;
        }
    }
    else if (newDefs->defined & field)
        useNew = true;
    return useNew;
}

void *
ClearCommonInfo(CommonInfo * cmn)
{
    if (cmn != NULL)
    {
        CommonInfo *this, *next;
        for (this = cmn; this != NULL; this = next)
        {
            next = this->next;
            free(this);
        }
    }
    return NULL;
}

void *
AddCommonInfo(CommonInfo * old, CommonInfo * new)
{
    CommonInfo *first;

    first = old;
    while (old && old->next)
    {
        old = old->next;
    }
    new->next = NULL;
    if (old)
    {
        old->next = new;
        return first;
    }
    return new;
}

/***====================================================================***/

/**
 * Find the key with the given name and return its keycode in kc_rtrn.
 *
 * @param keymap The keymap to search in.
 * @param name The 4-letter name of the key as a long.
 * @param kc_rtrn Set to the keycode if the key was found, otherwise 0.
 * @param use_aliases true if the key aliases should be searched too.
 * @param create If true and the key is not found, it is added to the
 *        keymap->names at the first free keycode.
 * @param start_from Keycode to start searching from.
 *
 * @return true if found, false otherwise.
 */
bool
FindNamedKey(struct xkb_keymap *keymap, unsigned long name,
             xkb_keycode_t *kc_rtrn, bool use_aliases, bool create,
             xkb_keycode_t start_from)
{
    unsigned n;

    if (start_from < keymap->min_key_code)
    {
        start_from = keymap->min_key_code;
    }
    else if (start_from > keymap->max_key_code)
    {
        return false;
    }

    *kc_rtrn = 0;               /* some callers rely on this */
    if (keymap && keymap->names && !darray_empty(keymap->names->keys))
    {
        for (n = start_from; n <= keymap->max_key_code; n++)
        {
            unsigned long tmp;
            tmp = KeyNameToLong(darray_item(keymap->names->keys, n).name);
            if (tmp == name)
            {
                *kc_rtrn = n;
                return true;
            }
        }
        if (use_aliases)
        {
            unsigned long new_name;
            if (FindKeyNameForAlias(keymap, name, &new_name))
                return FindNamedKey(keymap, new_name, kc_rtrn, false,
                                    create, 0);
        }
    }
    if (create)
    {
        if ((!keymap->names) || darray_empty(keymap->names->keys))
        {
            if (XkbcAllocNames(keymap, XkbKeyNamesMask, 0) != Success)
            {
                if (warningLevel > 0)
                {
                    WARN("Couldn't allocate key names in FindNamedKey\n");
                    ACTION("Key \"%s\" not automatically created\n",
                            longText(name));
                }
                return false;
            }
        }
        /* Find first unused keycode and store our key here */
        for (n = keymap->min_key_code; n <= keymap->max_key_code; n++)
        {
            if (darray_item(keymap->names->keys, n).name[0] == '\0')
            {
                char buf[XkbKeyNameLength + 1];
                LongToKeyName(name, buf);
                memcpy(darray_item(keymap->names->keys, n).name, buf,
                       XkbKeyNameLength);
                *kc_rtrn = n;
                return true;
            }
        }
    }
    return false;
}

bool
FindKeyNameForAlias(struct xkb_keymap *keymap, unsigned long lname,
                    unsigned long *real_name)
{
    char name[XkbKeyNameLength + 1];
    struct xkb_key_alias *a;

    if (keymap && keymap->names) {
        LongToKeyName(lname, name);
        name[XkbKeyNameLength] = '\0';
        darray_foreach(a, keymap->names->key_aliases) {
            if (strncmp(name, a->alias, XkbKeyNameLength) == 0) {
                *real_name = KeyNameToLong(a->real);
                return true;
            }
        }
    }
    return false;
}
