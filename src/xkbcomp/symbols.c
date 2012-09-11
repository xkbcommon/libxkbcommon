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

/*
 * Copyright © 2012 Intel Corporation
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 *         Ran Benita <ran234@gmail.com>
 */

#include "xkbcomp-priv.h"
#include "text.h"
#include "expr.h"
#include "action.h"
#include "vmod.h"
#include "keycodes.h"
#include "include.h"

enum key_repeat {
    KEY_REPEAT_YES = 1,
    KEY_REPEAT_NO = 0,
    KEY_REPEAT_UNDEFINED = -1
};

enum group_field {
    GROUP_FIELD_SYMS = (1 << 0),
    GROUP_FIELD_ACTS = (1 << 1),
    GROUP_FIELD_TYPE = (1 << 2),
};

enum key_field {
    KEY_FIELD_REPEAT    = (1 << 0),
    KEY_FIELD_TYPE_DFLT = (1 << 1),
    KEY_FIELD_GROUPINFO = (1 << 2),
    KEY_FIELD_VMODMAP   = (1 << 3),
};

typedef struct {
    enum group_field defined;

    xkb_level_index_t numLevels;
    darray(xkb_keysym_t) syms;
    /*
     * symsMapIndex[level] -> The index from which the syms for the
     * level begin in the syms array. Remember each keycode can have
     * multiple keysyms in each level (that is, each key press can
     * result in multiple keysyms).
     */
    darray(int) symsMapIndex;
    /*
     * symsMapNumEntries[level] -> How many syms are in
     * syms[symsMapIndex[level]].
     */
    darray(size_t) symsMapNumEntries;
    darray(union xkb_action) acts;
    xkb_atom_t type;
} GroupInfo;

typedef struct _KeyInfo {
    enum key_field defined;
    unsigned file_id;
    enum merge_mode merge;

    unsigned long name; /* the 4 chars of the key name, as long */

    GroupInfo groups[XKB_NUM_GROUPS];

    enum key_repeat repeat;
    xkb_mod_mask_t vmodmap;
    xkb_atom_t dfltType;

    enum xkb_range_exceed_type out_of_range_group_action;
    xkb_group_index_t out_of_range_group_number;
} KeyInfo;

static void
InitGroupInfo(GroupInfo *groupi)
{
    memset(groupi, 0, sizeof(*groupi));
}

static void
ClearGroupInfo(GroupInfo *groupi)
{
    darray_free(groupi->syms);
    darray_free(groupi->symsMapIndex);
    darray_free(groupi->symsMapNumEntries);
    darray_free(groupi->acts);
    InitGroupInfo(groupi);
}

static void
InitKeyInfo(KeyInfo *keyi, unsigned file_id)
{
    xkb_group_index_t i;
    static const char dflt[4] = "*";

    keyi->defined = 0;
    keyi->file_id = file_id;
    keyi->merge = MERGE_OVERRIDE;
    keyi->name = KeyNameToLong(dflt);
    for (i = 0; i < XKB_NUM_GROUPS; i++)
        InitGroupInfo(&keyi->groups[i]);
    keyi->dfltType = XKB_ATOM_NONE;
    keyi->vmodmap = 0;
    keyi->repeat = KEY_REPEAT_UNDEFINED;
    keyi->out_of_range_group_action = RANGE_WRAP;
    keyi->out_of_range_group_number = 0;
}

static void
ClearKeyInfo(KeyInfo *keyi)
{
    xkb_group_index_t i;

    for (i = 0; i < XKB_NUM_GROUPS; i++)
        ClearGroupInfo(&keyi->groups[i]);
}

static bool
CopyKeyInfo(KeyInfo * old, KeyInfo * new, bool clearOld)
{
    xkb_group_index_t i;

    *new = *old;

    if (clearOld) {
        for (i = 0; i < XKB_NUM_GROUPS; i++) {
            InitGroupInfo(&old->groups[i]);
        }
    }
    else {
        for (i = 0; i < XKB_NUM_GROUPS; i++) {
            GroupInfo *n = &new->groups[i], *o = &old->groups[i];
            darray_copy(n->syms, o->syms);
            darray_copy(n->symsMapIndex, o->symsMapIndex);
            darray_copy(n->symsMapNumEntries, o->symsMapNumEntries);
            darray_copy(n->acts, o->acts);
        }
    }

    return true;
}

/***====================================================================***/

typedef struct _ModMapEntry {
    enum merge_mode merge;
    bool haveSymbol;
    int modifier;
    union {
        unsigned long keyName;
        xkb_keysym_t keySym;
    } u;
} ModMapEntry;

typedef struct _SymbolsInfo {
    char *name;         /* e.g. pc+us+inet(evdev) */
    int errorCount;
    unsigned file_id;
    enum merge_mode merge;
    xkb_group_index_t explicit_group;
    darray(KeyInfo) keys;
    KeyInfo dflt;
    VModInfo vmods;
    ActionsInfo *actions;
    xkb_atom_t groupNames[XKB_NUM_GROUPS];
    darray(ModMapEntry) modMaps;

    struct xkb_keymap *keymap;
} SymbolsInfo;

static void
InitSymbolsInfo(SymbolsInfo *info, struct xkb_keymap *keymap,
                unsigned file_id, ActionsInfo *actions)
{
    xkb_group_index_t i;

    info->name = NULL;
    info->explicit_group = 0;
    info->errorCount = 0;
    info->file_id = file_id;
    info->merge = MERGE_OVERRIDE;
    darray_init(info->keys);
    darray_growalloc(info->keys, 110);
    darray_init(info->modMaps);
    for (i = 0; i < XKB_NUM_GROUPS; i++)
        info->groupNames[i] = XKB_ATOM_NONE;
    InitKeyInfo(&info->dflt, file_id);
    InitVModInfo(&info->vmods, keymap);
    info->actions = actions;
    info->keymap = keymap;
}

static void
ClearSymbolsInfo(SymbolsInfo * info)
{
    KeyInfo *keyi;

    free(info->name);
    darray_foreach(keyi, info->keys)
        ClearKeyInfo(keyi);
    darray_free(info->keys);
    darray_free(info->modMaps);
    memset(info, 0, sizeof(SymbolsInfo));
}

static bool
ResizeGroupInfo(GroupInfo *groupi, xkb_level_index_t numLevels,
                unsigned sizeSyms, bool forceActions)
{
    xkb_level_index_t i;

    if (darray_size(groupi->syms) < sizeSyms)
        darray_resize0(groupi->syms, sizeSyms);

    if (darray_empty(groupi->symsMapIndex) ||
        groupi->numLevels < numLevels) {
        darray_resize(groupi->symsMapIndex, numLevels);
        for (i = groupi->numLevels; i < numLevels; i++)
            darray_item(groupi->symsMapIndex, i) = -1;
    }

    if (darray_empty(groupi->symsMapNumEntries) ||
        groupi->numLevels < numLevels)
        darray_resize0(groupi->symsMapNumEntries, numLevels);

    if ((forceActions && (groupi->numLevels < numLevels ||
                          darray_empty(groupi->acts))) ||
        (groupi->numLevels < numLevels && !darray_empty(groupi->acts)))
        darray_resize0(groupi->acts, numLevels);

    groupi->numLevels = MAX(groupi->numLevels, numLevels);

    return true;
}

enum key_group_selector {
    NONE = 0,
    FROM = (1 << 0),
    TO = (1 << 1),
};

/*
 * Merge @from into @into, where both are groups with the same index
 * for the same key, and have at least on level to merge.
 * @group and @key_name are just for reporting.
 */
