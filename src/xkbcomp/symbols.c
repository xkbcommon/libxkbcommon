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

#include <limits.h>

#include "xkbcomp-priv.h"
#include "parseutils.h"
#include "action.h"
#include "vmod.h"

/***====================================================================***/

/* Needed to work with the typechecker. */
typedef darray(xkb_keysym_t) darray_xkb_keysym_t;
typedef darray(union xkb_action) darray_xkb_action;

#define RepeatYes       1
#define RepeatNo        0
#define RepeatUndefined ~((unsigned) 0)

#define _Key_Syms       (1 << 0)
#define _Key_Acts       (1 << 1)
#define _Key_Repeat     (1 << 2)
#define _Key_Behavior   (1 << 3)
#define _Key_Type_Dflt  (1 << 4)
#define _Key_Types      (1 << 5)
#define _Key_GroupInfo  (1 << 6)
#define _Key_VModMap    (1 << 7)

static inline const char *
longText(unsigned long val)
{
    char buf[4];

    LongToKeyName(val, buf);
    return XkbcKeyNameText(buf);
}

typedef struct _KeyInfo {
    unsigned short defined;
    unsigned file_id;
    enum merge_mode merge;

    unsigned long name; /* the 4 chars of the key name, as long */
    unsigned char typesDefined;
    unsigned char symsDefined;
    unsigned char actsDefined;
    unsigned int numLevels[XkbNumKbdGroups];

    /* syms[group] -> Single array for all the keysyms in the group. */
    darray_xkb_keysym_t syms[XkbNumKbdGroups];
    /*
     * symsMapIndex[group][level] -> The index from which the syms for
     * the level begin in the syms[group] array. Remember each keycode
     * can have multiple keysyms in each level (that is, each key press
     * can result in multiple keysyms).
     */
    darray(int) symsMapIndex[XkbNumKbdGroups];
    /*
     * symsMapNumEntries[group][level] -> How many syms are in
     * syms[group][symsMapIndex[group][level]].
     */
    darray(size_t) symsMapNumEntries[XkbNumKbdGroups];

    darray_xkb_action acts[XkbNumKbdGroups];

    xkb_atom_t types[XkbNumKbdGroups];
    unsigned repeat;
    struct xkb_behavior behavior;
    unsigned short vmodmap;
    xkb_atom_t dfltType;

    uint8_t out_of_range_group_action;
    xkb_group_index_t out_of_range_group_number;
} KeyInfo;

/**
 * Init the given key info to sane values.
 */
static void
InitKeyInfo(KeyInfo *keyi, unsigned file_id)
{
    xkb_group_index_t i;
    static const char dflt[4] = "*";

    keyi->defined = 0;
    keyi->file_id = file_id;
    keyi->merge = MERGE_OVERRIDE;
    keyi->name = KeyNameToLong(dflt);
    keyi->typesDefined = keyi->symsDefined = keyi->actsDefined = 0;

    for (i = 0; i < XkbNumKbdGroups; i++) {
        keyi->numLevels[i] = 0;
        keyi->types[i] = XKB_ATOM_NONE;
        darray_init(keyi->syms[i]);
        darray_init(keyi->symsMapIndex[i]);
        darray_init(keyi->symsMapNumEntries[i]);
        darray_init(keyi->acts[i]);
    }

    keyi->dfltType = XKB_ATOM_NONE;
    keyi->behavior.type = XkbKB_Default;
    keyi->behavior.data = 0;
    keyi->vmodmap = 0;
    keyi->repeat = RepeatUndefined;
    keyi->out_of_range_group_action = 0;
    keyi->out_of_range_group_number = 0;
}

static void
FreeKeyInfo(KeyInfo *keyi)
{
    xkb_group_index_t i;

    for (i = 0; i < XkbNumKbdGroups; i++) {
        darray_free(keyi->syms[i]);
        darray_free(keyi->symsMapIndex[i]);
        darray_free(keyi->symsMapNumEntries[i]);
        darray_free(keyi->acts[i]);
    }
}

/**
 * Copy old into new, optionally reset old to 0.
 * If old is reset, new simply re-uses old's memory. Otherwise, the memory is
 * newly allocated and new points to the new memory areas.
 */
static bool
CopyKeyInfo(KeyInfo * old, KeyInfo * new, bool clearOld)
{
    xkb_group_index_t i;

    *new = *old;

    if (clearOld) {
        for (i = 0; i < XkbNumKbdGroups; i++) {
            old->numLevels[i] = 0;
            darray_init(old->symsMapIndex[i]);
            darray_init(old->symsMapNumEntries[i]);
            darray_init(old->syms[i]);
            darray_init(old->acts[i]);
        }
    }
    else {
        for (i = 0; i < XkbNumKbdGroups; i++) {
            darray_copy(new->syms[i], old->syms[i]);
            darray_copy(new->symsMapIndex[i], old->symsMapIndex[i]);
            darray_copy(new->symsMapNumEntries[i], old->symsMapNumEntries[i]);
            darray_copy(new->acts[i], old->acts[i]);
        }
    }

    return true;
}

/***====================================================================***/

typedef struct _ModMapEntry {
    struct list entry;
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
    ActionInfo *action;
    xkb_atom_t groupNames[XkbNumKbdGroups];

    struct list modMaps;

    struct xkb_keymap *keymap;
} SymbolsInfo;

static void
InitSymbolsInfo(SymbolsInfo * info, struct xkb_keymap *keymap,
                unsigned file_id)
{
    xkb_group_index_t i;

    info->name = NULL;
    info->explicit_group = 0;
    info->errorCount = 0;
    info->file_id = file_id;
    info->merge = MERGE_OVERRIDE;
    darray_init(info->keys);
    darray_growalloc(info->keys, 110);
    list_init(&info->modMaps);
    for (i = 0; i < XkbNumKbdGroups; i++)
        info->groupNames[i] = XKB_ATOM_NONE;
    InitKeyInfo(&info->dflt, file_id);
    InitVModInfo(&info->vmods, keymap);
    info->action = NULL;
    info->keymap = keymap;
}

static void
FreeSymbolsInfo(SymbolsInfo * info)
{
    KeyInfo *keyi;
    ModMapEntry *mm, *next;

    free(info->name);
    darray_foreach(keyi, info->keys) {
        FreeKeyInfo(keyi);
    }
    darray_free(info->keys);
    list_foreach_safe(mm, next, &info->modMaps, entry)
        free(mm);
    memset(info, 0, sizeof(SymbolsInfo));
}

static bool
ResizeKeyGroup(KeyInfo *keyi, xkb_group_index_t group, unsigned int numLevels,
               unsigned sizeSyms, bool forceActions)
{
    int i;

    if (darray_size(keyi->syms[group]) < sizeSyms)
        darray_resize0(keyi->syms[group], sizeSyms);

    if (darray_empty(keyi->symsMapIndex[group]) ||
        keyi->numLevels[group] < numLevels) {
        darray_resize(keyi->symsMapIndex[group], numLevels);
        for (i = keyi->numLevels[group]; i < numLevels; i++)
            darray_item(keyi->symsMapIndex[group], i) = -1;
    }

    if (darray_empty(keyi->symsMapNumEntries[group]) ||
        keyi->numLevels[group] < numLevels)
        darray_resize0(keyi->symsMapNumEntries[group], numLevels);

    if ((forceActions && (keyi->numLevels[group] < numLevels ||
                          darray_empty(keyi->acts[group]))) ||
        (keyi->numLevels[group] < numLevels && !darray_empty(keyi->acts[group])))
        darray_resize0(keyi->acts[group], numLevels);

    if (keyi->numLevels[group] < numLevels)
        keyi->numLevels[group] = numLevels;

    return true;
}

