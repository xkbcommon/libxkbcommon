/************************************************************
 Copyright (c) 1995 by Silicon Graphics Computer Systems, Inc.

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
#include "xkbgeom.h"
#include "xkbmisc.h"
#include "misc.h"
#include "alias.h"
#include "keycodes.h"

static void
HandleCollision(AliasInfo * old, AliasInfo * new)
{
    if (strncmp(new->real, old->real, XkbKeyNameLength) == 0)
    {
        if (((new->def.fileID == old->def.fileID) && (warningLevel > 0)) ||
            (warningLevel > 9))
        {
            WARN("Alias of %s for %s declared more than once\n",
                  XkbcKeyNameText(new->alias), XkbcKeyNameText(new->real));
            ACTION("First definition ignored\n");
        }
    }
    else
    {
        char *use, *ignore;
        if (new->def.merge == MergeAugment)
        {
            use = old->real;
            ignore = new->real;
        }
        else
        {
            use = new->real;
            ignore = old->real;
        }
        if (((old->def.fileID == new->def.fileID) && (warningLevel > 0)) ||
            (warningLevel > 9))
        {
            WARN("Multiple definitions for alias %s\n",
                  XkbcKeyNameText(old->alias));
            ACTION("Using %s, ignoring %s\n",
                    XkbcKeyNameText(use), XkbcKeyNameText(ignore));
        }
        if (use != old->real)
            memcpy(old->real, use, XkbKeyNameLength);
    }
    old->def.fileID = new->def.fileID;
    old->def.merge = new->def.merge;
}

static void
InitAliasInfo(AliasInfo * info,
              unsigned merge, unsigned file_id, char *alias, char *real)
{
    bzero(info, sizeof(AliasInfo));
    info->def.merge = merge;
    info->def.fileID = file_id;
    strncpy(info->alias, alias, XkbKeyNameLength);
    strncpy(info->real, real, XkbKeyNameLength);
}

int
HandleAliasDef(KeyAliasDef * def,
               unsigned merge, unsigned file_id, AliasInfo ** info_in)
{
    AliasInfo *info;

    for (info = *info_in; info != NULL; info = (AliasInfo *) info->def.next)
    {
        if (strncmp(info->alias, def->alias, XkbKeyNameLength) == 0)
        {
            AliasInfo new;
            InitAliasInfo(&new, merge, file_id, def->alias, def->real);
            HandleCollision(info, &new);
            return True;
        }
    }
    info = uTypedCalloc(1, AliasInfo);
    if (info == NULL)
    {
        WSGO("Allocation failure in HandleAliasDef\n");
        return False;
    }
    info->def.fileID = file_id;
    info->def.merge = merge;
    info->def.next = (CommonInfo *) * info_in;
    memcpy(info->alias, def->alias, XkbKeyNameLength);
    memcpy(info->real, def->real, XkbKeyNameLength);
    *info_in = (AliasInfo *) AddCommonInfo(&(*info_in)->def, &info->def);
    return True;
}

void
ClearAliases(AliasInfo ** info_in)
{
    if ((info_in) && (*info_in))
        ClearCommonInfo(&(*info_in)->def);
}

Bool
MergeAliases(AliasInfo ** into, AliasInfo ** merge, unsigned how_merge)
{
    AliasInfo *tmp;
    KeyAliasDef def;

    if ((*merge) == NULL)
        return True;
    if ((*into) == NULL)
    {
        *into = *merge;
        *merge = NULL;
        return True;
    }
    bzero((char *) &def, sizeof(KeyAliasDef));
    for (tmp = *merge; tmp != NULL; tmp = (AliasInfo *) tmp->def.next)
    {
        if (how_merge == MergeDefault)
            def.merge = tmp->def.merge;
        else
            def.merge = how_merge;
        memcpy(def.alias, tmp->alias, XkbKeyNameLength);
        memcpy(def.real, tmp->real, XkbKeyNameLength);
        if (!HandleAliasDef(&def, def.merge, tmp->def.fileID, into))
            return False;
    }
    return True;
}

int
ApplyAliases(struct xkb_desc * xkb, Bool toGeom, AliasInfo ** info_in)
{
    int i;
    struct xkb_key_alias *old, *a;
    AliasInfo *info;
    int nNew, nOld;
    int status;

    if (*info_in == NULL)
        return True;
    if (toGeom)
    {
        nOld = (xkb->geom ? xkb->geom->num_key_aliases : 0);
        old = (xkb->geom ? xkb->geom->key_aliases : NULL);
    }
    else
    {
        nOld = (xkb->names ? xkb->names->num_key_aliases : 0);
        old = (xkb->names ? xkb->names->key_aliases : NULL);
    }
    for (nNew = 0, info = *info_in; info != NULL;
         info = (AliasInfo *) info->def.next)
    {
        unsigned long lname;
        xkb_keycode_t kc;

        lname = KeyNameToLong(info->real);
        if (!FindNamedKey(xkb, lname, &kc, False, CreateKeyNames(xkb), 0))
        {
            if (warningLevel > 4)
            {
                WARN("Attempt to alias %s to non-existent key %s\n",
                      XkbcKeyNameText(info->alias), XkbcKeyNameText(info->real));
                ACTION("Ignored\n");
            }
            info->alias[0] = '\0';
            continue;
        }
        lname = KeyNameToLong(info->alias);
        if (FindNamedKey(xkb, lname, &kc, False, False, 0))
        {
            if (warningLevel > 4)
            {
                WARN("Attempt to create alias with the name of a real key\n");
                ACTION("Alias \"%s = %s\" ignored\n",
                        XkbcKeyNameText(info->alias),
                        XkbcKeyNameText(info->real));
            }
            info->alias[0] = '\0';
            continue;
        }
        nNew++;
        if (old)
        {
            for (i = 0, a = old; i < nOld; i++, a++)
            {
                if (strncmp(a->alias, info->alias, XkbKeyNameLength) == 0)
                {
                    AliasInfo old_info;
                    InitAliasInfo(&old_info, MergeAugment, 0, a->alias, a->real);
                    HandleCollision(&old_info, info);
                    memcpy(old_info.real, a->real, XkbKeyNameLength);
                    info->alias[0] = '\0';
                    nNew--;
                    break;
                }
            }
        }
    }
    if (nNew == 0)
    {
        ClearCommonInfo(&(*info_in)->def);
        *info_in = NULL;
        return True;
    }
    if (toGeom)
    {
        if (!xkb->geom)
        {
            struct xkb_geometry_sizes sizes;
            bzero((char *) &sizes, sizeof(struct xkb_geometry_sizes));
            sizes.which = XkbGeomKeyAliasesMask;
            sizes.num_key_aliases = nOld + nNew;
            status = XkbcAllocGeometry(xkb, &sizes);
        }
        else
        {
            status = XkbcAllocGeomKeyAliases(xkb->geom, nOld + nNew);
        }
        if (xkb->geom)
            old = xkb->geom->key_aliases;
    }
    else
    {
        status = XkbcAllocNames(xkb, XkbKeyAliasesMask, 0, nOld + nNew);
        if (xkb->names)
            old = xkb->names->key_aliases;
    }
    if (status != Success)
    {
        WSGO("Allocation failure in ApplyAliases\n");
        return False;
    }
    if (toGeom)
        a = &xkb->geom->key_aliases[nOld];
    else
        a = xkb->names ? &xkb->names->key_aliases[nOld] : NULL;
    for (info = *info_in; info != NULL; info = (AliasInfo *) info->def.next)
    {
        if (info->alias[0] != '\0')
        {
            strncpy(a->alias, info->alias, XkbKeyNameLength);
            strncpy(a->real, info->real, XkbKeyNameLength);
            a++;
        }
    }
#ifdef DEBUG
    if ((a - old) != (nOld + nNew))
    {
        WSGO("Expected %d aliases total but created %d\n", nOld + nNew,
              a - old);
    }
#endif
    if (toGeom)
        xkb->geom->num_key_aliases += nNew;
    ClearCommonInfo(&(*info_in)->def);
    *info_in = NULL;
    return True;
}