static bool
MergeGroups(SymbolsInfo *info, GroupInfo *into, GroupInfo *from, bool clobber,
            bool report, xkb_group_index_t group, unsigned long key_name)
{
    GroupInfo result;
    unsigned int resultSize = 0;
    enum key_group_selector using = NONE;
    size_t cur_idx = 0;
    xkb_level_index_t i;

    /* First find the type of the merged group. */
    if (into->type != from->type && from->type != XKB_ATOM_NONE) {
        if (into->type == XKB_ATOM_NONE) {
            into->type = from->type;
        }
        else {
            xkb_atom_t use, ignore;

            use = (clobber ? from->type : into->type);
            ignore = (clobber ? into->type : from->type);

            if (report)
                log_warn(info->keymap->ctx,
                         "Multiple definitions for group %d type of key %s; "
                         "Using %s, ignoring %s\n",
                         group + 1, LongKeyNameText(key_name),
                         xkb_atom_text(info->keymap->ctx, use),
                         xkb_atom_text(info->keymap->ctx, ignore));

            into->type = use;
        }

        into->defined |= GROUP_FIELD_TYPE;
    }

    /* Now look at the levels. */

    /* Easy case: @from is empty. Just stick with @into. */
    if (from->numLevels == 0) {
        InitGroupInfo(from);
        return true;
    }

    /* Easy case: @into is empty. Just copy @from as-is. */
    if (into->numLevels == 0) {
        from->type = into->type;
        *into = *from;
        InitGroupInfo(from);
        return true;
    }

    /*
     * Hard case: need to start merging levels. The aim is to avoid
     * copying/allocating where possible.
     */

    InitGroupInfo(&result);

    if (into->numLevels >= from->numLevels) {
        result.acts = into->acts;
        result.numLevels = into->numLevels;
    }
    else {
        result.acts = from->acts;
        result.numLevels = from->numLevels;
        darray_resize(into->symsMapIndex, from->numLevels);
        darray_resize0(into->symsMapNumEntries, from->numLevels);
        for (i = into->numLevels; i < from->numLevels; i++)
            darray_item(into->symsMapIndex, i) = -1;
    }

    if (darray_empty(result.acts) && (!darray_empty(into->acts) ||
                                      !darray_empty(from->acts))) {
        darray_resize0(result.acts, result.numLevels);
        for (i = 0; i < result.numLevels; i++) {
            union xkb_action *fromAct = NULL, *toAct = NULL;

            if (!darray_empty(from->acts))
                fromAct = &darray_item(from->acts, i);

            if (!darray_empty(into->acts))
                toAct = &darray_item(into->acts, i);

            if (((fromAct == NULL) || (fromAct->type == ACTION_TYPE_NONE))
                && (toAct != NULL)) {
                darray_item(result.acts, i) = *toAct;
            }
            else if (((toAct == NULL) || (toAct->type == ACTION_TYPE_NONE))
                     && (fromAct != NULL)) {
                darray_item(result.acts, i) = *fromAct;
            }
            else {
                union xkb_action *use, *ignore;
                if (clobber) {
                    use = fromAct;
                    ignore = toAct;
                }
                else {
                    use = toAct;
                    ignore = fromAct;
                }
                if (report)
                    log_warn(info->keymap->ctx,
                             "Multiple actions for level %d/group %u on key %s; "
                             "Using %s, ignoring %s\n",
                             i + 1, group + 1, LongKeyNameText(key_name),
                             ActionTypeText(use->type),
                             ActionTypeText(ignore->type));
                if (use)
                    darray_item(result.acts, i) = *use;
            }
        }
    }

    for (i = 0; i < result.numLevels; i++) {
        unsigned int fromSize = 0;
        unsigned int toSize = 0;

        if (!darray_empty(from->symsMapNumEntries) && i < from->numLevels)
            fromSize = darray_item(from->symsMapNumEntries, i);

        if (!darray_empty(into->symsMapNumEntries) && i < into->numLevels)
            toSize = darray_item(into->symsMapNumEntries, i);

        if (fromSize == 0) {
            resultSize += toSize;
            using |= TO;
        }
        else if (toSize == 0 || clobber) {
            resultSize += fromSize;
            using |= FROM;
        }
        else {
            resultSize += toSize;
            using |= TO;
        }
    }

    if (resultSize == 0)
        goto out;

    if (using == FROM) {
        result.syms = from->syms;
        darray_free(into->symsMapNumEntries);
        darray_free(into->symsMapIndex);
        into->symsMapNumEntries = from->symsMapNumEntries;
        into->symsMapIndex = from->symsMapIndex;
        darray_init(from->symsMapNumEntries);
        darray_init(from->symsMapIndex);
        goto out;
    }
    else if (using == TO) {
        result.syms = into->syms;
        goto out;
    }

    darray_resize0(result.syms, resultSize);

    for (i = 0; i < result.numLevels; i++) {
        enum key_group_selector use = NONE;
        unsigned int fromSize = 0;
        unsigned int toSize = 0;

        if (i < from->numLevels)
            fromSize = darray_item(from->symsMapNumEntries, i);

        if (i < into->numLevels)
            toSize = darray_item(into->symsMapNumEntries, i);

        if (fromSize == 0 && toSize == 0) {
            darray_item(into->symsMapIndex, i) = -1;
            darray_item(into->symsMapNumEntries, i) = 0;
            continue;
        }

        if (fromSize == 0)
            use = TO;
        else if (toSize == 0 || clobber)
            use = FROM;
        else
            use = TO;

        if (toSize && fromSize && report) {
            log_info(info->keymap->ctx,
                     "Multiple symbols for group %u, level %d on key %s; "
                     "Using %s, ignoring %s\n",
                     group + 1, i + 1, LongKeyNameText(key_name),
                     (use == FROM ? "from" : "to"),
                     (use == FROM ? "to" : "from"));
        }

        if (use == FROM) {
            memcpy(darray_mem(result.syms, cur_idx),
                   darray_mem(from->syms, darray_item(from->symsMapIndex, i)),
                   darray_item(from->symsMapNumEntries, i) * sizeof(xkb_keysym_t));
            darray_item(into->symsMapIndex, i) = cur_idx;
            darray_item(into->symsMapNumEntries, i) =
                darray_item(from->symsMapNumEntries, i);
        }
        else {
            memcpy(darray_mem(result.syms, cur_idx),
                   darray_mem(into->syms, darray_item(into->symsMapIndex, i)),
                   darray_item(into->symsMapNumEntries, i) * sizeof(xkb_keysym_t));
            darray_item(into->symsMapIndex, i) = cur_idx;
        }

        cur_idx += darray_item(into->symsMapNumEntries, i);
    }

out:
    into->numLevels = result.numLevels;

    if (!darray_same(result.acts, into->acts))
        darray_free(into->acts);
    if (!darray_same(result.acts, from->acts))
        darray_free(from->acts);
    into->acts = result.acts;
    into->defined |= GROUP_FIELD_ACTS;
    darray_init(from->acts);
    from->defined &= ~GROUP_FIELD_ACTS;

    if (!darray_same(result.syms, into->syms))
        darray_free(into->syms);
    if (!darray_same(result.syms, from->syms))
        darray_free(from->syms);
    into->syms = result.syms;
    if (darray_empty(into->syms))
        into->defined &= ~GROUP_FIELD_SYMS;
    else
        into->defined |= GROUP_FIELD_SYMS;
    darray_init(from->syms);
    from->defined &= ~GROUP_FIELD_SYMS;

    darray_free(from->symsMapIndex);
    darray_free(from->symsMapNumEntries);

    return true;
}

static bool
UseNewKeyField(enum key_field field, enum key_field old, enum key_field new,
               bool clobber, bool report, enum key_field *collide)
{
    if (!(old & field))
        return (new & field);

    if (new & field) {
        if (report)
            *collide |= field;

        if (clobber)
            return true;
    }

    return false;
}