enum key_group_selector {
    NONE = 0,
    FROM = (1 << 0),
    TO = (1 << 1),
};

static bool
MergeKeyGroups(SymbolsInfo * info,
               KeyInfo * into, KeyInfo * from, xkb_group_index_t group)
{
    darray_xkb_keysym_t resultSyms;
    enum key_group_selector using = NONE;
    darray_xkb_action resultActs;
    unsigned int resultWidth;
    unsigned int resultSize = 0;
    int cur_idx = 0;
    int i;
    bool report, clobber;
    int verbosity = xkb_get_log_verbosity(info->keymap->ctx);

    clobber = (from->merge != MERGE_AUGMENT);

    report = (verbosity > 9) ||
             (into->file_id == from->file_id && verbosity > 0);

    darray_init(resultSyms);

    if (into->numLevels[group] >= from->numLevels[group]) {
        resultActs = into->acts[group];
        resultWidth = into->numLevels[group];
    }
    else {
        resultActs = from->acts[group];
        resultWidth = from->numLevels[group];
        darray_resize(into->symsMapIndex[group],
                      from->numLevels[group]);
        darray_resize0(into->symsMapNumEntries[group],
                       from->numLevels[group]);

        for (i = into->numLevels[group]; i < from->numLevels[group]; i++)
            darray_item(into->symsMapIndex[group], i) = -1;
    }

    if (darray_empty(resultActs) && (!darray_empty(into->acts[group]) ||
                                     !darray_empty(from->acts[group]))) {
        darray_resize0(resultActs, resultWidth);
        for (i = 0; i < resultWidth; i++) {
            union xkb_action *fromAct = NULL, *toAct = NULL;

            if (!darray_empty(from->acts[group]))
                fromAct = &darray_item(from->acts[group], i);

            if (!darray_empty(into->acts[group]))
                toAct = &darray_item(into->acts[group], i);

            if (((fromAct == NULL) || (fromAct->type == XkbSA_NoAction))
                && (toAct != NULL)) {
                darray_item(resultActs, i) = *toAct;
            }
            else if (((toAct == NULL) || (toAct->type == XkbSA_NoAction))
                     && (fromAct != NULL)) {
                darray_item(resultActs, i) = *fromAct;
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
                             i + 1, group + 1, longText(into->name),
                             XkbcActionTypeText(use->type),
                             XkbcActionTypeText(ignore->type));
                if (use)
                    darray_item(resultActs, i) = *use;
            }
        }
    }

    for (i = 0; i < resultWidth; i++) {
        unsigned int fromSize = 0;
        unsigned toSize = 0;

        if (!darray_empty(from->symsMapNumEntries[group]) &&
            i < from->numLevels[group])
            fromSize = darray_item(from->symsMapNumEntries[group], i);

        if (!darray_empty(into->symsMapNumEntries[group]) &&
            i < into->numLevels[group])
            toSize = darray_item(into->symsMapNumEntries[group], i);

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
        resultSyms = from->syms[group];
        darray_free(into->symsMapNumEntries[group]);
        darray_free(into->symsMapIndex[group]);
        into->symsMapNumEntries[group] = from->symsMapNumEntries[group];
        into->symsMapIndex[group] = from->symsMapIndex[group];
        darray_init(from->symsMapNumEntries[group]);
        darray_init(from->symsMapIndex[group]);
        goto out;
    }
    else if (using == TO) {
        resultSyms = into->syms[group];
        goto out;
    }

    darray_resize0(resultSyms, resultSize);

    for (i = 0; i < resultWidth; i++) {
        enum key_group_selector use = NONE;
        unsigned int fromSize = 0;
        unsigned int toSize = 0;

        if (i < from->numLevels[group])
            fromSize = darray_item(from->symsMapNumEntries[group], i);

        if (i < into->numLevels[group])
            toSize = darray_item(into->symsMapNumEntries[group], i);

        if (fromSize == 0 && toSize == 0) {
            darray_item(into->symsMapIndex[group], i) = -1;
            darray_item(into->symsMapNumEntries[group], i) = 0;
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
                     group + 1, i + 1, longText(into->name),
                     (use == FROM ? "from" : "to"),
                     (use == FROM ? "to" : "from"));
        }

        if (use == FROM) {
            memcpy(darray_mem(resultSyms, cur_idx),
                   darray_mem(from->syms[group],
                              darray_item(from->symsMapIndex[group], i)),
                   darray_item(from->symsMapNumEntries[group],
                               i) * sizeof(xkb_keysym_t));
            darray_item(into->symsMapIndex[group], i) = cur_idx;
            darray_item(into->symsMapNumEntries[group], i) =
                darray_item(from->symsMapNumEntries[group], i);
        }
        else {
            memcpy(darray_mem(resultSyms, cur_idx),
                   darray_mem(into->syms[group],
                              darray_item(into->symsMapIndex[group], i)),
                   darray_item(into->symsMapNumEntries[group],
                               i) * sizeof(xkb_keysym_t));
            darray_item(into->symsMapIndex[group], i) = cur_idx;
        }
        cur_idx += darray_item(into->symsMapNumEntries[group], i);
    }

out:
    if (!darray_same(resultActs, into->acts[group]))
        darray_free(into->acts[group]);
    if (!darray_same(resultActs, from->acts[group]))
        darray_free(from->acts[group]);
    into->numLevels[group] = resultWidth;
    if (!darray_same(resultSyms, into->syms[group]))
        darray_free(into->syms[group]);
    into->syms[group] = resultSyms;
    if (!darray_same(resultSyms, from->syms[group]))
        darray_free(from->syms[group]);
    darray_init(from->syms[group]);
    darray_free(from->symsMapIndex[group]);
    darray_free(from->symsMapNumEntries[group]);
    into->acts[group] = resultActs;
    darray_init(from->acts[group]);
    if (!darray_empty(into->syms[group]))
        into->symsDefined |= (1 << group);
    from->symsDefined &= ~(1 << group);
    into->actsDefined |= (1 << group);
    from->actsDefined &= ~(1 << group);

    return true;
}

static bool
UseNewKeyField(unsigned field, KeyInfo *old, KeyInfo *new, int verbosity,
               unsigned *collide)
{
    if (!(old->defined & field))
        return true;

    if (new->defined & field) {
        if ((old->file_id == new->file_id && verbosity > 0) || verbosity > 9)
            *collide |= field;

        if (new->merge != MERGE_AUGMENT)
            return true;
    }

    return false;
}


