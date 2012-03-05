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

#include "xkbcomp.h"
#include "xkballoc.h"
#include "xkbmisc.h"
#include "xkbpath.h"
#include "keycodes.h"
#include "misc.h"
#include <X11/keysym.h>
#include "parseutils.h"

/***====================================================================***/

/**
 * Open the file given in the include statement and parse it's content.
 * If the statement defines a specific map to use, this map is returned in
 * file_rtrn. Otherwise, the default map is returned.
 *
 * @param stmt The include statement, specifying the file name to look for.
 * @param file_type Type of file (XkmKeyNamesIdx, etc.)
 * @param file_rtrn Returns the key map to be used.
 * @param merge_rtrn Always returns stmt->merge.
 *
 * @return True on success or False otherwise.
 */
Bool
ProcessIncludeFile(IncludeStmt * stmt,
                   unsigned file_type,
                   XkbFile ** file_rtrn, unsigned *merge_rtrn)
{
    FILE *file;
    XkbFile *rtrn, *mapToUse, *next;
    char oldFile[1024] = {0};
    int oldLine = lineNum;

    file = XkbFindFileInPath(stmt->file, file_type, &stmt->path);
    if (file == NULL)
    {
        ERROR("Can't find file \"%s\" for %s include\n", stmt->file,
                XkbDirectoryForInclude(file_type));
        return False;
    }
    if (scanFile)
        strcpy(oldFile, scanFile);
    else
        memset(oldFile, 0, sizeof(oldFile));
    oldLine = lineNum;
    setScanState(stmt->file, 1);
    if (debugFlags & 2)
        INFO("About to parse include file %s\n", stmt->file);
    /* parse the file */
    if ((XKBParseFile(file, &rtrn) == 0) || (rtrn == NULL))
    {
        setScanState(oldFile, oldLine);
        ERROR("Error interpreting include file \"%s\"\n", stmt->file);
        fclose(file);
        return False;
    }
    fclose(file);

    mapToUse = rtrn;
    if (stmt->map != NULL)
    {
        while (mapToUse)
        {
            next = (XkbFile *)mapToUse->common.next;
            mapToUse->common.next = NULL;
            if (uStringEqual(mapToUse->name, stmt->map) &&
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
                   XkbcConfigText(file_type), stmt->map, stmt->file);
            return False;
        }
    }
    else if ((rtrn->common.next != NULL) && (warningLevel > 5))
    {
        WARN("No map in include statement, but \"%s\" contains several\n",
              stmt->file);
        ACTION("Using first defined map, \"%s\"\n", rtrn->name);
    }
    setScanState(oldFile, oldLine);
    if (mapToUse->type != file_type)
    {
        ERROR("Include file wrong type (expected %s, got %s)\n",
               XkbcConfigText(file_type), XkbcConfigText(mapToUse->type));
        ACTION("Include file \"%s\" ignored\n", stmt->file);
        return False;
    }
    /* FIXME: we have to check recursive includes here (or somewhere) */

    mapToUse->compiled = True;
    *file_rtrn = mapToUse;
    *merge_rtrn = stmt->merge;
    return True;
}

/***====================================================================***/

int
ReportNotArray(const char *type, const char *field, const char *name)
{
    ERROR("The %s %s field is not an array\n", type, field);
    ACTION("Ignoring illegal assignment in %s\n", name);
    return False;
}

int
ReportShouldBeArray(const char *type, const char *field, const char *name)
{
    ERROR("Missing subscript for %s %s\n", type, field);
    ACTION("Ignoring illegal assignment in %s\n", name);
    return False;
}

int
ReportBadType(const char *type, const char *field,
              const char *name, const char *wanted)
{
    ERROR("The %s %s field must be a %s\n", type, field, wanted);
    ACTION("Ignoring illegal assignment in %s\n", name);
    return False;
}

int
ReportBadField(const char *type, const char *field, const char *name)
{
    ERROR("Unknown %s field %s in %s\n", type, field, name);
    ACTION("Ignoring assignment to unknown field in %s\n", name);
    return False;
}

/***====================================================================***/