/* Merge @from into @into, where both contain info for the same key. */
static bool
MergeKeys(SymbolsInfo *info, KeyInfo *into, KeyInfo *from)
{
    xkb_group_index_t i;
    enum key_field collide = 0;
    bool clobber, report;
    int verbosity = xkb_get_log_verbosity(info->keymap->ctx);

    /* Forget about @into; just copy @from as-is. */
    if (from->merge == MERGE_REPLACE) {
        for (i = 0; i < XKB_NUM_GROUPS; i++)
            if (into->groups[i].numLevels != 0)
                ClearGroupInfo(&into->groups[i]);
        *into = *from;
        InitKeyInfo(from, info->file_id);
        return true;
    }

    clobber = (from->merge != MERGE_AUGMENT);
    report = (verbosity > 9 ||
              (into->file_id == from->file_id && verbosity > 0));

    /* Start looking into the key groups. */
    for (i = 0; i < XKB_NUM_GROUPS; i++)
        MergeGroups(info, &into->groups[i], &from->groups[i], clobber,
                    report, i, into->name);

    if (UseNewKeyField(KEY_FIELD_VMODMAP, into->defined, from->defined,
                       clobber, report, &collide)) {
        into->vmodmap = from->vmodmap;
        into->defined |= KEY_FIELD_VMODMAP;
    }
    if (UseNewKeyField(KEY_FIELD_REPEAT, into->defined, from->defined,
                       clobber, report, &collide)) {
        into->repeat = from->repeat;
        into->defined |= KEY_FIELD_REPEAT;
    }
    if (UseNewKeyField(KEY_FIELD_TYPE_DFLT, into->defined, from->defined,
                       clobber, report, &collide)) {
        into->dfltType = from->dfltType;
        into->defined |= KEY_FIELD_TYPE_DFLT;
    }
    if (UseNewKeyField(KEY_FIELD_GROUPINFO, into->defined, from->defined,
                       clobber, report, &collide)) {
        into->out_of_range_group_action = from->out_of_range_group_action;
        into->out_of_range_group_number = from->out_of_range_group_number;
        into->defined |= KEY_FIELD_GROUPINFO;
    }

    if (collide)
        log_warn(info->keymap->ctx,
                 "Symbol map for key %s redefined; "
                 "Using %s definition for conflicting fields\n",
                 LongKeyNameText(into->name),
                 (clobber ? "first" : "last"));

    return true;
}

static bool
AddKeySymbols(SymbolsInfo *info, KeyInfo *keyi)
{
    unsigned long real_name;
    KeyInfo *iter, *new;

    darray_foreach(iter, info->keys)
        if (iter->name == keyi->name)
            return MergeKeys(info, iter, keyi);

    if (FindKeyNameForAlias(info->keymap, keyi->name, &real_name))
        darray_foreach(iter, info->keys)
            if (iter->name == real_name)
                return MergeKeys(info, iter, keyi);

    darray_resize0(info->keys, darray_size(info->keys) + 1);
    new = &darray_item(info->keys, darray_size(info->keys) - 1);
    return CopyKeyInfo(keyi, new, true);
}

static bool
AddModMapEntry(SymbolsInfo * info, ModMapEntry * new)
{
    ModMapEntry *mm;
    bool clobber;

    clobber = (new->merge != MERGE_AUGMENT);
    darray_foreach(mm, info->modMaps) {
        if (new->haveSymbol && mm->haveSymbol
            && (new->u.keySym == mm->u.keySym)) {
            unsigned use, ignore;
            if (mm->modifier != new->modifier) {
                if (clobber) {
                    use = new->modifier;
                    ignore = mm->modifier;
                }
                else {
                    use = mm->modifier;
                    ignore = new->modifier;
                }
                log_err(info->keymap->ctx,
                        "%s added to symbol map for multiple modifiers; "
                        "Using %s, ignoring %s.\n",
                        KeysymText(new->u.keySym), ModIndexText(use),
                        ModIndexText(ignore));
                mm->modifier = use;
            }
            return true;
        }
        if ((!new->haveSymbol) && (!mm->haveSymbol) &&
            (new->u.keyName == mm->u.keyName)) {
            unsigned use, ignore;
            if (mm->modifier != new->modifier) {
                if (clobber) {
                    use = new->modifier;
                    ignore = mm->modifier;
                }
                else {
                    use = mm->modifier;
                    ignore = new->modifier;
                }
                log_err(info->keymap->ctx,
                        "Key %s added to map for multiple modifiers; "
                        "Using %s, ignoring %s.\n",
                        LongKeyNameText(new->u.keyName), ModIndexText(use),
                        ModIndexText(ignore));
                mm->modifier = use;
            }
            return true;
        }
    }

    darray_append(info->modMaps, *new);
    return true;
}

/***====================================================================***/

static void
MergeIncludedSymbols(SymbolsInfo *into, SymbolsInfo *from,
                     enum merge_mode merge)
{
    unsigned int i;
    KeyInfo *keyi;
    ModMapEntry *mm;

    if (from->errorCount > 0) {
        into->errorCount += from->errorCount;
        return;
    }
    if (into->name == NULL) {
        into->name = from->name;
        from->name = NULL;
    }
    for (i = 0; i < XKB_NUM_GROUPS; i++) {
        if (from->groupNames[i] != XKB_ATOM_NONE) {
            if ((merge != MERGE_AUGMENT) ||
                (into->groupNames[i] == XKB_ATOM_NONE))
                into->groupNames[i] = from->groupNames[i];
        }
    }

    darray_foreach(keyi, from->keys) {
        merge = (merge == MERGE_DEFAULT ? keyi->merge : merge);
        if (!AddKeySymbols(into, keyi))
            into->errorCount++;
    }

    darray_foreach(mm, from->modMaps) {
        mm->merge = (merge == MERGE_DEFAULT ? mm->merge : merge);
        if (!AddModMapEntry(into, mm))
            into->errorCount++;
    }
}

static void
HandleSymbolsFile(SymbolsInfo *info, XkbFile *file, enum merge_mode merge);

static bool
HandleIncludeSymbols(SymbolsInfo *info, IncludeStmt *stmt)
{
    enum merge_mode merge = MERGE_DEFAULT;
    XkbFile *rtrn;
    SymbolsInfo included, next_incl;

    InitSymbolsInfo(&included, info->keymap, info->file_id, info->actions);
    if (stmt->stmt) {
        free(included.name);
        included.name = stmt->stmt;
        stmt->stmt = NULL;
    }

    for (; stmt; stmt = stmt->next_incl) {
        if (!ProcessIncludeFile(info->keymap->ctx, stmt, FILE_TYPE_SYMBOLS,
                                &rtrn, &merge)) {
            info->errorCount += 10;
            ClearSymbolsInfo(&included);
            return false;
        }

        InitSymbolsInfo(&next_incl, info->keymap, rtrn->id, info->actions);
        next_incl.merge = next_incl.dflt.merge = MERGE_OVERRIDE;
        if (stmt->modifier)
            next_incl.explicit_group = atoi(stmt->modifier) - 1;
        else
            next_incl.explicit_group = info->explicit_group;

        HandleSymbolsFile(&next_incl, rtrn, MERGE_OVERRIDE);

        MergeIncludedSymbols(&included, &next_incl, merge);

        ClearSymbolsInfo(&next_incl);
        FreeXkbFile(rtrn);
    }

    MergeIncludedSymbols(info, &included, merge);
    ClearSymbolsInfo(&included);

    return (info->errorCount == 0);
}

#define SYMBOLS 1
#define ACTIONS 2