static bool
MergeKeys(SymbolsInfo *info, KeyInfo *into, KeyInfo *from)
{
    xkb_group_index_t i;
    unsigned collide = 0;
    bool report;
    int verbosity = xkb_get_log_verbosity(info->keymap->ctx);

    if (from->merge == MERGE_REPLACE) {
        for (i = 0; i < XkbNumKbdGroups; i++) {
            if (into->numLevels[i] != 0) {
                darray_free(into->syms[i]);
                darray_free(into->acts[i]);
            }
        }
        *into = *from;
        memset(from, 0, sizeof(KeyInfo));
        return true;
    }

    report = (verbosity > 9 ||
              (into->file_id == from->file_id && verbosity > 0));

    for (i = 0; i < XkbNumKbdGroups; i++) {
        if (from->numLevels[i] > 0) {
            if (into->numLevels[i] == 0) {
                into->numLevels[i] = from->numLevels[i];
                into->syms[i] = from->syms[i];
                into->symsMapIndex[i] = from->symsMapIndex[i];
                into->symsMapNumEntries[i] = from->symsMapNumEntries[i];
                into->acts[i] = from->acts[i];
                into->symsDefined |= (1 << i);
                darray_init(from->syms[i]);
                darray_init(from->symsMapIndex[i]);
                darray_init(from->symsMapNumEntries[i]);
                darray_init(from->acts[i]);
                from->numLevels[i] = 0;
                from->symsDefined &= ~(1 << i);
                if (!darray_empty(into->syms[i]))
                    into->defined |= _Key_Syms;
                if (!darray_empty(into->acts[i]))
                    into->defined |= _Key_Acts;
            }
            else {
                if (report) {
                    if (!darray_empty(into->syms[i]))
                        collide |= _Key_Syms;
                    if (!darray_empty(into->acts[i]))
                        collide |= _Key_Acts;
                }
                MergeKeyGroups(info, into, from, (unsigned) i);
            }
        }
        if (from->types[i] != XKB_ATOM_NONE) {
            if ((into->types[i] != XKB_ATOM_NONE) && report &&
                (into->types[i] != from->types[i])) {
                xkb_atom_t use, ignore;
                collide |= _Key_Types;
                if (from->merge != MERGE_AUGMENT) {
                    use = from->types[i];
                    ignore = into->types[i];
                }
                else {
                    use = into->types[i];
                    ignore = from->types[i];
                }

                log_warn(info->keymap->ctx,
                         "Multiple definitions for group %d type of key %s; "
                         "Using %s, ignoring %s\n",
                         i, longText(into->name),
                         xkb_atom_text(info->keymap->ctx, use),
                         xkb_atom_text(info->keymap->ctx, ignore));
            }

            if (from->merge != MERGE_AUGMENT ||
                into->types[i] == XKB_ATOM_NONE) {
                into->types[i] = from->types[i];
            }
        }
    }

    if (UseNewKeyField(_Key_Behavior, into, from, verbosity, &collide)) {
        into->behavior = from->behavior;
        into->defined |= _Key_Behavior;
    }
    if (UseNewKeyField(_Key_VModMap, into, from, verbosity, &collide)) {
        into->vmodmap = from->vmodmap;
        into->defined |= _Key_VModMap;
    }
    if (UseNewKeyField(_Key_Repeat, into, from, verbosity, &collide)) {
        into->repeat = from->repeat;
        into->defined |= _Key_Repeat;
    }
    if (UseNewKeyField(_Key_Type_Dflt, into, from, verbosity, &collide)) {
        into->dfltType = from->dfltType;
        into->defined |= _Key_Type_Dflt;
    }
    if (UseNewKeyField(_Key_GroupInfo, into, from, verbosity, &collide)) {
        into->out_of_range_group_action = from->out_of_range_group_action;
        into->out_of_range_group_number = from->out_of_range_group_number;
        into->defined |= _Key_GroupInfo;
    }

    if (collide)
        log_warn(info->keymap->ctx,
                 "Symbol map for key %s redefined; "
                 "Using %s definition for conflicting fields\n",
                 longText(into->name),
                 (from->merge == MERGE_AUGMENT ? "first" : "last"));

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
    list_foreach(mm, &info->modMaps, entry) {
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
                        XkbcKeysymText(new->u.keySym),
                        XkbcModIndexText(use),
                        XkbcModIndexText(ignore));
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
                        longText(new->u.keyName),
                        XkbcModIndexText(use),
                        XkbcModIndexText(ignore));
                mm->modifier = use;
            }
            return true;
        }
    }

    mm = malloc(sizeof(*mm));
    if (!mm) {
        log_wsgo(info->keymap->ctx,
                 "Could not allocate modifier map entry; "
                 "Modifier map for %s will be incomplete\n",
                 XkbcModIndexText(new->modifier));
        return false;
    }

    *mm = *new;
    list_add(&mm->entry, &info->modMaps);
    return true;
}

/***====================================================================***/

static void
MergeIncludedSymbols(SymbolsInfo *into, SymbolsInfo *from,
                     enum merge_mode merge)
{
    unsigned int i;
    KeyInfo *keyi;
    ModMapEntry *mm, *next;

    if (from->errorCount > 0) {
        into->errorCount += from->errorCount;
        return;
    }
    if (into->name == NULL) {
        into->name = from->name;
        from->name = NULL;
    }
    for (i = 0; i < XkbNumKbdGroups; i++) {
        if (from->groupNames[i] != XKB_ATOM_NONE) {
            if ((merge != MERGE_AUGMENT) ||
                (into->groupNames[i] == XKB_ATOM_NONE))
                into->groupNames[i] = from->groupNames[i];
        }
    }

    darray_foreach(keyi, from->keys) {
        if (merge != MERGE_DEFAULT)
            keyi->merge = merge;

        if (!AddKeySymbols(into, keyi))
            into->errorCount++;
    }

    list_foreach_safe(mm, next, &from->modMaps, entry) {
        if (merge != MERGE_DEFAULT)
            mm->merge = merge;
        if (!AddModMapEntry(into, mm))
            into->errorCount++;
        free(mm);
    }
    list_init(&from->modMaps);
}

static void
HandleSymbolsFile(SymbolsInfo *info, XkbFile *file, enum merge_mode merge);

static bool
HandleIncludeSymbols(SymbolsInfo *info, IncludeStmt *stmt)
{
    enum merge_mode merge = MERGE_DEFAULT;
    XkbFile *rtrn;
    SymbolsInfo included, next_incl;

    InitSymbolsInfo(&included, info->keymap, info->file_id);
    if (stmt->stmt) {
        free(included.name);
        included.name = stmt->stmt;
        stmt->stmt = NULL;
    }

    for (; stmt; stmt = stmt->next) {
        if (!ProcessIncludeFile(info->keymap->ctx, stmt, FILE_TYPE_SYMBOLS,
                                &rtrn, &merge)) {
            info->errorCount += 10;
            FreeSymbolsInfo(&included);
            return false;
        }

        InitSymbolsInfo(&next_incl, info->keymap, rtrn->id);
        next_incl.merge = next_incl.dflt.merge = MERGE_OVERRIDE;
        if (stmt->modifier)
            next_incl.explicit_group = atoi(stmt->modifier) - 1;
        else
            next_incl.explicit_group = info->explicit_group;

        HandleSymbolsFile(&next_incl, rtrn, MERGE_OVERRIDE);

        MergeIncludedSymbols(&included, &next_incl, merge);

        FreeSymbolsInfo(&next_incl);
        FreeXKBFile(rtrn);
    }

    MergeIncludedSymbols(info, &included, merge);
    FreeSymbolsInfo(&included);

    return (info->errorCount == 0);
}

