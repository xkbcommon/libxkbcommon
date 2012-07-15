/************************************************************
 * Copyright (c) 1995 by Silicon Graphics Computer Systems, Inc.
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

#include "alias.h"
#include "keycodes.h"

static void
HandleCollision(AliasInfo * old, AliasInfo * new)
{
    if (strncmp(new->real, old->real, XkbKeyNameLength) == 0) {
        if (((new->def.file_id == old->def.file_id) && (warningLevel > 0)) ||
            (warningLevel > 9)) {
            WARN("Alias of %s for %s declared more than once\n",
                 XkbcKeyNameText(new->alias), XkbcKeyNameText(new->real));
            ACTION("First definition ignored\n");
        }
    }
    else {
        char *use, *ignore;
        if (new->def.merge == MERGE_AUGMENT) {
            use = old->real;
            ignore = new->real;
        }
        else {
            use = new->real;
            ignore = old->real;
        }
        if (((old->def.file_id == new->def.file_id) && (warningLevel > 0)) ||
            (warningLevel > 9)) {
            WARN("Multiple definitions for alias %s\n",
                 XkbcKeyNameText(old->alias));
            ACTION("Using %s, ignoring %s\n",
                   XkbcKeyNameText(use), XkbcKeyNameText(ignore));
        }
        if (use != old->real)
            memcpy(old->real, use, XkbKeyNameLength);
    }
    old->def.file_id = new->def.file_id;
    old->def.merge = new->def.merge;
}

static void
InitAliasInfo(AliasInfo * info,
              enum merge_mode merge, unsigned file_id, char *alias,
              char *real)
{
    memset(info, 0, sizeof(AliasInfo));
    info->def.merge = merge;
    info->def.file_id = file_id;
    strncpy(info->alias, alias, XkbKeyNameLength);
    strncpy(info->real, real, XkbKeyNameLength);
}

int
HandleAliasDef(KeyAliasDef * def,
               enum merge_mode merge, unsigned file_id, AliasInfo ** info_in)
{
    AliasInfo *info;

    for (info = *info_in; info != NULL; info =
             (AliasInfo *) info->def.next) {
        if (strncmp(info->alias, def->alias, XkbKeyNameLength) == 0) {
            AliasInfo new;
            InitAliasInfo(&new, merge, file_id, def->alias, def->real);
            HandleCollision(info, &new);
            return true;
        }
    }
    info = uTypedCalloc(1, AliasInfo);
    if (info == NULL) {
        WSGO("Allocation failure in HandleAliasDef\n");
        return false;
    }
    info->def.file_id = file_id;
    info->def.merge = merge;
    info->def.next = (CommonInfo *) *info_in;
    memcpy(info->alias, def->alias, XkbKeyNameLength);
    memcpy(info->real, def->real, XkbKeyNameLength);
    *info_in = AddCommonInfo(&(*info_in)->def, &info->def);
    return true;
}

void
ClearAliases(AliasInfo ** info_in)
{
    if ((info_in) && (*info_in))
        ClearCommonInfo(&(*info_in)->def);
}

bool
MergeAliases(AliasInfo ** into, AliasInfo ** merge,
             enum merge_mode how_merge)
{
    AliasInfo *tmp;
    KeyAliasDef def;

    if ((*merge) == NULL)
        return true;
    if ((*into) == NULL) {
        *into = *merge;
        *merge = NULL;
        return true;
    }
    memset(&def, 0, sizeof(KeyAliasDef));
    for (tmp = *merge; tmp != NULL; tmp = (AliasInfo *) tmp->def.next) {
        if (how_merge == MERGE_DEFAULT)
            def.merge = tmp->def.merge;
        else
            def.merge = how_merge;
        memcpy(def.alias, tmp->alias, XkbKeyNameLength);
        memcpy(def.real, tmp->real, XkbKeyNameLength);
        if (!HandleAliasDef(&def, def.merge, tmp->def.file_id, into))
            return false;
    }
    return true;
}

int
ApplyAliases(struct xkb_keymap *keymap, AliasInfo ** info_in)
{
    int i;
    struct xkb_key *key;
    struct xkb_key_alias *old, *a;
    AliasInfo *info;
    int nNew, nOld;

    nOld = darray_size(keymap->key_aliases);
    old = &darray_item(keymap->key_aliases, 0);

    for (nNew = 0, info = *info_in; info;
         info = (AliasInfo *)info->def.next) {
        unsigned long lname;

        lname = KeyNameToLong(info->real);
        key = FindNamedKey(keymap, lname, false, CreateKeyNames(keymap), 0);
        if (!key) {
            if (warningLevel > 4) {
                WARN("Attempt to alias %s to non-existent key %s\n",
                     XkbcKeyNameText(info->alias), XkbcKeyNameText(info->real));
                ACTION("Ignored\n");
            }
            info->alias[0] = '\0';
            continue;
        }

        lname = KeyNameToLong(info->alias);
        key = FindNamedKey(keymap, lname, false, false, 0);
        if (key) {
            if (warningLevel > 4) {
                WARN("Attempt to create alias with the name of a real key\n");
                ACTION("Alias \"%s = %s\" ignored\n",
                       XkbcKeyNameText(info->alias),
                       XkbcKeyNameText(info->real));
            }
            info->alias[0] = '\0';
            continue;
        }

        nNew++;

        if (!old)
            continue;

        for (i = 0, a = old; i < nOld; i++, a++) {
            AliasInfo old_info;

            if (strncmp(a->alias, info->alias, XkbKeyNameLength) != 0)
                continue;

            InitAliasInfo(&old_info, MERGE_AUGMENT, 0, a->alias, a->real);
            HandleCollision(&old_info, info);
            memcpy(old_info.real, a->real, XkbKeyNameLength);
            info->alias[0] = '\0';
            nNew--;
            break;
        }
    }

    if (nNew == 0)
        goto out;

    darray_resize0(keymap->key_aliases, nOld + nNew);

    a = &darray_item(keymap->key_aliases, nOld);
    for (info = *info_in; info; info = (AliasInfo *)info->def.next) {
        if (info->alias[0] != '\0') {
            strncpy(a->alias, info->alias, XkbKeyNameLength);
            strncpy(a->real, info->real, XkbKeyNameLength);
            a++;
        }
    }

out:
    ClearCommonInfo(&(*info_in)->def);
    *info_in = NULL;
    return true;
}