static bool
GetGroupIndex(SymbolsInfo *info, KeyInfo *keyi, ExprDef *arrayNdx,
              unsigned what, xkb_group_index_t *ndx_rtrn)
{
    const char *name = (what == SYMBOLS ? "symbols" : "actions");

    if (arrayNdx == NULL) {
        xkb_group_index_t i;
        enum group_field field = (what == SYMBOLS ?
                                  GROUP_FIELD_SYMS : GROUP_FIELD_ACTS);

        for (i = 0; i < XKB_NUM_GROUPS; i++) {
            if (!(keyi->groups[i].defined & field)) {
                *ndx_rtrn = i;
                return true;
            }
        }

        log_err(info->keymap->ctx,
                "Too many groups of %s for key %s (max %u); "
                "Ignoring %s defined for extra groups\n",
                name, LongKeyNameText(keyi->name), XKB_NUM_GROUPS + 1, name);
        return false;
    }

    if (!ExprResolveGroup(info->keymap->ctx, arrayNdx, ndx_rtrn)) {
        log_err(info->keymap->ctx,
                "Illegal group index for %s of key %s\n"
                "Definition with non-integer array index ignored\n",
                name, LongKeyNameText(keyi->name));
        return false;
    }

    (*ndx_rtrn)--;
    return true;
}

bool
LookupKeysym(const char *str, xkb_keysym_t *sym_rtrn)
{
    xkb_keysym_t sym;

    if (!str || istreq(str, "any") || istreq(str, "nosymbol")) {
        *sym_rtrn = XKB_KEY_NoSymbol;
        return 1;
    }

    if (istreq(str, "none") || istreq(str, "voidsymbol")) {
        *sym_rtrn = XKB_KEY_VoidSymbol;
        return 1;
    }

    sym = xkb_keysym_from_name(str);
    if (sym != XKB_KEY_NoSymbol) {
        *sym_rtrn = sym;
        return 1;
    }

    return 0;
}

static bool
AddSymbolsToKey(SymbolsInfo *info, KeyInfo *keyi, ExprDef *arrayNdx,
                ExprDef *value)
{
    xkb_group_index_t ndx;
    GroupInfo *groupi;
    size_t nSyms;
    xkb_level_index_t nLevels;
    xkb_level_index_t i;
    int j;

    if (!GetGroupIndex(info, keyi, arrayNdx, SYMBOLS, &ndx))
        return false;

    groupi = &keyi->groups[ndx];

    if (value == NULL) {
        groupi->defined |= GROUP_FIELD_SYMS;
        return true;
    }

    if (value->op != EXPR_KEYSYM_LIST) {
        log_err(info->keymap->ctx,
                "Expected a list of symbols, found %s; "
                "Ignoring symbols for group %u of %s\n",
                expr_op_type_to_string(value->op), ndx + 1,
                LongKeyNameText(keyi->name));
        return false;
    }

    if (!darray_empty(groupi->syms)) {
        log_err(info->keymap->ctx,
                "Symbols for key %s, group %u already defined; "
                "Ignoring duplicate definition\n",
                LongKeyNameText(keyi->name), ndx + 1);
        return false;
    }

    nSyms = darray_size(value->value.list.syms);
    nLevels = darray_size(value->value.list.symsMapIndex);

    if ((groupi->numLevels < nSyms || darray_empty(groupi->syms))) {
        if (!ResizeGroupInfo(groupi, nLevels, nSyms, false)) {
            log_wsgo(info->keymap->ctx,
                     "Could not resize group %u of key %s to contain %zu levels; "
                     "Symbols lost\n",
                     ndx + 1, LongKeyNameText(keyi->name), nSyms);
            return false;
        }
    }

    groupi->defined |= GROUP_FIELD_SYMS;

    for (i = 0; i < nLevels; i++) {
        darray_item(groupi->symsMapIndex, i) =
            darray_item(value->value.list.symsMapIndex, i);
        darray_item(groupi->symsMapNumEntries, i) =
            darray_item(value->value.list.symsNumEntries, i);

        for (j = 0; j < darray_item(groupi->symsMapNumEntries, i); j++) {
            /* FIXME: What's abort() doing here? */
            if (darray_item(groupi->symsMapIndex, i) + j >= nSyms)
                abort();

            if (!LookupKeysym(darray_item(value->value.list.syms,
                                          darray_item(value->value.list.symsMapIndex, i) + j),
                              &darray_item(groupi->syms,
                                           darray_item(groupi->symsMapIndex, i) + j))) {
                log_warn(info->keymap->ctx,
                         "Could not resolve keysym %s for key %s, group %u (%s), level %zu\n",
                         darray_item(value->value.list.syms, i),
                         LongKeyNameText(keyi->name),
                         ndx + 1,
                         xkb_atom_text(info->keymap->ctx,
                                       info->groupNames[ndx]),
                         nSyms);

                while (--j >= 0)
                    darray_item(groupi->syms,
                                darray_item(groupi->symsMapIndex, i) + j) = XKB_KEY_NoSymbol;

                darray_item(groupi->symsMapIndex, i) = -1;
                darray_item(groupi->symsMapNumEntries, i) = 0;
                break;
            }

            if (darray_item(groupi->symsMapNumEntries, i) == 1 &&
                darray_item(groupi->syms,
                            darray_item(groupi->symsMapIndex, i) + j) == XKB_KEY_NoSymbol) {
                darray_item(groupi->symsMapIndex, i) = -1;
                darray_item(groupi->symsMapNumEntries, i) = 0;
            }
        }
    }

    for (j = groupi->numLevels - 1;
         j >= 0 && darray_item(groupi->symsMapNumEntries, j) == 0; j--)
        groupi->numLevels--;

    return true;
}

static bool
AddActionsToKey(SymbolsInfo *info, KeyInfo *keyi, ExprDef *arrayNdx,
                ExprDef *value)
{
    size_t i;
    xkb_group_index_t ndx;
    GroupInfo *groupi;
    size_t nActs;
    ExprDef *act;
    union xkb_action *toAct;

    if (!GetGroupIndex(info, keyi, arrayNdx, ACTIONS, &ndx))
        return false;

    groupi = &keyi->groups[ndx];

    if (value == NULL) {
        groupi->defined |= GROUP_FIELD_ACTS;
        return true;
    }

    if (value->op != EXPR_ACTION_LIST) {
        log_wsgo(info->keymap->ctx,
                 "Bad expression type (%d) for action list value; "
                 "Ignoring actions for group %u of %s\n",
                 value->op, ndx, LongKeyNameText(keyi->name));
        return false;
    }

    if (!darray_empty(groupi->acts)) {
        log_wsgo(info->keymap->ctx,
                 "Actions for key %s, group %u already defined\n",
                 LongKeyNameText(keyi->name), ndx);
        return false;
    }

    for (nActs = 0, act = value->value.child; act != NULL; nActs++) {
        act = (ExprDef *) act->common.next;
    }

    if (nActs < 1) {
        log_wsgo(info->keymap->ctx,
                 "Action list but not actions in AddActionsToKey\n");
        return false;
    }

    if ((groupi->numLevels < nActs || darray_empty(groupi->acts))) {
        if (!ResizeGroupInfo(&keyi->groups[ndx], nActs, nActs, true)) {
            log_wsgo(info->keymap->ctx,
                     "Could not resize group %u of key %s; "
                     "Actions lost\n",
                     ndx, LongKeyNameText(keyi->name));
            return false;
        }
    }

    groupi->defined |= GROUP_FIELD_ACTS;

    toAct = darray_mem(groupi->acts, 0);
    act = value->value.child;
    for (i = 0; i < nActs; i++, toAct++) {
        if (!HandleActionDef(act, info->keymap, toAct, info->actions)) {
            log_err(info->keymap->ctx,
                    "Illegal action definition for %s; "
                    "Action for group %u/level %zu ignored\n",
                    LongKeyNameText(keyi->name), ndx + 1, i + 1);
        }
        act = (ExprDef *) act->common.next;
    }

    return true;
}

static const LookupEntry repeatEntries[] = {
    { "true", KEY_REPEAT_YES },
    { "yes", KEY_REPEAT_YES },
    { "on", KEY_REPEAT_YES },
    { "false", KEY_REPEAT_NO },
    { "no", KEY_REPEAT_NO },
    { "off", KEY_REPEAT_NO },
    { "default", KEY_REPEAT_UNDEFINED },
    { NULL, 0 }
};