#define SYMBOLS 1
#define ACTIONS 2

static bool
GetGroupIndex(SymbolsInfo *info, KeyInfo *keyi, ExprDef *arrayNdx,
              unsigned what, xkb_group_index_t *ndx_rtrn)
{
    const char *name;
    ExprResult tmp;

    if (what == SYMBOLS)
        name = "symbols";
    else
        name = "actions";

    if (arrayNdx == NULL) {
        xkb_group_index_t i;
        unsigned defined;
        if (what == SYMBOLS)
            defined = keyi->symsDefined;
        else
            defined = keyi->actsDefined;

        for (i = 0; i < XkbNumKbdGroups; i++) {
            if ((defined & (1 << i)) == 0) {
                *ndx_rtrn = i;
                return true;
            }
        }
        log_err(info->keymap->ctx,
                "Too many groups of %s for key %s (max %u); "
                "Ignoring %s defined for extra groups\n",
                name, longText(keyi->name), XkbNumKbdGroups + 1, name);
        return false;
    }
    if (!ExprResolveGroup(info->keymap->ctx, arrayNdx, &tmp)) {
        log_err(info->keymap->ctx,
                "Illegal group index for %s of key %s\n"
                "Definition with non-integer array index ignored\n",
                name, longText(keyi->name));
        return false;
    }
    *ndx_rtrn = tmp.uval - 1;
    return true;
}

static bool
AddSymbolsToKey(SymbolsInfo *info, KeyInfo *keyi, ExprDef *arrayNdx,
                ExprDef *value)
{
    xkb_group_index_t ndx;
    size_t nSyms, nLevels;
    size_t i;
    int j;

    if (!GetGroupIndex(info, keyi, arrayNdx, SYMBOLS, &ndx))
        return false;
    if (value == NULL) {
        keyi->symsDefined |= (1 << ndx);
        return true;
    }
    if (value->op != ExprKeysymList) {
        log_err(info->keymap->ctx,
                "Expected a list of symbols, found %s; "
                "Ignoring symbols for group %u of %s\n",
                exprOpText(value->op), ndx + 1, longText(keyi->name));
        return false;
    }
    if (!darray_empty(keyi->syms[ndx])) {
        log_err(info->keymap->ctx,
                "Symbols for key %s, group %u already defined; "
                "Ignoring duplicate definition\n",
                longText(keyi->name), ndx + 1);
        return false;
    }
    nSyms = darray_size(value->value.list.syms);
    nLevels = darray_size(value->value.list.symsMapIndex);
    if ((keyi->numLevels[ndx] < nSyms || darray_empty(keyi->syms[ndx])) &&
        (!ResizeKeyGroup(keyi, ndx, nLevels, nSyms, false))) {
        log_wsgo(info->keymap->ctx,
                 "Could not resize group %u of key %s to contain %zu levels; "
                 "Symbols lost\n",
                 ndx + 1, longText(keyi->name), nSyms);
        return false;
    }
    keyi->symsDefined |= (1 << ndx);
    for (i = 0; i < nLevels; i++) {
        darray_item(keyi->symsMapIndex[ndx], i) =
            darray_item(value->value.list.symsMapIndex, i);
        darray_item(keyi->symsMapNumEntries[ndx], i) =
            darray_item(value->value.list.symsNumEntries, i);

        for (j = 0; j < darray_item(keyi->symsMapNumEntries[ndx], i); j++) {
            /* FIXME: What's abort() doing here? */
            if (darray_item(keyi->symsMapIndex[ndx], i) + j >= nSyms)
                abort();
            if (!LookupKeysym(darray_item(value->value.list.syms,
                                          darray_item(value->value.list.symsMapIndex,
                                                      i) + j),
                              &darray_item(keyi->syms[ndx],
                                           darray_item(keyi->symsMapIndex[ndx],
                                                       i) + j))) {
                log_warn(info->keymap->ctx,
                         "Could not resolve keysym %s for key %s, group %u (%s), level %zu\n",
                         darray_item(value->value.list.syms, i),
                         longText(keyi->name),
                         ndx + 1,
                         xkb_atom_text(info->keymap->ctx,
                                       info->groupNames[ndx]),
                         nSyms);
                while (--j >= 0)
                    darray_item(keyi->syms[ndx],
                                darray_item(keyi->symsMapIndex[ndx],
                                            i) + j) = XKB_KEY_NoSymbol;
                darray_item(keyi->symsMapIndex[ndx], i) = -1;
                darray_item(keyi->symsMapNumEntries[ndx], i) = 0;
                break;
            }
            if (darray_item(keyi->symsMapNumEntries[ndx], i) == 1 &&
                darray_item(keyi->syms[ndx],
                            darray_item(keyi->symsMapIndex[ndx],
                                        i) + j) == XKB_KEY_NoSymbol) {
                darray_item(keyi->symsMapIndex[ndx], i) = -1;
                darray_item(keyi->symsMapNumEntries[ndx], i) = 0;
            }
        }
    }
    for (j = keyi->numLevels[ndx] - 1;
         j >= 0 && darray_item(keyi->symsMapNumEntries[ndx], j) == 0; j--)
        keyi->numLevels[ndx]--;
    return true;
}