Bool
UseNewField(unsigned field,
            CommonInfo * oldDefs, CommonInfo * newDefs, unsigned *pCollide)
{
    Bool useNew;

    useNew = False;
    if (oldDefs->defined & field)
    {
        if (newDefs->defined & field)
        {
            if (((oldDefs->fileID == newDefs->fileID)
                 && (warningLevel > 0)) || (warningLevel > 9))
            {
                *pCollide |= field;
            }
            if (newDefs->merge != MergeAugment)
                useNew = True;
        }
    }
    else if (newDefs->defined & field)
        useNew = True;
    return useNew;
}

char *
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

char *
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
        return (char *) first;
    }
    return (char *) new;
}

/***====================================================================***/

typedef struct _KeyNameDesc
{
    uint32_t level1;
    uint32_t level2;
    char name[5];
    Bool used;
} KeyNameDesc;

/**
 * Find the key with the given name and return its keycode in kc_rtrn.
 *
 * @param name The 4-letter name of the key as a long.
 * @param kc_rtrn Set to the keycode if the key was found, otherwise 0.
 * @param use_aliases True if the key aliases should be searched too.
 * @param create If True and the key is not found, it is added to the
 *        xkb->names at the first free keycode.
 * @param start_from Keycode to start searching from.
 *
 * @return True if found, False otherwise.
 */
Bool
FindNamedKey(struct xkb_desc * xkb,
             unsigned long name,
             xkb_keycode_t *kc_rtrn,
             Bool use_aliases, Bool create, int start_from)
{
    unsigned n;

    if (start_from < xkb->min_key_code)
    {
        start_from = xkb->min_key_code;
    }
    else if (start_from > xkb->max_key_code)
    {
        return False;
    }

    *kc_rtrn = 0;               /* some callers rely on this */
    if (xkb && xkb->names && xkb->names->keys)
    {
        for (n = start_from; n <= xkb->max_key_code; n++)
        {
            unsigned long tmp;
            tmp = KeyNameToLong(xkb->names->keys[n].name);
            if (tmp == name)
            {
                *kc_rtrn = n;
                return True;
            }
        }
        if (use_aliases)
        {
            unsigned long new_name;
            if (FindKeyNameForAlias(xkb, name, &new_name))
                return FindNamedKey(xkb, new_name, kc_rtrn, False, create, 0);
        }
    }
    if (create)
    {
        if ((!xkb->names) || (!xkb->names->keys))
        {
            if (XkbcAllocNames(xkb, XkbKeyNamesMask, 0, 0) != Success)
            {
                if (warningLevel > 0)
                {
                    WARN("Couldn't allocate key names in FindNamedKey\n");
                    ACTION("Key \"%s\" not automatically created\n",
                            longText(name));
                }
                return False;
            }
        }
        /* Find first unused keycode and store our key here */
        for (n = xkb->min_key_code; n <= xkb->max_key_code; n++)
        {
            if (xkb->names->keys[n].name[0] == '\0')
            {
                char buf[XkbKeyNameLength + 1];
                LongToKeyName(name, buf);
                memcpy(xkb->names->keys[n].name, buf, XkbKeyNameLength);
                *kc_rtrn = n;
                return True;
            }
        }
    }
    return False;
}

Bool
FindKeyNameForAlias(struct xkb_desc * xkb, unsigned long lname,
                    unsigned long *real_name)
{
    int i;
    char name[XkbKeyNameLength + 1];

    if (xkb && xkb->geom && xkb->geom->key_aliases)
    {
        struct xkb_key_alias * a;
        a = xkb->geom->key_aliases;
        LongToKeyName(lname, name);
        name[XkbKeyNameLength] = '\0';
        for (i = 0; i < xkb->geom->num_key_aliases; i++, a++)
        {
            if (strncmp(name, a->alias, XkbKeyNameLength) == 0)
            {
                *real_name = KeyNameToLong(a->real);
                return True;
            }
        }
    }
    if (xkb && xkb->names && xkb->names->key_aliases)
    {
        struct xkb_key_alias * a;
        a = xkb->names->key_aliases;
        LongToKeyName(lname, name);
        name[XkbKeyNameLength] = '\0';
        for (i = 0; i < xkb->names->num_key_aliases; i++, a++)
        {
            if (strncmp(name, a->alias, XkbKeyNameLength) == 0)
            {
                *real_name = KeyNameToLong(a->real);
                return True;
            }
        }
    }
    return False;
}