static bool
SetSymbolsField(SymbolsInfo *info, KeyInfo *keyi, const char *field,
                ExprDef *arrayNdx, ExprDef *value)
{
    bool ok = true;
    struct xkb_context *ctx = info->keymap->ctx;

    if (istreq(field, "type")) {
        xkb_group_index_t ndx;
        xkb_atom_t val;

        if (!ExprResolveString(ctx, value, &val))
            log_vrb(ctx, 1,
                    "The type field of a key symbol map must be a string; "
                    "Ignoring illegal type definition\n");

        if (arrayNdx == NULL) {
            keyi->dfltType = val;
            keyi->defined |= KEY_FIELD_TYPE_DFLT;
        }
        else if (!ExprResolveGroup(ctx, arrayNdx, &ndx)) {
            log_err(ctx,
                    "Illegal group index for type of key %s; "
                    "Definition with non-integer array index ignored\n",
                    LongKeyNameText(keyi->name));
            return false;
        }
        else {
            ndx--;
            keyi->groups[ndx].type = val;
            keyi->groups[ndx].defined |= GROUP_FIELD_TYPE;
        }
    }
    else if (istreq(field, "symbols"))
        return AddSymbolsToKey(info, keyi, arrayNdx, value);
    else if (istreq(field, "actions"))
        return AddActionsToKey(info, keyi, arrayNdx, value);
    else if (istreq(field, "vmods") ||
             istreq(field, "virtualmods") ||
             istreq(field, "virtualmodifiers")) {
        xkb_mod_mask_t mask;

        ok = ExprResolveVModMask(info->keymap, value, &mask);
        if (ok) {
            keyi->vmodmap = (mask >> XKB_NUM_CORE_MODS) & 0xffff;
            keyi->defined |= KEY_FIELD_VMODMAP;
        }
        else {
            log_err(info->keymap->ctx,
                    "Expected a virtual modifier mask, found %s; "
                    "Ignoring virtual modifiers definition for key %s\n",
                    expr_op_type_to_string(value->op),
                    LongKeyNameText(keyi->name));
        }
    }
    else if (istreq(field, "locking") ||
             istreq(field, "lock") ||
             istreq(field, "locks")) {
        log_err(info->keymap->ctx,
                "Key behaviors not supported; "
                "Ignoring locking specification for key %s\n",
                LongKeyNameText(keyi->name));
    }
    else if (istreq(field, "radiogroup") ||
             istreq(field, "permanentradiogroup") ||
             istreq(field, "allownone")) {
        log_err(info->keymap->ctx,
                "Radio groups not supported; "
                "Ignoring radio group specification for key %s\n",
                LongKeyNameText(keyi->name));
    }
    else if (istreq_prefix("overlay", field) ||
             istreq_prefix("permanentoverlay", field)) {
        log_err(info->keymap->ctx,
                "Overlays not supported; "
                "Ignoring overlay specification for key %s\n",
                LongKeyNameText(keyi->name));
    }
    else if (istreq(field, "repeating") ||
             istreq(field, "repeats") ||
             istreq(field, "repeat")) {
        unsigned int val;

        ok = ExprResolveEnum(ctx, value, &val, repeatEntries);
        if (!ok) {
            log_err(info->keymap->ctx,
                    "Illegal repeat setting for %s; "
                    "Non-boolean repeat setting ignored\n",
                    LongKeyNameText(keyi->name));
            return false;
        }
        keyi->repeat = val;
        keyi->defined |= KEY_FIELD_REPEAT;
    }
    else if (istreq(field, "groupswrap") ||
             istreq(field, "wrapgroups")) {
        bool set;

        if (!ExprResolveBoolean(ctx, value, &set)) {
            log_err(info->keymap->ctx,
                    "Illegal groupsWrap setting for %s; "
                    "Non-boolean value ignored\n",
                    LongKeyNameText(keyi->name));
            return false;
        }

        if (set)
            keyi->out_of_range_group_action = RANGE_WRAP;
        else
            keyi->out_of_range_group_action = RANGE_SATURATE;

        keyi->defined |= KEY_FIELD_GROUPINFO;
    }
    else if (istreq(field, "groupsclamp") ||
             istreq(field, "clampgroups")) {
        bool set;

        if (!ExprResolveBoolean(ctx, value, &set)) {
            log_err(info->keymap->ctx,
                    "Illegal groupsClamp setting for %s; "
                    "Non-boolean value ignored\n",
                    LongKeyNameText(keyi->name));
            return false;
        }

        if (set)
            keyi->out_of_range_group_action = RANGE_SATURATE;
        else
            keyi->out_of_range_group_action = RANGE_WRAP;

        keyi->defined |= KEY_FIELD_GROUPINFO;
    }
    else if (istreq(field, "groupsredirect") ||
             istreq(field, "redirectgroups")) {
        xkb_group_index_t grp;

        if (!ExprResolveGroup(ctx, value, &grp)) {
            log_err(info->keymap->ctx,
                    "Illegal group index for redirect of key %s; "
                    "Definition with non-integer group ignored\n",
                    LongKeyNameText(keyi->name));
            return false;
        }

        keyi->out_of_range_group_action = RANGE_REDIRECT;
        keyi->out_of_range_group_number = grp - 1;
        keyi->defined |= KEY_FIELD_GROUPINFO;
    }
    else {
        log_err(info->keymap->ctx,
                "Unknown field %s in a symbol interpretation; "
                "Definition ignored\n",
                field);
        ok = false;
    }

    return ok;
}

static int
SetGroupName(SymbolsInfo *info, ExprDef *arrayNdx, ExprDef *value)
{
    xkb_group_index_t grp;
    xkb_atom_t name;

    if (!arrayNdx) {
        log_vrb(info->keymap->ctx, 1,
                "You must specify an index when specifying a group name; "
                "Group name definition without array subscript ignored\n");
        return false;
    }

    if (!ExprResolveGroup(info->keymap->ctx, arrayNdx, &grp)) {
        log_err(info->keymap->ctx,
                "Illegal index in group name definition; "
                "Definition with non-integer array index ignored\n");
        return false;
    }

    if (!ExprResolveString(info->keymap->ctx, value, &name)) {
        log_err(info->keymap->ctx,
                "Group name must be a string; "
                "Illegal name for group %d ignored\n", grp);
        return false;
    }

    info->groupNames[grp - 1 + info->explicit_group] = name;
    return true;
}

static int
HandleSymbolsVar(SymbolsInfo *info, VarDef *stmt)
{
    const char *elem, *field;
    ExprDef *arrayNdx;
    bool ret;

    if (ExprResolveLhs(info->keymap->ctx, stmt->name, &elem, &field,
                       &arrayNdx) == 0)
        return 0;               /* internal error, already reported */
    if (elem && istreq(elem, "key")) {
        ret = SetSymbolsField(info, &info->dflt, field, arrayNdx,
                              stmt->value);
    }
    else if (!elem && (istreq(field, "name") ||
                       istreq(field, "groupname"))) {
        ret = SetGroupName(info, arrayNdx, stmt->value);
    }
    else if (!elem && (istreq(field, "groupswrap") ||
                       istreq(field, "wrapgroups"))) {
        log_err(info->keymap->ctx,
                "Global \"groupswrap\" not supported; Ignored\n");
        ret = true;
    }
    else if (!elem && (istreq(field, "groupsclamp") ||
                       istreq(field, "clampgroups"))) {
        log_err(info->keymap->ctx,
                "Global \"groupsclamp\" not supported; Ignored\n");
        ret = true;
    }
    else if (!elem && (istreq(field, "groupsredirect") ||
                       istreq(field, "redirectgroups"))) {
        log_err(info->keymap->ctx,
                "Global \"groupsredirect\" not supported; Ignored\n");
        ret = true;
    }
    else if (!elem && istreq(field, "allownone")) {
        log_err(info->keymap->ctx,
                "Radio groups not supported; "
                "Ignoring \"allownone\" specification\n");
        ret = true;
    }
    else {
        ret = SetActionField(info->keymap, elem, field, arrayNdx, stmt->value,
                             info->actions);
    }

    return ret;
}