static bool
AddActionsToKey(SymbolsInfo *info, KeyInfo *keyi, ExprDef *arrayNdx,
                ExprDef *value)
{
    size_t i;
    xkb_group_index_t ndx;
    size_t nActs;
    ExprDef *act;
    struct xkb_any_action *toAct;

    if (!GetGroupIndex(info, keyi, arrayNdx, ACTIONS, &ndx))
        return false;

    if (value == NULL) {
        keyi->actsDefined |= (1 << ndx);
        return true;
    }

    if (value->op != ExprActionList) {
        log_wsgo(info->keymap->ctx,
                 "Bad expression type (%d) for action list value; "
                 "Ignoring actions for group %u of %s\n",
                 value->op, ndx, longText(keyi->name));
        return false;
    }

    if (!darray_empty(keyi->acts[ndx])) {
        log_wsgo(info->keymap->ctx,
                 "Actions for key %s, group %u already defined\n",
                 longText(keyi->name), ndx);
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

    if ((keyi->numLevels[ndx] < nActs || darray_empty(keyi->acts[ndx])) &&
        !ResizeKeyGroup(keyi, ndx, nActs, nActs, true)) {
        log_wsgo(info->keymap->ctx,
                 "Could not resize group %u of key %s; "
                 "Actions lost\n",
                 ndx, longText(keyi->name));
        return false;
    }
    keyi->actsDefined |= (1 << ndx);

    toAct = (struct xkb_any_action *) darray_mem(keyi->acts[ndx], 0);
    act = value->value.child;
    for (i = 0; i < nActs; i++, toAct++) {
        if (!HandleActionDef(act, info->keymap, toAct, info->action)) {
            log_err(info->keymap->ctx,
                    "Illegal action definition for %s; "
                    "Action for group %u/level %zu ignored\n",
                    longText(keyi->name), ndx + 1, i + 1);
        }
        act = (ExprDef *) act->common.next;
    }

    return true;
}

static const LookupEntry lockingEntries[] = {
    { "true", XkbKB_Lock },
    { "yes", XkbKB_Lock },
    { "on", XkbKB_Lock },
    { "false", XkbKB_Default },
    { "no", XkbKB_Default },
    { "off", XkbKB_Default },
    { "permanent", XkbKB_Lock | XkbKB_Permanent },
    { NULL, 0 }
};

static const LookupEntry repeatEntries[] = {
    { "true", RepeatYes },
    { "yes", RepeatYes },
    { "on", RepeatYes },
    { "false", RepeatNo },
    { "no", RepeatNo },
    { "off", RepeatNo },
    { "default", RepeatUndefined },
    { NULL, 0 }
};

static bool
SetSymbolsField(SymbolsInfo *info, KeyInfo *keyi, char *field,
                ExprDef *arrayNdx, ExprDef *value)
{
    bool ok = true;
    ExprResult tmp;
    struct xkb_context *ctx = info->keymap->ctx;

    if (strcasecmp(field, "type") == 0) {
        ExprResult ndx;
        if (!ExprResolveString(ctx, value, &tmp))
            log_lvl(info->keymap->ctx, 1,
                    "The type field of a key symbol map must be a string; "
                    "Ignoring illegal type definition\n");
        if (arrayNdx == NULL) {
            keyi->dfltType = xkb_atom_intern(ctx, tmp.str);
            keyi->defined |= _Key_Type_Dflt;
        }
        else if (!ExprResolveGroup(ctx, arrayNdx, &ndx)) {
            log_err(info->keymap->ctx,
                    "Illegal group index for type of key %s; "
                    "Definition with non-integer array index ignored\n",
                    longText(keyi->name));
            free(tmp.str);
            return false;
        }
        else {
            keyi->types[ndx.uval - 1] = xkb_atom_intern(ctx, tmp.str);
            keyi->typesDefined |= (1 << (ndx.uval - 1));
        }
        free(tmp.str);
    }
    else if (strcasecmp(field, "symbols") == 0)
        return AddSymbolsToKey(info, keyi, arrayNdx, value);
    else if (strcasecmp(field, "actions") == 0)
        return AddActionsToKey(info, keyi, arrayNdx, value);
    else if ((strcasecmp(field, "vmods") == 0) ||
             (strcasecmp(field, "virtualmods") == 0) ||
             (strcasecmp(field, "virtualmodifiers") == 0)) {
        ok = ExprResolveVModMask(value, &tmp, info->keymap);
        if (ok) {
            keyi->vmodmap = (tmp.uval >> 8);
            keyi->defined |= _Key_VModMap;
        }
        else {
            log_err(info->keymap->ctx,
                    "Expected a virtual modifier mask, found %s; "
                    "Ignoring virtual modifiers definition for key %s\n",
                    exprOpText(value->op), longText(keyi->name));
        }
    }
    else if ((strcasecmp(field, "locking") == 0) ||
             (strcasecmp(field, "lock") == 0) ||
             (strcasecmp(field, "locks") == 0)) {
        ok = ExprResolveEnum(ctx, value, &tmp, lockingEntries);
        if (ok)
            keyi->behavior.type = tmp.uval;
        keyi->defined |= _Key_Behavior;
    }
    else if ((strcasecmp(field, "radiogroup") == 0) ||
             (strcasecmp(field, "permanentradiogroup") == 0) ||
             (strcasecmp(field, "allownone") == 0)) {
        log_err(info->keymap->ctx,
                "Radio groups not supported; "
                "Ignoring radio group specification for key %s\n",
                longText(keyi->name));
        return false;
    }
    else if (uStrCasePrefix("overlay", field) ||
             uStrCasePrefix("permanentoverlay", field)) {
        log_err(info->keymap->ctx,
                "Overlays not supported; "
                "Ignoring overlay specification for key %s\n",
                longText(keyi->name));
    }
    else if ((strcasecmp(field, "repeating") == 0) ||
             (strcasecmp(field, "repeats") == 0) ||
             (strcasecmp(field, "repeat") == 0)) {
        ok = ExprResolveEnum(ctx, value, &tmp, repeatEntries);
        if (!ok) {
            log_err(info->keymap->ctx,
                    "Illegal repeat setting for %s; "
                    "Non-boolean repeat setting ignored\n",
                    longText(keyi->name));
            return false;
        }
        keyi->repeat = tmp.uval;
        keyi->defined |= _Key_Repeat;
    }
    else if ((strcasecmp(field, "groupswrap") == 0) ||
             (strcasecmp(field, "wrapgroups") == 0)) {
        ok = ExprResolveBoolean(ctx, value, &tmp);
        if (!ok) {
            log_err(info->keymap->ctx,
                    "Illegal groupsWrap setting for %s; "
                    "Non-boolean value ignored\n",
                    longText(keyi->name));
            return false;
        }
        if (tmp.uval)
            keyi->out_of_range_group_action = XkbWrapIntoRange;
        else
            keyi->out_of_range_group_action = XkbClampIntoRange;
        keyi->defined |= _Key_GroupInfo;
    }
    else if ((strcasecmp(field, "groupsclamp") == 0) ||
             (strcasecmp(field, "clampgroups") == 0)) {
        ok = ExprResolveBoolean(ctx, value, &tmp);
        if (!ok) {
            log_err(info->keymap->ctx,
                    "Illegal groupsClamp setting for %s; "
                    "Non-boolean value ignored\n",
                    longText(keyi->name));
            return false;
        }
        if (tmp.uval)
            keyi->out_of_range_group_action = XkbClampIntoRange;
        else
            keyi->out_of_range_group_action = XkbWrapIntoRange;
        keyi->defined |= _Key_GroupInfo;
    }
    else if ((strcasecmp(field, "groupsredirect") == 0) ||
             (strcasecmp(field, "redirectgroups") == 0)) {
        if (!ExprResolveGroup(ctx, value, &tmp)) {
            log_err(info->keymap->ctx,
                    "Illegal group index for redirect of key %s; "
                    "Definition with non-integer group ignored\n",
                    longText(keyi->name));
            return false;
        }
        keyi->out_of_range_group_action = XkbRedirectIntoRange;
        keyi->out_of_range_group_number = tmp.uval - 1;
        keyi->defined |= _Key_GroupInfo;
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
    ExprResult tmp, name;

    if (!arrayNdx) {
        log_lvl(info->keymap->ctx, 1,
                "You must specify an index when specifying a group name; "
                "Group name definition without array subscript ignored\n");
        return false;
    }

    if (!ExprResolveGroup(info->keymap->ctx, arrayNdx, &tmp)) {
        log_err(info->keymap->ctx,
                "Illegal index in group name definition; "
                "Definition with non-integer array index ignored\n");
        return false;
    }

    if (!ExprResolveString(info->keymap->ctx, value, &name)) {
        log_err(info->keymap->ctx,
                "Group name must be a string; "
                "Illegal name for group %d ignored\n", tmp.uval);
        return false;
    }

    info->groupNames[tmp.uval - 1 + info->explicit_group] =
        xkb_atom_intern(info->keymap->ctx, name.str);
    free(name.str);

    return true;
}

static int
HandleSymbolsVar(SymbolsInfo *info, VarDef *stmt)
{
    ExprResult elem, field;
    ExprDef *arrayNdx;
    bool ret;

    if (ExprResolveLhs(info->keymap, stmt->name, &elem, &field,
                       &arrayNdx) == 0)
        return 0;               /* internal error, already reported */
    if (elem.str && (strcasecmp(elem.str, "key") == 0)) {
        ret = SetSymbolsField(info, &info->dflt, field.str, arrayNdx,
                              stmt->value);
    }
    else if ((elem.str == NULL) && ((strcasecmp(field.str, "name") == 0) ||
                                    (strcasecmp(field.str, "groupname") ==
                                     0))) {
        ret = SetGroupName(info, arrayNdx, stmt->value);
    }
    else if ((elem.str == NULL)
             && ((strcasecmp(field.str, "groupswrap") == 0) ||
                 (strcasecmp(field.str, "wrapgroups") == 0))) {
        log_err(info->keymap->ctx,
                "Global \"groupswrap\" not supported; Ignored\n");
        ret = true;
    }
    else if ((elem.str == NULL)
             && ((strcasecmp(field.str, "groupsclamp") == 0) ||
                 (strcasecmp(field.str, "clampgroups") == 0))) {
        log_err(info->keymap->ctx,
                "Global \"groupsclamp\" not supported; Ignored\n");
        ret = true;
    }
    else if ((elem.str == NULL)
             && ((strcasecmp(field.str, "groupsredirect") == 0) ||
                 (strcasecmp(field.str, "redirectgroups") == 0))) {
        log_err(info->keymap->ctx,
                "Global \"groupsredirect\" not supported; Ignored\n");
        ret = true;
    }
    else if ((elem.str == NULL) &&
             (strcasecmp(field.str, "allownone") == 0)) {
        log_err(info->keymap->ctx,
                "Radio groups not supported; "
                "Ignoring \"allownone\" specification\n");
        ret = true;
    }
    else {
        ret = SetActionField(info->keymap, elem.str, field.str, arrayNdx,
                             stmt->value, &info->action);
    }

    free(elem.str);
    free(field.str);
    return ret;
}

static bool
HandleSymbolsBody(SymbolsInfo *info, VarDef *def, KeyInfo *keyi)
{
    bool ok = true;
    ExprResult tmp, field;
    ExprDef *arrayNdx;

    for (; def != NULL; def = (VarDef *) def->common.next) {
        if ((def->name) && (def->name->type == ExprFieldRef)) {
            ok = HandleSymbolsVar(info, def);
            continue;
        }
        else {
            if (def->name == NULL) {
                if ((def->value == NULL)
                    || (def->value->op == ExprKeysymList))
                    field.str = strdup("symbols");
                else
                    field.str = strdup("actions");
                arrayNdx = NULL;
            }
            else {
                ok = ExprResolveLhs(info->keymap, def->name, &tmp, &field,
                                    &arrayNdx);
            }
            if (ok)
                ok = SetSymbolsField(info, keyi, field.str, arrayNdx,
                                     def->value);
            free(field.str);
        }
    }
    return ok;
}

static bool
SetExplicitGroup(SymbolsInfo *info, KeyInfo *keyi)
{
    xkb_group_index_t group = info->explicit_group;

    if (group == 0)
        return true;

    if ((keyi->typesDefined | keyi->symsDefined | keyi->actsDefined) & ~1) {
        xkb_group_index_t i;
        log_warn(info->keymap->ctx,
                 "For the map %s an explicit group specified\n",
                 info->name);
        log_warn(info->keymap->ctx,
                 "but key %s has more than one group defined; "
                 "All groups except first one will be ignored\n",
                 longText(keyi->name));
        for (i = 1; i < XkbNumKbdGroups; i++) {
            keyi->numLevels[i] = 0;
            darray_free(keyi->syms[i]);
            darray_free(keyi->acts[i]);
            keyi->types[i] = 0;
        }
    }
    keyi->typesDefined = keyi->symsDefined = keyi->actsDefined = 1 << group;

    keyi->numLevels[group] = keyi->numLevels[0];
    keyi->numLevels[0] = 0;
    keyi->syms[group] = keyi->syms[0];
    darray_init(keyi->syms[0]);
    keyi->symsMapIndex[group] = keyi->symsMapIndex[0];
    darray_init(keyi->symsMapIndex[0]);
    keyi->symsMapNumEntries[group] = keyi->symsMapNumEntries[0];
    darray_init(keyi->symsMapNumEntries[0]);
    keyi->acts[group] = keyi->acts[0];
    darray_init(keyi->acts[0]);
    keyi->types[group] = keyi->types[0];
    keyi->types[0] = 0;
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
    ExprResult rtrn;
    bool ok;
    struct xkb_context *ctx = info->keymap->ctx;

    if (!LookupModIndex(ctx, NULL, def->modifier, TypeInt, &rtrn)) {
        log_err(info->keymap->ctx,
                "Illegal modifier map definition; "
                "Ignoring map for non-modifier \"%s\"\n",
                xkb_atom_text(ctx, def->modifier));
        return false;
    }
    ok = true;
    tmp.modifier = rtrn.uval;
    for (key = def->keys; key != NULL; key = (ExprDef *) key->common.next) {
        if ((key->op == ExprValue) && (key->type == TypeKeyName)) {
            tmp.haveSymbol = false;
            tmp.u.keyName = KeyNameToLong(key->value.keyName);
        }
        else if (ExprResolveKeySym(ctx, key, &rtrn)) {
            tmp.haveSymbol = true;
            tmp.u.keySym = rtrn.uval;
        }
        else {
            log_err(info->keymap->ctx,
                    "Modmap entries may contain only key names or keysyms; "
                    "Illegal definition for %s modifier ignored\n",
                    XkbcModIndexText(tmp.modifier));
            continue;
        }

        ok = AddModMapEntry(info, &tmp) && ok;
    }
    return ok;
}

static void
HandleSymbolsFile(SymbolsInfo *info, XkbFile *file, enum merge_mode merge)
{
    ParseCommon *stmt;

    free(info->name);
    info->name = uDupString(file->name);
    stmt = file->defs;
    while (stmt)
    {
        switch (stmt->stmtType) {
        case StmtInclude:
            if (!HandleIncludeSymbols(info, (IncludeStmt *) stmt))
                info->errorCount++;
            break;
        case StmtSymbolsDef:
            if (!HandleSymbolsDef(info, (SymbolsDef *) stmt))
                info->errorCount++;
            break;
        case StmtVarDef:
            if (!HandleSymbolsVar(info, (VarDef *) stmt))
                info->errorCount++;
            break;
        case StmtVModDef:
            if (!HandleVModDef((VModDef *) stmt, info->keymap, merge,
                               &info->vmods))
                info->errorCount++;
            break;
        case StmtInterpDef:
            log_err(info->keymap->ctx,
                    "Interpretation files may not include other types; "
                    "Ignoring definition of symbol interpretation\n");
            info->errorCount++;
            break;
        case StmtKeycodeDef:
            log_err(info->keymap->ctx,
                    "Interpretation files may not include other types; "
                    "Ignoring definition of key name\n");
            info->errorCount++;
            break;
        case StmtModMapDef:
            if (!HandleModMapDef(info, (ModMapDef *) stmt))
                info->errorCount++;
            break;
        default:
            log_wsgo(info->keymap->ctx,
                     "Unexpected statement type %d in HandleSymbolsFile\n",
                     stmt->stmtType);
            break;
        }
        stmt = stmt->next;
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
    xkb_group_index_t group, min_group = UINT_MAX;
    unsigned int level, min_level = UINT_MAX;

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
 * @param atom The atom to search for.
 * @param type_rtrn Set to the index of the name if found.
 *
 * @return true if found, false otherwise.
 */
static bool
FindNamedType(struct xkb_keymap *keymap, xkb_atom_t atom, unsigned *type_rtrn)
{
    unsigned n = 0;
    const char *name = xkb_atom_text(keymap->ctx, atom);
    struct xkb_key_type *type;

    if (keymap) {
        darray_foreach(type, keymap->types) {
            if (strcmp(type->name, name) == 0) {
                *type_rtrn = n;
                return true;
            }
            n++;
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
FindAutomaticType(struct xkb_keymap *keymap, int width,
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
    return ((width >= 0) && (width <= 4));
}

/**
 * Ensure the given KeyInfo is in a coherent state, i.e. no gaps between the
 * groups, and reduce to one group if all groups are identical anyway.
 */
static void
PrepareKeyDef(KeyInfo *keyi)
{
    xkb_group_index_t i, lastGroup;
    int j, width, defined;
    bool identical;

    defined = keyi->symsDefined | keyi->actsDefined | keyi->typesDefined;
    /* get highest group number */
    for (i = XkbNumKbdGroups - 1; i > 0; i--) {
        if (defined & (1 << i))
            break;
    }
    lastGroup = i;

    if (lastGroup == 0)
        return;

    /* If there are empty groups between non-empty ones fill them with data */
    /* from the first group. */
    /* We can make a wrong assumption here. But leaving gaps is worse. */
    for (i = lastGroup; i > 0; i--) {
        if (defined & (1 << i))
            continue;
        width = keyi->numLevels[0];
        if (keyi->typesDefined & 1) {
            for (j = 0; j < width; j++) {
                keyi->types[i] = keyi->types[0];
            }
            keyi->typesDefined |= 1 << i;
        }
        if ((keyi->actsDefined & 1) && !darray_empty(keyi->acts[0])) {
            darray_copy(keyi->acts[i], keyi->acts[0]);
            keyi->actsDefined |= 1 << i;
        }
        if ((keyi->symsDefined & 1) && !darray_empty(keyi->syms[0])) {
            darray_copy(keyi->syms[i], keyi->syms[0]);
            darray_copy(keyi->symsMapIndex[i], keyi->symsMapIndex[0]);
            darray_copy(keyi->symsMapNumEntries[i],
                        keyi->symsMapNumEntries[0]);
            keyi->symsDefined |= 1 << i;
        }
        if (defined & 1) {
            keyi->numLevels[i] = keyi->numLevels[0];
        }
    }
    /* If all groups are completely identical remove them all */
    /* exept the first one. */
    identical = true;
    for (i = lastGroup; i > 0; i--) {
        if ((keyi->numLevels[i] != keyi->numLevels[0]) ||
            (keyi->types[i] != keyi->types[0])) {
            identical = false;
            break;
        }
        if (!darray_same(keyi->syms[i], keyi->syms[0]) &&
            (darray_empty(keyi->syms[i]) || darray_empty(keyi->syms[0]) ||
             darray_size(keyi->syms[i]) != darray_size(keyi->syms[0]) ||
             memcmp(darray_mem(keyi->syms[i], 0),
                    darray_mem(keyi->syms[0], 0),
                    sizeof(xkb_keysym_t) * darray_size(keyi->syms[0])))) {
            identical = false;
            break;
        }
        if (!darray_same(keyi->symsMapIndex[i], keyi->symsMapIndex[0]) &&
            (darray_empty(keyi->symsMapIndex[i]) ||
             darray_empty(keyi->symsMapIndex[0]) ||
             memcmp(darray_mem(keyi->symsMapIndex[i], 0),
                    darray_mem(keyi->symsMapIndex[0], 0),
                    keyi->numLevels[0] * sizeof(int)))) {
            identical = false;
            continue;
        }
        if (!darray_same(keyi->symsMapNumEntries[i],
                         keyi->symsMapNumEntries[0]) &&
            (darray_empty(keyi->symsMapNumEntries[i]) ||
             darray_empty(keyi->symsMapNumEntries[0]) ||
             memcmp(darray_mem(keyi->symsMapNumEntries[i], 0),
                    darray_mem(keyi->symsMapNumEntries[0], 0),
                    keyi->numLevels[0] * sizeof(size_t)))) {
            identical = false;
            continue;
        }
        if (!darray_same(keyi->acts[i], keyi->acts[0]) &&
            (darray_empty(keyi->acts[i]) || darray_empty(keyi->acts[0]) ||
             memcmp(darray_mem(keyi->acts[i], 0),
                    darray_mem(keyi->acts[0], 0),
                    keyi->numLevels[0] * sizeof(union xkb_action)))) {
            identical = false;
            break;
        }
    }
    if (identical) {
        for (i = lastGroup; i > 0; i--) {
            keyi->numLevels[i] = 0;
            darray_free(keyi->syms[i]);
            darray_free(keyi->symsMapIndex[i]);
            darray_free(keyi->symsMapNumEntries[i]);
            darray_free(keyi->acts[i]);
            keyi->types[i] = 0;
        }
        keyi->symsDefined &= 1;
        keyi->actsDefined &= 1;
        keyi->typesDefined &= 1;
    }
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
    unsigned width, tmp;
    struct xkb_key_type * type;
    bool haveActions, autoType, useAlias;
    unsigned types[XkbNumKbdGroups];
    union xkb_action *outActs;
    unsigned int symIndex = 0;

    useAlias = (start_from == 0);

    key = FindNamedKey(keymap, keyi->name, useAlias,
                       CreateKeyNames(keymap), start_from);
    if (!key) {
        if (start_from == 0)
            log_lvl(info->keymap->ctx, 5,
                    "Key %s not found in keycodes; Symbols ignored\n",
                    longText(keyi->name));
        return false;
    }
    kc = XkbKeyGetKeycode(keymap, key);

    haveActions = false;
    width = 0;
    for (i = nGroups = 0; i < XkbNumKbdGroups; i++) {
        if (((i + 1) > nGroups)
            && (((keyi->symsDefined | keyi->actsDefined) & (1 << i))
                || (keyi->typesDefined) & (1 << i)))
            nGroups = i + 1;
        if (!darray_empty(keyi->acts[i]))
            haveActions = true;
        autoType = false;
        /* Assign the type to the key, if it is missing. */
        if (keyi->types[i] == XKB_ATOM_NONE) {
            if (keyi->dfltType != XKB_ATOM_NONE)
                keyi->types[i] = keyi->dfltType;
            else if (FindAutomaticType(keymap, keyi->numLevels[i],
                                       darray_mem(keyi->syms[i], 0),
                                       &keyi->types[i], &autoType)) { }
            else
                log_lvl(info->keymap->ctx, 5,
                        "No automatic type for %d symbols; "
                        "Using %s for the %s key (keycode %d)\n",
                        keyi->numLevels[i],
                        xkb_atom_text(keymap->ctx, keyi->types[i]),
                        longText(keyi->name), kc);
        }
        if (FindNamedType(keymap, keyi->types[i], &types[i])) {
            if (!autoType || keyi->numLevels[i] > 2)
                key->explicit |= (1 << i);
        }
        else {
            log_lvl(info->keymap->ctx, 3,
                    "Type \"%s\" is not defined; "
                    "Using TWO_LEVEL for the %s key (keycode %d)\n",
                    xkb_atom_text(keymap->ctx, keyi->types[i]),
                    longText(keyi->name), kc);
            types[i] = XkbTwoLevelIndex;
        }

        /* if the type specifies fewer levels than the key has, shrink the key */
        type = &darray_item(keymap->types, types[i]);
        if (type->num_levels < keyi->numLevels[i]) {
            log_lvl(info->keymap->ctx, 1,
                    "Type \"%s\" has %d levels, but %s has %d symbols; "
                    "Ignoring extra symbols\n",
                    type->name, type->num_levels,
                    longText(keyi->name),
                    keyi->numLevels[i]);
            keyi->numLevels[i] = type->num_levels;
        }
        if (keyi->numLevels[i] > width)
            width = keyi->numLevels[i];
        if (type->num_levels > width)
            width = type->num_levels;
        sizeSyms += darray_size(keyi->syms[i]);
    }

    darray_resize0(key->syms, sizeSyms);

    if (haveActions) {
        outActs = XkbcResizeKeyActions(keymap, key, width * nGroups);
        if (outActs == NULL) {
            log_wsgo(info->keymap->ctx,
                     "Could not enlarge actions for %s (key %d)\n",
                     longText(keyi->name), kc);
            return false;
        }
        key->explicit |= XkbExplicitInterpretMask;
    }
    else
        outActs = NULL;

    key->num_groups = nGroups;
    if (keyi->defined & _Key_GroupInfo) {
        key->out_of_range_group_number = keyi->out_of_range_group_number;
        key->out_of_range_group_action = keyi->out_of_range_group_action;
    }
    key->width = width;
    key->sym_index = calloc(nGroups * width, sizeof(*key->sym_index));
    key->num_syms = calloc(nGroups * width, sizeof(*key->num_syms));

    for (i = 0; i < nGroups; i++) {
        /* assign kt_index[i] to the index of the type in map->types.
         * kt_index[i] may have been set by a previous run (if we have two
         * layouts specified). Let's not overwrite it with the ONE_LEVEL
         * default group if we dont even have keys for this group anyway.
         *
         * FIXME: There should be a better fix for this.
         */
        if (keyi->numLevels[i])
            key->kt_index[i] = types[i];
        if (!darray_empty(keyi->syms[i])) {
            /* fill key to "width" symbols*/
            for (tmp = 0; tmp < width; tmp++) {
                if (tmp < keyi->numLevels[i] &&
                    darray_item(keyi->symsMapNumEntries[i], tmp) != 0) {
                    memcpy(darray_mem(key->syms, symIndex),
                           darray_mem(keyi->syms[i],
                                      darray_item(keyi->symsMapIndex[i], tmp)),
                           darray_item(keyi->symsMapNumEntries[i],
                                       tmp) * sizeof(xkb_keysym_t));
                    key->sym_index[(i * width) + tmp] = symIndex;
                    key->num_syms[(i * width) + tmp] =
                        darray_item(keyi->symsMapNumEntries[i], tmp);
                    symIndex += key->num_syms[(i * width) + tmp];
                }
                else {
                    key->sym_index[(i * width) + tmp] = -1;
                    key->num_syms[(i * width) + tmp] = 0;
                }
                if (outActs != NULL && !darray_empty(keyi->acts[i])) {
                    if (tmp < keyi->numLevels[i])
                        outActs[tmp] = darray_item(keyi->acts[i], tmp);
                    else
                        outActs[tmp].type = XkbSA_NoAction;
                }
            }
        }
    }
    switch (keyi->behavior.type & XkbKB_OpMask) {
    case XkbKB_Default:
        break;

    default:
        key->behavior = keyi->behavior;
        key->explicit |= XkbExplicitBehaviorMask;
        break;
    }
    if (keyi->defined & _Key_VModMap) {
        key->vmodmap = keyi->vmodmap;
        key->explicit |= XkbExplicitVModMapMask;
    }
    if (keyi->repeat != RepeatUndefined) {
        key->repeats = keyi->repeat == RepeatYes;
        key->explicit |= XkbExplicitAutoRepeatMask;
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
        key = FindNamedKey(keymap, entry->u.keyName, true,
                           CreateKeyNames(keymap), 0);
        if (!key) {
            log_lvl(info->keymap->ctx, 5,
                    "Key %s not found in keycodes; "
                    "Modifier map entry for %s not updated\n",
                    longText(entry->u.keyName),
                    XkbcModIndexText(entry->modifier));
            return false;
        }
    }
    else {
        key = FindKeyForSymbol(keymap, entry->u.keySym);
        if (!key) {
            log_lvl(info->keymap->ctx, 5,
                    "Key \"%s\" not found in symbol map; "
                    "Modifier map entry for %s not updated\n",
                    XkbcKeysymText(entry->u.keySym),
                    XkbcModIndexText(entry->modifier));
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
    KeyInfo *keyi;
    ModMapEntry *mm;

    InitSymbolsInfo(&info, keymap, file->id);
    info.dflt.merge = merge;

    HandleSymbolsFile(&info, file, merge);

    if (darray_empty(info.keys))
        goto err_info;

    if (info.errorCount != 0)
        goto err_info;

    darray_resize0(keymap->acts, darray_size(keymap->acts) + 32 + 1);

    if (info.name)
        keymap->symbols_section_name = strdup(info.name);

    for (i = 0; i < XkbNumKbdGroups; i++) {
        if (info.groupNames[i] != XKB_ATOM_NONE) {
            free(keymap->group_names[i]);
            keymap->group_names[i] = xkb_atom_strdup(keymap->ctx,
                                                     info.groupNames[i]);
        }
    }

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
                log_warn(info.keymap->ctx,
                         "No symbols defined for <%.4s> (keycode %d)\n",
                         key->name, XkbKeyGetKeycode(keymap, key));
        }
    }

    list_foreach(mm, &info.modMaps, entry)
        if (!CopyModMapDef(&info, mm))
            info.errorCount++;

    FreeSymbolsInfo(&info);
    return true;

err_info:
    FreeSymbolsInfo(&info);
    return false;
}