static bool
HandleSymbolsBody(SymbolsInfo *info, VarDef *def, KeyInfo *keyi)
{
    bool ok = true;
    const char *elem, *field;
    ExprDef *arrayNdx;

    for (; def; def = (VarDef *) def->common.next) {
        if (def->name && def->name->op == EXPR_FIELD_REF) {
            ok = HandleSymbolsVar(info, def);
            continue;
        }

        if (!def->name) {
            if (!def->value || def->value->op == EXPR_KEYSYM_LIST)
                field = "symbols";
            else
                field = "actions";
            arrayNdx = NULL;
        }
        else {
            ok = ExprResolveLhs(info->keymap->ctx, def->name, &elem, &field,
                                &arrayNdx);
        }

        if (ok)
            ok = SetSymbolsField(info, keyi, field, arrayNdx, def->value);
    }

    return ok;
}

static bool
SetExplicitGroup(SymbolsInfo *info, KeyInfo *keyi)
{
    xkb_group_index_t i;

    if (info->explicit_group == 0)
        return true;

    for (i = 1; i < XKB_NUM_GROUPS; i++) {
        if (keyi->groups[i].defined) {
            log_warn(info->keymap->ctx,
                     "For the map %s an explicit group specified, "
                     "but key %s has more than one group defined; "
                     "All groups except first one will be ignored\n",
                     info->name, LongKeyNameText(keyi->name));
            break;
        }
    }
    if (i < XKB_NUM_GROUPS)
        for (i = 1; i < XKB_NUM_GROUPS; i++)
            ClearGroupInfo(&keyi->groups[i]);

    keyi->groups[info->explicit_group] = keyi->groups[0];
    InitGroupInfo(&keyi->groups[0]);
    return true;
}

static int
HandleSymbolsDef(SymbolsInfo *info, SymbolsDef *stmt)
{
    KeyInfo keyi;

    InitKeyInfo(&keyi, info->file_id);
    CopyKeyInfo(&info->dflt, &keyi, false);
    keyi.merge = stmt->merge;
    keyi.name = KeyNameToLong(stmt->keyName);
    if (!HandleSymbolsBody(info, (VarDef *) stmt->symbols, &keyi)) {
        info->errorCount++;
        return false;
    }

    if (!SetExplicitGroup(info, &keyi)) {
        info->errorCount++;
        return false;
    }

    if (!AddKeySymbols(info, &keyi)) {
        info->errorCount++;
        return false;
    }
    return true;
}

static bool
HandleModMapDef(SymbolsInfo *info, ModMapDef *def)
{
    ExprDef *key;
    ModMapEntry tmp;
    xkb_mod_index_t ndx;
    bool ok;
    struct xkb_context *ctx = info->keymap->ctx;

    if (!LookupModIndex(ctx, NULL, def->modifier, EXPR_TYPE_INT, &ndx)) {
        log_err(info->keymap->ctx,
                "Illegal modifier map definition; "
                "Ignoring map for non-modifier \"%s\"\n",
                xkb_atom_text(ctx, def->modifier));
        return false;
    }

    ok = true;
    tmp.modifier = ndx;

    for (key = def->keys; key != NULL; key = (ExprDef *) key->common.next) {
        xkb_keysym_t sym;

        if (key->op == EXPR_VALUE && key->value_type == EXPR_TYPE_KEYNAME) {
            tmp.haveSymbol = false;
            tmp.u.keyName = KeyNameToLong(key->value.keyName);
        }
        else if (ExprResolveKeySym(ctx, key, &sym)) {
            tmp.haveSymbol = true;
            tmp.u.keySym = sym;
        }
        else {
            log_err(info->keymap->ctx,
                    "Modmap entries may contain only key names or keysyms; "
                    "Illegal definition for %s modifier ignored\n",
                    ModIndexText(tmp.modifier));
            continue;
        }

        ok = AddModMapEntry(info, &tmp) && ok;
    }
    return ok;
}

static void
HandleSymbolsFile(SymbolsInfo *info, XkbFile *file, enum merge_mode merge)
{
    bool ok;
    ParseCommon *stmt;

    free(info->name);
    info->name = strdup_safe(file->name);

    stmt = file->defs;
    for (stmt = file->defs; stmt; stmt = stmt->next) {
        switch (stmt->type) {
        case STMT_INCLUDE:
            ok = HandleIncludeSymbols(info, (IncludeStmt *) stmt);
            break;
        case STMT_SYMBOLS:
            ok = HandleSymbolsDef(info, (SymbolsDef *) stmt);
            break;
        case STMT_VAR:
            ok = HandleSymbolsVar(info, (VarDef *) stmt);
            break;
        case STMT_VMOD:
            ok = HandleVModDef((VModDef *) stmt, info->keymap, merge,
                               &info->vmods);
            break;
        case STMT_MODMAP:
            ok = HandleModMapDef(info, (ModMapDef *) stmt);
            break;
        default:
            log_err(info->keymap->ctx,
                    "Interpretation files may not include other types; "
                    "Ignoring %s\n", stmt_type_to_string(stmt->type));
            ok = false;
            break;
        }

        if (!ok)
            info->errorCount++;

        if (info->errorCount > 10) {
            log_err(info->keymap->ctx, "Abandoning symbols file \"%s\"\n",
                    file->topName);
            break;
        }
    }
}

/**
 * Given a keysym @sym, return a key which generates it, or NULL.
 * This is used for example in a modifier map definition, such as:
 *      modifier_map Lock           { Caps_Lock };
 * where we want to add the Lock modifier to the modmap of the key
 * which matches the keysym Caps_Lock.
 * Since there can be many keys which generates the keysym, the key
 * is chosen first by lowest group in which the keysym appears, than
 * by lowest level and than by lowest key code.
 */
static struct xkb_key *
FindKeyForSymbol(struct xkb_keymap *keymap, xkb_keysym_t sym)
{
    struct xkb_key *key, *ret = NULL;
    xkb_group_index_t group, min_group = UINT32_MAX;
    xkb_level_index_t level, min_level = UINT16_MAX;

    xkb_foreach_key(key, keymap) {
        for (group = 0; group < key->num_groups; group++) {
            for (level = 0; level < XkbKeyGroupWidth(keymap, key, group);
                 level++) {
                if (XkbKeyNumSyms(key, group, level) != 1 ||
                    (XkbKeySymEntry(key, group, level))[0] != sym)
                    continue;

                /*
                 * If the keysym was found in a group or level > 0, we must
                 * keep looking since we might find a key in which the keysym
                 * is in a lower group or level.
                 */
                if (group < min_group ||
                    (group == min_group && level < min_level)) {
                    ret = key;
                    if (group == 0 && level == 0) {
                        return ret;
                    }
                    else {
                        min_group = group;
                        min_level = level;
                    }
                }
            }
        }
    }

    return ret;
}

/**
 * Find the given name in the keymap->map->types and return its index.
 *
 * @param name The name to search for.
 * @param type_rtrn Set to the index of the name if found.
 *
 * @return true if found, false otherwise.
 */
static bool
FindNamedType(struct xkb_keymap *keymap, xkb_atom_t name, unsigned *type_rtrn)
{
    unsigned int i;

    for (i = 0; i < keymap->num_types; i++) {
        if (keymap->types[i].name == name) {
            *type_rtrn = i;
            return true;
        }
    }

    return false;
}

/**
 * Assign a type to the given sym and return the Atom for the type assigned.
 *
 * Simple recipe:
 * - ONE_LEVEL for width 0/1
 * - ALPHABETIC for 2 shift levels, with lower/upercase
 * - KEYPAD for keypad keys.
 * - TWO_LEVEL for other 2 shift level keys.
 * and the same for four level keys.
 *
 * @param width Number of sysms in syms.
 * @param syms The keysyms for the given key (must be size width).
 * @param typeNameRtrn Set to the Atom of the type name.
 *
 * @returns true if a type could be found, false otherwise.
 *
 * FIXME: I need to take the KeyInfo so I can look at symsMapIndex and
 *        all that fun stuff rather than just assuming there's always one
 *        symbol per level.
 */
static bool
FindAutomaticType(struct xkb_keymap *keymap, xkb_level_index_t width,
                  const xkb_keysym_t *syms, xkb_atom_t *typeNameRtrn,
                  bool *autoType)
{
    *autoType = false;
    if ((width == 1) || (width == 0)) {
        *typeNameRtrn = xkb_atom_intern(keymap->ctx, "ONE_LEVEL");
        *autoType = true;
    }
    else if (width == 2) {
        if (syms && xkb_keysym_is_lower(syms[0]) &&
            xkb_keysym_is_upper(syms[1])) {
            *typeNameRtrn = xkb_atom_intern(keymap->ctx, "ALPHABETIC");
        }
        else if (syms && (xkb_keysym_is_keypad(syms[0]) ||
                          xkb_keysym_is_keypad(syms[1]))) {
            *typeNameRtrn = xkb_atom_intern(keymap->ctx, "KEYPAD");
            *autoType = true;
        }
        else {
            *typeNameRtrn = xkb_atom_intern(keymap->ctx, "TWO_LEVEL");
            *autoType = true;
        }
    }
    else if (width <= 4) {
        if (syms && xkb_keysym_is_lower(syms[0]) &&
            xkb_keysym_is_upper(syms[1]))
            if (xkb_keysym_is_lower(syms[2]) && xkb_keysym_is_upper(syms[3]))
                *typeNameRtrn =
                    xkb_atom_intern(keymap->ctx, "FOUR_LEVEL_ALPHABETIC");
            else
                *typeNameRtrn = xkb_atom_intern(keymap->ctx,
                                                "FOUR_LEVEL_SEMIALPHABETIC");

        else if (syms && (xkb_keysym_is_keypad(syms[0]) ||
                          xkb_keysym_is_keypad(syms[1])))
            *typeNameRtrn = xkb_atom_intern(keymap->ctx, "FOUR_LEVEL_KEYPAD");
        else
            *typeNameRtrn = xkb_atom_intern(keymap->ctx, "FOUR_LEVEL");
        /* XXX: why not set autoType here? */
    }
    return width <= 4;
}

/**
 * Ensure the given KeyInfo is in a coherent state, i.e. no gaps between the
 * groups, and reduce to one group if all groups are identical anyway.
 */
static void
PrepareKeyDef(KeyInfo *keyi)
{
    xkb_group_index_t i, lastGroup;
    const GroupInfo *group0;
    bool identical;

    /* get highest group number */
    for (i = XKB_NUM_GROUPS - 1; i > 0; i--)
        if (keyi->groups[i].defined)
            break;
    lastGroup = i;

    if (lastGroup == 0)
        return;

    group0 = &keyi->groups[0];

    /* If there are empty groups between non-empty ones fill them with data */
    /* from the first group. */
    /* We can make a wrong assumption here. But leaving gaps is worse. */
    for (i = lastGroup; i > 0; i--) {
        GroupInfo *groupi = &keyi->groups[i];

        if (groupi->defined)
            continue;

        if (group0->defined & GROUP_FIELD_TYPE) {
            groupi->type = group0->type;
            groupi->defined |= GROUP_FIELD_TYPE;
        }
        if ((group0->defined & GROUP_FIELD_ACTS) &&
            !darray_empty(group0->acts)) {
            darray_copy(groupi->acts, group0->acts);
            groupi->defined |= GROUP_FIELD_ACTS;
        }
        if ((group0->defined & GROUP_FIELD_SYMS) &&
            !darray_empty(group0->syms)) {
            darray_copy(groupi->syms, group0->syms);
            darray_copy(groupi->symsMapIndex, group0->symsMapIndex);
            darray_copy(groupi->symsMapNumEntries, group0->symsMapNumEntries);
            groupi->defined |= GROUP_FIELD_SYMS;
        }
        if (group0->defined)
            groupi->numLevels = group0->numLevels;
    }

    /* If all groups are completely identical remove them all */
    /* exept the first one. */
    identical = true;
    for (i = lastGroup; i > 0; i--) {
        GroupInfo *groupi = &keyi->groups[i];

        if (groupi->numLevels != group0->numLevels ||
            groupi->type != group0->type) {
            identical = false;
            break;
        }
        if (!darray_same(groupi->syms, group0->syms) &&
            (darray_empty(groupi->syms) || darray_empty(group0->syms) ||
             darray_size(groupi->syms) != darray_size(group0->syms) ||
             memcmp(darray_mem(groupi->syms, 0),
                    darray_mem(group0->syms, 0),
                    sizeof(xkb_keysym_t) * darray_size(group0->syms)))) {
            identical = false;
            break;
        }
        if (!darray_same(groupi->symsMapIndex, group0->symsMapIndex) &&
            (darray_empty(groupi->symsMapIndex) ||
             darray_empty(group0->symsMapIndex) ||
             memcmp(darray_mem(groupi->symsMapIndex, 0),
                    darray_mem(group0->symsMapIndex, 0),
                    group0->numLevels * sizeof(int)))) {
            identical = false;
            continue;
        }
        if (!darray_same(groupi->symsMapNumEntries,
                         group0->symsMapNumEntries) &&
            (darray_empty(groupi->symsMapNumEntries) ||
             darray_empty(group0->symsMapNumEntries) ||
             memcmp(darray_mem(groupi->symsMapNumEntries, 0),
                    darray_mem(group0->symsMapNumEntries, 0),
                    group0->numLevels * sizeof(size_t)))) {
            identical = false;
            continue;
        }
        if (!darray_same(groupi->acts, group0->acts) &&
            (darray_empty(groupi->acts) || darray_empty(group0->acts) ||
             memcmp(darray_mem(groupi->acts, 0),
                    darray_mem(group0->acts, 0),
                    group0->numLevels * sizeof(union xkb_action)))) {
            identical = false;
            break;
        }
    }

    if (identical)
        for (i = lastGroup; i > 0; i--)
            ClearGroupInfo(&keyi->groups[i]);
}

/**
 * Copy the KeyInfo into the keyboard description.
 *
 * This function recurses.
 */
static bool
CopySymbolsDef(SymbolsInfo *info, KeyInfo *keyi,
               xkb_keycode_t start_from)
{
    struct xkb_keymap *keymap = info->keymap;
    xkb_keycode_t kc;
    struct xkb_key *key;
    size_t sizeSyms = 0;
    xkb_group_index_t i, nGroups;
    xkb_level_index_t width, tmp;
    struct xkb_key_type * type;
    bool haveActions, autoType, useAlias;
    unsigned types[XKB_NUM_GROUPS];
    unsigned int symIndex = 0;

    useAlias = (start_from == 0);

    key = FindNamedKey(keymap, keyi->name, useAlias, start_from);
    if (!key) {
        if (start_from == 0)
            log_vrb(info->keymap->ctx, 5,
                    "Key %s not found in keycodes; Symbols ignored\n",
                    LongKeyNameText(keyi->name));
        return false;
    }
    kc = XkbKeyGetKeycode(keymap, key);

    haveActions = false;
    width = 0;
    for (i = nGroups = 0; i < XKB_NUM_GROUPS; i++) {
        GroupInfo *groupi = &keyi->groups[i];

        if (i + 1 > nGroups && groupi->defined)
            nGroups = i + 1;

        if (!darray_empty(groupi->acts))
            haveActions = true;

        autoType = false;

        /* Assign the type to the key, if it is missing. */
        if (groupi->type == XKB_ATOM_NONE) {
            if (keyi->dfltType != XKB_ATOM_NONE)
                groupi->type = keyi->dfltType;
            else if (FindAutomaticType(keymap, groupi->numLevels,
                                       darray_mem(groupi->syms, 0),
                                       &groupi->type, &autoType)) { }
            else
                log_vrb(info->keymap->ctx, 5,
                        "No automatic type for %d symbols; "
                        "Using %s for the %s key (keycode %d)\n",
                        groupi->numLevels,
                        xkb_atom_text(keymap->ctx, groupi->type),
                        LongKeyNameText(keyi->name), kc);
        }

        if (FindNamedType(keymap, groupi->type, &types[i])) {
            if (!autoType || groupi->numLevels > 2)
                key->explicit_groups |= (1 << i);
        }
        else {
            log_vrb(info->keymap->ctx, 3,
                    "Type \"%s\" is not defined; "
                    "Using default type for the %s key (keycode %d)\n",
                    xkb_atom_text(keymap->ctx, groupi->type),
                    LongKeyNameText(keyi->name), kc);
            /*
             * Index 0 is guaranteed to contain something, usually
             * ONE_LEVEL or at least some default one-level type.
             */
            types[i] = 0;
        }

        /* if the type specifies fewer levels than the key has, shrink the key */
        type = &keymap->types[types[i]];
        if (type->num_levels < groupi->numLevels) {
            log_vrb(info->keymap->ctx, 1,
                    "Type \"%s\" has %d levels, but %s has %d symbols; "
                    "Ignoring extra symbols\n",
                    xkb_atom_text(keymap->ctx, type->name),
                    type->num_levels,
                    LongKeyNameText(keyi->name),
                    groupi->numLevels);
            groupi->numLevels = type->num_levels;
        }

        width = MAX3(width, groupi->numLevels, type->num_levels);
        sizeSyms += darray_size(groupi->syms);
    }

    darray_resize0(key->syms, sizeSyms);

    key->num_groups = nGroups;

    key->width = width;

    key->sym_index = calloc(nGroups * width, sizeof(*key->sym_index));

    key->num_syms = calloc(nGroups * width, sizeof(*key->num_syms));

    if (haveActions) {
        key->actions = calloc(nGroups * width, sizeof(*key->actions));
        key->explicit |= EXPLICIT_INTERP;
    }

    key->out_of_range_group_number = keyi->out_of_range_group_number;
    key->out_of_range_group_action = keyi->out_of_range_group_action;

    for (i = 0; i < nGroups; i++) {
        GroupInfo *groupi = &keyi->groups[i];

        /* assign kt_index[i] to the index of the type in map->types.
         * kt_index[i] may have been set by a previous run (if we have two
         * layouts specified). Let's not overwrite it with the ONE_LEVEL
         * default group if we dont even have keys for this group anyway.
         *
         * FIXME: There should be a better fix for this.
         */
        if (groupi->numLevels)
            key->kt_index[i] = types[i];

        if (!darray_empty(groupi->syms)) {
            /* fill key to "width" symbols*/
            for (tmp = 0; tmp < width; tmp++) {
                if (tmp < groupi->numLevels &&
                    darray_item(groupi->symsMapNumEntries, tmp) != 0) {
                    memcpy(darray_mem(key->syms, symIndex),
                           darray_mem(groupi->syms,
                                      darray_item(groupi->symsMapIndex, tmp)),
                           darray_item(groupi->symsMapNumEntries, tmp) * sizeof(xkb_keysym_t));
                    key->sym_index[(i * width) + tmp] = symIndex;
                    key->num_syms[(i * width) + tmp] =
                        darray_item(groupi->symsMapNumEntries, tmp);
                    symIndex += key->num_syms[(i * width) + tmp];
                }
                else {
                    key->sym_index[(i * width) + tmp] = -1;
                    key->num_syms[(i * width) + tmp] = 0;
                }

                if (key->actions && !darray_empty(groupi->acts)) {
                    if (tmp < groupi->numLevels)
                        key->actions[tmp] = darray_item(groupi->acts, tmp);
                    else
                        key->actions[tmp].type = ACTION_TYPE_NONE;
                }
            }
        }
    }

    if (keyi->defined & KEY_FIELD_VMODMAP) {
        key->vmodmap = keyi->vmodmap;
        key->explicit |= EXPLICIT_VMODMAP;
    }

    if (keyi->repeat != KEY_REPEAT_UNDEFINED) {
        key->repeats = (keyi->repeat == KEY_REPEAT_YES);
        key->explicit |= EXPLICIT_REPEAT;
    }

    /* do the same thing for the next key */
    CopySymbolsDef(info, keyi, kc + 1);
    return true;
}

static bool
CopyModMapDef(SymbolsInfo *info, ModMapEntry *entry)
{
    struct xkb_key *key;
    struct xkb_keymap *keymap = info->keymap;

    if (!entry->haveSymbol) {
        key = FindNamedKey(keymap, entry->u.keyName, true, 0);
        if (!key) {
            log_vrb(info->keymap->ctx, 5,
                    "Key %s not found in keycodes; "
                    "Modifier map entry for %s not updated\n",
                    LongKeyNameText(entry->u.keyName),
                    ModIndexText(entry->modifier));
            return false;
        }
    }
    else {
        key = FindKeyForSymbol(keymap, entry->u.keySym);
        if (!key) {
            log_vrb(info->keymap->ctx, 5,
                    "Key \"%s\" not found in symbol map; "
                    "Modifier map entry for %s not updated\n",
                    KeysymText(entry->u.keySym),
                    ModIndexText(entry->modifier));
            return false;
        }
    }

    key->modmap |= (1 << entry->modifier);
    return true;
}

/**
 * Handle the xkb_symbols section of an xkb file.
 *
 * @param file The parsed xkb_symbols section of the xkb file.
 * @param keymap Handle to the keyboard description to store the symbols in.
 * @param merge Merge strategy (e.g. MERGE_OVERRIDE).
 */
bool
CompileSymbols(XkbFile *file, struct xkb_keymap *keymap,
               enum merge_mode merge)
{
    xkb_group_index_t i;
    struct xkb_key *key;
    SymbolsInfo info;
    ActionsInfo *actions;
    KeyInfo *keyi;
    ModMapEntry *mm;

    actions = NewActionsInfo();
    if (!actions)
        return false;

    InitSymbolsInfo(&info, keymap, file->id, actions);
    info.dflt.merge = merge;

    HandleSymbolsFile(&info, file, merge);

    if (darray_empty(info.keys))
        goto err_info;

    if (info.errorCount != 0)
        goto err_info;

    if (info.name)
        keymap->symbols_section_name = strdup(info.name);

    for (i = 0; i < XKB_NUM_GROUPS; i++)
        if (info.groupNames[i] != XKB_ATOM_NONE)
            keymap->group_names[i] = info.groupNames[i];

    /* sanitize keys */
    darray_foreach(keyi, info.keys)
        PrepareKeyDef(keyi);

    /* copy! */
    darray_foreach(keyi, info.keys)
        if (!CopySymbolsDef(&info, keyi, 0))
            info.errorCount++;

    if (xkb_get_log_verbosity(keymap->ctx) > 3) {
        xkb_foreach_key(key, keymap) {
            if (key->name[0] == '\0')
                continue;

            if (key->num_groups < 1)
                log_info(info.keymap->ctx,
                         "No symbols defined for %s (keycode %d)\n",
                         KeyNameText(key->name),
                         XkbKeyGetKeycode(keymap, key));
        }
    }

    darray_foreach(mm, info.modMaps)
        if (!CopyModMapDef(&info, mm))
            info.errorCount++;

    ClearSymbolsInfo(&info);
    FreeActionsInfo(actions);
    return true;

err_info:
    FreeActionsInfo(actions);
    ClearSymbolsInfo(&info);
    return false;
}
