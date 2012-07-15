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
#include "alias.h"
#include "keycodes.h"
#include "vmod.h"

/***====================================================================***/

/* Needed to work with the typechecker. */
typedef darray (xkb_keysym_t) darray_xkb_keysym_t;
typedef darray (union xkb_action) darray_xkb_action;

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

typedef struct _KeyInfo {
    CommonInfo defs;
    unsigned long name; /* the 4 chars of the key name, as long */
    unsigned char groupInfo;
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
} KeyInfo;

/**
 * Init the given key info to sane values.
 */
static void
InitKeyInfo(KeyInfo * info, unsigned file_id)
{
    int i;
    static const char dflt[4] = "*";

    info->defs.defined = 0;
    info->defs.file_id = file_id;
    info->defs.merge = MERGE_OVERRIDE;
    info->defs.next = NULL;
    info->name = KeyNameToLong(dflt);
    info->groupInfo = 0;
    info->typesDefined = info->symsDefined = info->actsDefined = 0;
    for (i = 0; i < XkbNumKbdGroups; i++) {
        info->numLevels[i] = 0;
        info->types[i] = XKB_ATOM_NONE;
        darray_init(info->syms[i]);
        darray_init(info->symsMapIndex[i]);
        darray_init(info->symsMapNumEntries[i]);
        darray_init(info->acts[i]);
    }
    info->dfltType = XKB_ATOM_NONE;
    info->behavior.type = XkbKB_Default;
    info->behavior.data = 0;
    info->vmodmap = 0;
    info->repeat = RepeatUndefined;
}

/**
 * Free memory associated with this key info and reset to sane values.
 */
static void
FreeKeyInfo(KeyInfo * info)
{
    int i;

    info->defs.defined = 0;
    info->defs.file_id = 0;
    info->defs.merge = MERGE_OVERRIDE;
    info->defs.next = NULL;
    info->groupInfo = 0;
    info->typesDefined = info->symsDefined = info->actsDefined = 0;
    for (i = 0; i < XkbNumKbdGroups; i++) {
        info->numLevels[i] = 0;
        info->types[i] = XKB_ATOM_NONE;
        darray_free(info->syms[i]);
        darray_free(info->symsMapIndex[i]);
        darray_free(info->symsMapNumEntries[i]);
        darray_free(info->acts[i]);
    }
    info->dfltType = XKB_ATOM_NONE;
    info->behavior.type = XkbKB_Default;
    info->behavior.data = 0;
    info->vmodmap = 0;
    info->repeat = RepeatUndefined;
}

/**
 * Copy old into new, optionally reset old to 0.
 * If old is reset, new simply re-uses old's memory. Otherwise, the memory is
 * newly allocated and new points to the new memory areas.
 */
static bool
CopyKeyInfo(KeyInfo * old, KeyInfo * new, bool clearOld)
{
    int i;

    *new = *old;
    new->defs.next = NULL;

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
    CommonInfo defs;
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
    unsigned explicit_group;
    darray(KeyInfo) keys;
    KeyInfo dflt;
    VModInfo vmods;
    ActionInfo *action;
    xkb_atom_t groupNames[XkbNumKbdGroups];

    ModMapEntry *modMap;
    AliasInfo *aliases;
} SymbolsInfo;

static void
InitSymbolsInfo(SymbolsInfo * info, struct xkb_keymap *keymap,
                unsigned file_id)
{
    int i;

    info->name = NULL;
    info->explicit_group = 0;
    info->errorCount = 0;
    info->file_id = file_id;
    info->merge = MERGE_OVERRIDE;
    darray_init(info->keys);
    darray_growalloc(info->keys, 110);
    info->modMap = NULL;
    for (i = 0; i < XkbNumKbdGroups; i++)
        info->groupNames[i] = XKB_ATOM_NONE;
    InitKeyInfo(&info->dflt, file_id);
    InitVModInfo(&info->vmods, keymap);
    info->action = NULL;
    info->aliases = NULL;
}

static void
FreeSymbolsInfo(SymbolsInfo * info)
{
    KeyInfo *key;

    free(info->name);
    darray_foreach(key, info->keys)
    FreeKeyInfo(key);
    darray_free(info->keys);
    if (info->modMap)
        ClearCommonInfo(&info->modMap->defs);
    if (info->aliases)
        ClearAliases(&info->aliases);
    memset(info, 0, sizeof(SymbolsInfo));
}

static bool
ResizeKeyGroup(KeyInfo * key, unsigned int group, unsigned int numLevels,
               unsigned sizeSyms, bool forceActions)
{
    int i;

    if (darray_size(key->syms[group]) < sizeSyms)
        darray_resize0(key->syms[group], sizeSyms);

    if (darray_empty(key->symsMapIndex[group]) ||
        key->numLevels[group] < numLevels) {
        darray_resize(key->symsMapIndex[group], numLevels);
        for (i = key->numLevels[group]; i < numLevels; i++)
            darray_item(key->symsMapIndex[group], i) = -1;
    }

    if (darray_empty(key->symsMapNumEntries[group]) ||
        key->numLevels[group] < numLevels)
        darray_resize0(key->symsMapNumEntries[group], numLevels);

    if ((forceActions && (key->numLevels[group] < numLevels ||
                          darray_empty(key->acts[group]))) ||
        (key->numLevels[group] < numLevels && !darray_empty(key->acts[group])))
        darray_resize0(key->acts[group], numLevels);

    if (key->numLevels[group] < numLevels)
        key->numLevels[group] = numLevels;

    return true;
}

enum key_group_selector {
    NONE = 0,
    FROM = (1 << 0),
    TO = (1 << 1),
};

static bool
MergeKeyGroups(SymbolsInfo * info,
               KeyInfo * into, KeyInfo * from, unsigned group)
{
    darray_xkb_keysym_t resultSyms;
    enum key_group_selector using = NONE;
    darray_xkb_action resultActs;
    unsigned int resultWidth;
    unsigned int resultSize = 0;
    int cur_idx = 0;
    int i;
    bool report, clobber;

    clobber = (from->defs.merge != MERGE_AUGMENT);

    report = (warningLevel > 9) ||
             ((into->defs.file_id == from->defs.file_id) && (warningLevel > 0));

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
                if (report) {
                    WARN
                        ("Multiple actions for level %d/group %d on key %s\n",
                        i + 1, group + 1, longText(into->name));
                    ACTION("Using %s, ignoring %s\n",
                           XkbcActionTypeText(use->type),
                           XkbcActionTypeText(ignore->type));
                }
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
            INFO("Multiple symbols for group %d, level %d on key %s\n",
                 group + 1, i + 1, longText(into->name));
            ACTION("Using %s, ignoring %s\n",
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
MergeKeys(SymbolsInfo *info, struct xkb_keymap *keymap,
          KeyInfo *into, KeyInfo *from)
{
    int i;
    unsigned collide = 0;
    bool report;

    if (from->defs.merge == MERGE_REPLACE) {
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
    report = ((warningLevel > 9) ||
              ((into->defs.file_id == from->defs.file_id)
               && (warningLevel > 0)));
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
                    into->defs.defined |= _Key_Syms;
                if (!darray_empty(into->acts[i]))
                    into->defs.defined |= _Key_Acts;
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
                if (from->defs.merge != MERGE_AUGMENT) {
                    use = from->types[i];
                    ignore = into->types[i];
                }
                else {
                    use = into->types[i];
                    ignore = from->types[i];
                }
                WARN
                    ("Multiple definitions for group %d type of key %s\n",
                    i, longText(into->name));
                ACTION("Using %s, ignoring %s\n",
                       xkb_atom_text(keymap->ctx, use),
                       xkb_atom_text(keymap->ctx, ignore));
            }
            if ((from->defs.merge != MERGE_AUGMENT)
                || (into->types[i] == XKB_ATOM_NONE)) {
                into->types[i] = from->types[i];
            }
        }
    }
    if (UseNewField(_Key_Behavior, &into->defs, &from->defs, &collide)) {
        into->behavior = from->behavior;
        into->defs.defined |= _Key_Behavior;
    }
    if (UseNewField(_Key_VModMap, &into->defs, &from->defs, &collide)) {
        into->vmodmap = from->vmodmap;
        into->defs.defined |= _Key_VModMap;
    }
    if (UseNewField(_Key_Repeat, &into->defs, &from->defs, &collide)) {
        into->repeat = from->repeat;
        into->defs.defined |= _Key_Repeat;
    }
    if (UseNewField(_Key_Type_Dflt, &into->defs, &from->defs, &collide)) {
        into->dfltType = from->dfltType;
        into->defs.defined |= _Key_Type_Dflt;
    }
    if (UseNewField(_Key_GroupInfo, &into->defs, &from->defs, &collide)) {
        into->groupInfo = from->groupInfo;
        into->defs.defined |= _Key_GroupInfo;
    }
    if (collide) {
        WARN("Symbol map for key %s redefined\n",
             longText(into->name));
        ACTION("Using %s definition for conflicting fields\n",
               (from->defs.merge == MERGE_AUGMENT ? "first" : "last"));
    }
    return true;
}

static bool
AddKeySymbols(SymbolsInfo *info, KeyInfo *key, struct xkb_keymap *keymap)
{
    unsigned long real_name;
    KeyInfo *iter, *new;

    darray_foreach(iter, info->keys)
    if (iter->name == key->name)
        return MergeKeys(info, keymap, iter, key);

    if (FindKeyNameForAlias(keymap, key->name, &real_name))
        darray_foreach(iter, info->keys)
        if (iter->name == real_name)
            return MergeKeys(info, keymap, iter, key);

    darray_resize0(info->keys, darray_size(info->keys) + 1);
    new = &darray_item(info->keys, darray_size(info->keys) - 1);
    return CopyKeyInfo(key, new, true);
}

static bool
AddModMapEntry(SymbolsInfo * info, ModMapEntry * new)
{
    ModMapEntry *mm;
    bool clobber;

    clobber = (new->defs.merge != MERGE_AUGMENT);
    for (mm = info->modMap; mm != NULL; mm = (ModMapEntry *) mm->defs.next) {
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
                ERROR
                    ("%s added to symbol map for multiple modifiers\n",
                    XkbcKeysymText(new->u.keySym));
                ACTION("Using %s, ignoring %s.\n",
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
                ERROR("Key %s added to map for multiple modifiers\n",
                      longText(new->u.keyName));
                ACTION("Using %s, ignoring %s.\n",
                       XkbcModIndexText(use),
                       XkbcModIndexText(ignore));
                mm->modifier = use;
            }
            return true;
        }
    }
    mm = uTypedAlloc(ModMapEntry);
    if (mm == NULL) {
        WSGO("Could not allocate modifier map entry\n");
        ACTION("Modifier map for %s will be incomplete\n",
               XkbcModIndexText(new->modifier));
        return false;
    }
    *mm = *new;
    mm->defs.next = &info->modMap->defs;
    info->modMap = mm;
    return true;
}

/***====================================================================***/

static void
MergeIncludedSymbols(SymbolsInfo *into, SymbolsInfo *from,
                     enum merge_mode merge, struct xkb_keymap *keymap)
{
    unsigned int i;
    KeyInfo *key;

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

    darray_foreach(key, from->keys) {
        if (merge != MERGE_DEFAULT)
            key->defs.merge = merge;

        if (!AddKeySymbols(into, key, keymap))
            into->errorCount++;
    }

    if (from->modMap != NULL) {
        ModMapEntry *mm, *next;
        for (mm = from->modMap; mm != NULL; mm = next) {
            if (merge != MERGE_DEFAULT)
                mm->defs.merge = merge;
            if (!AddModMapEntry(into, mm))
                into->errorCount++;
            next = (ModMapEntry *) mm->defs.next;
            free(mm);
        }
        from->modMap = NULL;
    }
    if (!MergeAliases(&into->aliases, &from->aliases, merge))
        into->errorCount++;
}

static void
HandleSymbolsFile(XkbFile *file, struct xkb_keymap *keymap,
                  enum merge_mode merge,
                  SymbolsInfo *info);

static bool
HandleIncludeSymbols(IncludeStmt *stmt, struct xkb_keymap *keymap,
                     SymbolsInfo *info)
{
    enum merge_mode newMerge;
    XkbFile *rtrn;
    SymbolsInfo included;
    bool haveSelf;

    haveSelf = false;
    if ((stmt->file == NULL) && (stmt->map == NULL)) {
        haveSelf = true;
        included = *info;
        memset(info, 0, sizeof(SymbolsInfo));
    }
    else if (ProcessIncludeFile(keymap->ctx, stmt, FILE_TYPE_SYMBOLS, &rtrn,
                                &newMerge)) {
        InitSymbolsInfo(&included, keymap, rtrn->id);
        included.merge = included.dflt.defs.merge = MERGE_OVERRIDE;
        if (stmt->modifier) {
            included.explicit_group = atoi(stmt->modifier) - 1;
        }
        else {
            included.explicit_group = info->explicit_group;
        }
        HandleSymbolsFile(rtrn, keymap, MERGE_OVERRIDE, &included);
        if (stmt->stmt != NULL) {
            free(included.name);
            included.name = stmt->stmt;
            stmt->stmt = NULL;
        }
        FreeXKBFile(rtrn);
    }
    else {
        info->errorCount += 10;
        return false;
    }
    if ((stmt->next != NULL) && (included.errorCount < 1)) {
        IncludeStmt *next;
        unsigned op;
        SymbolsInfo next_incl;

        for (next = stmt->next; next != NULL; next = next->next) {
            if ((next->file == NULL) && (next->map == NULL)) {
                haveSelf = true;
                MergeIncludedSymbols(&included, info, next->merge, keymap);
                FreeSymbolsInfo(info);
            }
            else if (ProcessIncludeFile(keymap->ctx, next, FILE_TYPE_SYMBOLS,
                                        &rtrn, &op)) {
                InitSymbolsInfo(&next_incl, keymap, rtrn->id);
                next_incl.merge = next_incl.dflt.defs.merge = MERGE_OVERRIDE;
                if (next->modifier) {
                    next_incl.explicit_group = atoi(next->modifier) - 1;
                }
                else {
                    next_incl.explicit_group = info->explicit_group;
                }
                HandleSymbolsFile(rtrn, keymap, MERGE_OVERRIDE, &next_incl);
                MergeIncludedSymbols(&included, &next_incl, op, keymap);
                FreeSymbolsInfo(&next_incl);
                FreeXKBFile(rtrn);
            }
            else {
                info->errorCount += 10;
                FreeSymbolsInfo(&included);
                return false;
            }
        }
    }
    else if (stmt->next) {
        info->errorCount += included.errorCount;
    }
    if (haveSelf)
        *info = included;
    else {
        MergeIncludedSymbols(info, &included, newMerge, keymap);
        FreeSymbolsInfo(&included);
    }
    return (info->errorCount == 0);
}

#define SYMBOLS 1
#define ACTIONS 2

static bool
GetGroupIndex(KeyInfo *key, struct xkb_keymap *keymap,
              ExprDef * arrayNdx, unsigned what, unsigned *ndx_rtrn)
{
    const char *name;
    ExprResult tmp;

    if (what == SYMBOLS)
        name = "symbols";
    else
        name = "actions";

    if (arrayNdx == NULL) {
        int i;
        unsigned defined;
        if (what == SYMBOLS)
            defined = key->symsDefined;
        else
            defined = key->actsDefined;

        for (i = 0; i < XkbNumKbdGroups; i++) {
            if ((defined & (1 << i)) == 0) {
                *ndx_rtrn = i;
                return true;
            }
        }
        ERROR("Too many groups of %s for key %s (max %d)\n", name,
              longText(key->name), XkbNumKbdGroups + 1);
        ACTION("Ignoring %s defined for extra groups\n", name);
        return false;
    }
    if (!ExprResolveGroup(keymap->ctx, arrayNdx, &tmp)) {
        ERROR("Illegal group index for %s of key %s\n", name,
              longText(key->name));
        ACTION("Definition with non-integer array index ignored\n");
        return false;
    }
    *ndx_rtrn = tmp.uval - 1;
    return true;
}

static bool
AddSymbolsToKey(KeyInfo *key, struct xkb_keymap *keymap,
                ExprDef *arrayNdx, ExprDef *value, SymbolsInfo *info)
{
    unsigned ndx, nSyms, nLevels;
    unsigned int i;
    long j;

    if (!GetGroupIndex(key, keymap, arrayNdx, SYMBOLS, &ndx))
        return false;
    if (value == NULL) {
        key->symsDefined |= (1 << ndx);
        return true;
    }
    if (value->op != ExprKeysymList) {
        ERROR("Expected a list of symbols, found %s\n", exprOpText(value->op));
        ACTION("Ignoring symbols for group %d of %s\n", ndx + 1,
               longText(key->name));
        return false;
    }
    if (!darray_empty(key->syms[ndx])) {
        ERROR("Symbols for key %s, group %d already defined\n",
              longText(key->name), ndx + 1);
        ACTION("Ignoring duplicate definition\n");
        return false;
    }
    nSyms = darray_size(value->value.list.syms);
    nLevels = darray_size(value->value.list.symsMapIndex);
    if ((key->numLevels[ndx] < nSyms || darray_empty(key->syms[ndx])) &&
        (!ResizeKeyGroup(key, ndx, nLevels, nSyms, false))) {
        WSGO("Could not resize group %d of key %s to contain %d levels\n",
             ndx + 1, longText(key->name), nSyms);
        ACTION("Symbols lost\n");
        return false;
    }
    key->symsDefined |= (1 << ndx);
    for (i = 0; i < nLevels; i++) {
        darray_item(key->symsMapIndex[ndx], i) =
            darray_item(value->value.list.symsMapIndex, i);
        darray_item(key->symsMapNumEntries[ndx], i) =
            darray_item(value->value.list.symsNumEntries, i);

        for (j = 0; j < darray_item(key->symsMapNumEntries[ndx], i); j++) {
            /* FIXME: What's abort() doing here? */
            if (darray_item(key->symsMapIndex[ndx], i) + j >= nSyms)
                abort();
            if (!LookupKeysym(darray_item(value->value.list.syms,
                                          darray_item(value->value.list.
                                                      symsMapIndex, i) + j),
                              &darray_item(key->syms[ndx],
                                           darray_item(key->symsMapIndex[ndx],
                                                       i) + j))) {
                WARN(
                    "Could not resolve keysym %s for key %s, group %d (%s), level %d\n",
                    darray_item(value->value.list.syms, i),
                    longText(key->name),
                    ndx + 1,
                    xkb_atom_text(keymap->ctx, info->groupNames[ndx]), nSyms);
                while (--j >= 0)
                    darray_item(key->syms[ndx],
                                darray_item(key->symsMapIndex[ndx],
                                            i) + j) = XKB_KEY_NoSymbol;
                darray_item(key->symsMapIndex[ndx], i) = -1;
                darray_item(key->symsMapNumEntries[ndx], i) = 0;
                break;
            }
            if (darray_item(key->symsMapNumEntries[ndx], i) == 1 &&
                darray_item(key->syms[ndx],
                            darray_item(key->symsMapIndex[ndx],
                                        i) + j) == XKB_KEY_NoSymbol) {
                darray_item(key->symsMapIndex[ndx], i) = -1;
                darray_item(key->symsMapNumEntries[ndx], i) = 0;
            }
        }
    }
    for (j = key->numLevels[ndx] - 1;
         j >= 0 && darray_item(key->symsMapNumEntries[ndx], j) == 0; j--)
        key->numLevels[ndx]--;
    return true;
}

static bool
AddActionsToKey(KeyInfo *key, struct xkb_keymap *keymap, ExprDef *arrayNdx,
                ExprDef *value, SymbolsInfo *info)
{
    unsigned int i;
    unsigned ndx, nActs;
    ExprDef *act;
    struct xkb_any_action *toAct;

    if (!GetGroupIndex(key, keymap, arrayNdx, ACTIONS, &ndx))
        return false;

    if (value == NULL) {
        key->actsDefined |= (1 << ndx);
        return true;
    }
    if (value->op != ExprActionList) {
        WSGO("Bad expression type (%d) for action list value\n", value->op);
        ACTION("Ignoring actions for group %d of %s\n", ndx,
               longText(key->name));
        return false;
    }
    if (!darray_empty(key->acts[ndx])) {
        WSGO("Actions for key %s, group %d already defined\n",
             longText(key->name), ndx);
        return false;
    }
    for (nActs = 0, act = value->value.child; act != NULL; nActs++) {
        act = (ExprDef *) act->common.next;
    }
    if (nActs < 1) {
        WSGO("Action list but not actions in AddActionsToKey\n");
        return false;
    }
    if ((key->numLevels[ndx] < nActs || darray_empty(key->acts[ndx])) &&
        !ResizeKeyGroup(key, ndx, nActs, nActs, true)) {
        WSGO("Could not resize group %d of key %s\n", ndx,
             longText(key->name));
        ACTION("Actions lost\n");
        return false;
    }
    key->actsDefined |= (1 << ndx);

    toAct = (struct xkb_any_action *) darray_mem(key->acts[ndx], 0);
    act = value->value.child;
    for (i = 0; i < nActs; i++, toAct++) {
        if (!HandleActionDef(act, keymap, toAct, info->action)) {
            ERROR("Illegal action definition for %s\n",
                  longText(key->name));
            ACTION("Action for group %d/level %d ignored\n", ndx + 1, i + 1);
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
SetSymbolsField(KeyInfo *key, struct xkb_keymap *keymap, char *field,
                ExprDef *arrayNdx, ExprDef *value, SymbolsInfo *info)
{
    bool ok = true;
    ExprResult tmp;

    if (strcasecmp(field, "type") == 0) {
        ExprResult ndx;
        if ((!ExprResolveString(keymap->ctx, value, &tmp))
            && (warningLevel > 0)) {
            WARN("The type field of a key symbol map must be a string\n");
            ACTION("Ignoring illegal type definition\n");
        }
        if (arrayNdx == NULL) {
            key->dfltType = xkb_atom_intern(keymap->ctx, tmp.str);
            key->defs.defined |= _Key_Type_Dflt;
        }
        else if (!ExprResolveGroup(keymap->ctx, arrayNdx, &ndx)) {
            ERROR("Illegal group index for type of key %s\n",
                  longText(key->name));
            ACTION("Definition with non-integer array index ignored\n");
            free(tmp.str);
            return false;
        }
        else {
            key->types[ndx.uval - 1] = xkb_atom_intern(keymap->ctx, tmp.str);
            key->typesDefined |= (1 << (ndx.uval - 1));
        }
        free(tmp.str);
    }
    else if (strcasecmp(field, "symbols") == 0)
        return AddSymbolsToKey(key, keymap, arrayNdx, value, info);
    else if (strcasecmp(field, "actions") == 0)
        return AddActionsToKey(key, keymap, arrayNdx, value, info);
    else if ((strcasecmp(field, "vmods") == 0) ||
             (strcasecmp(field, "virtualmods") == 0) ||
             (strcasecmp(field, "virtualmodifiers") == 0)) {
        ok = ExprResolveVModMask(value, &tmp, keymap);
        if (ok) {
            key->vmodmap = (tmp.uval >> 8);
            key->defs.defined |= _Key_VModMap;
        }
        else {
            ERROR("Expected a virtual modifier mask, found %s\n",
                  exprOpText(value->op));
            ACTION("Ignoring virtual modifiers definition for key %s\n",
                   longText(key->name));
        }
    }
    else if ((strcasecmp(field, "locking") == 0) ||
             (strcasecmp(field, "lock") == 0) ||
             (strcasecmp(field, "locks") == 0)) {
        ok = ExprResolveEnum(keymap->ctx, value, &tmp, lockingEntries);
        if (ok)
            key->behavior.type = tmp.uval;
        key->defs.defined |= _Key_Behavior;
    }
    else if ((strcasecmp(field, "radiogroup") == 0) ||
             (strcasecmp(field, "permanentradiogroup") == 0) ||
             (strcasecmp(field, "allownone") == 0)) {
        ERROR("Radio groups not supported\n");
        ACTION("Ignoring radio group specification for key %s\n",
               longText(key->name));
        return false;
    }
    else if (uStrCasePrefix("overlay", field) ||
             uStrCasePrefix("permanentoverlay", field)) {
        ERROR("Overlays not supported\n");
        ACTION("Ignoring overlay specification for key %s\n",
               longText(key->name));
    }
    else if ((strcasecmp(field, "repeating") == 0) ||
             (strcasecmp(field, "repeats") == 0) ||
             (strcasecmp(field, "repeat") == 0)) {
        ok = ExprResolveEnum(keymap->ctx, value, &tmp, repeatEntries);
        if (!ok) {
            ERROR("Illegal repeat setting for %s\n",
                  longText(key->name));
            ACTION("Non-boolean repeat setting ignored\n");
            return false;
        }
        key->repeat = tmp.uval;
        key->defs.defined |= _Key_Repeat;
    }
    else if ((strcasecmp(field, "groupswrap") == 0) ||
             (strcasecmp(field, "wrapgroups") == 0)) {
        ok = ExprResolveBoolean(keymap->ctx, value, &tmp);
        if (!ok) {
            ERROR("Illegal groupsWrap setting for %s\n",
                  longText(key->name));
            ACTION("Non-boolean value ignored\n");
            return false;
        }
        if (tmp.uval)
            key->groupInfo = XkbWrapIntoRange;
        else
            key->groupInfo = XkbClampIntoRange;
        key->defs.defined |= _Key_GroupInfo;
    }
    else if ((strcasecmp(field, "groupsclamp") == 0) ||
             (strcasecmp(field, "clampgroups") == 0)) {
        ok = ExprResolveBoolean(keymap->ctx, value, &tmp);
        if (!ok) {
            ERROR("Illegal groupsClamp setting for %s\n",
                  longText(key->name));
            ACTION("Non-boolean value ignored\n");
            return false;
        }
        if (tmp.uval)
            key->groupInfo = XkbClampIntoRange;
        else
            key->groupInfo = XkbWrapIntoRange;
        key->defs.defined |= _Key_GroupInfo;
    }
    else if ((strcasecmp(field, "groupsredirect") == 0) ||
             (strcasecmp(field, "redirectgroups") == 0)) {
        if (!ExprResolveGroup(keymap->ctx, value, &tmp)) {
            ERROR("Illegal group index for redirect of key %s\n",
                  longText(key->name));
            ACTION("Definition with non-integer group ignored\n");
            return false;
        }
        key->groupInfo =
            XkbSetGroupInfo(0, XkbRedirectIntoRange, tmp.uval - 1);
        key->defs.defined |= _Key_GroupInfo;
    }
    else {
        ERROR("Unknown field %s in a symbol interpretation\n", field);
        ACTION("Definition ignored\n");
        ok = false;
    }
    return ok;
}

static int
SetGroupName(SymbolsInfo *info, struct xkb_keymap *keymap, ExprDef *arrayNdx,
             ExprDef *value)
{
    ExprResult tmp, name;

    if ((arrayNdx == NULL) && (warningLevel > 0)) {
        WARN("You must specify an index when specifying a group name\n");
        ACTION("Group name definition without array subscript ignored\n");
        return false;
    }
    if (!ExprResolveGroup(keymap->ctx, arrayNdx, &tmp)) {
        ERROR("Illegal index in group name definition\n");
        ACTION("Definition with non-integer array index ignored\n");
        return false;
    }
    if (!ExprResolveString(keymap->ctx, value, &name)) {
        ERROR("Group name must be a string\n");
        ACTION("Illegal name for group %d ignored\n", tmp.uval);
        return false;
    }
    info->groupNames[tmp.uval - 1 + info->explicit_group] =
        xkb_atom_intern(keymap->ctx, name.str);
    free(name.str);

    return true;
}

static int
HandleSymbolsVar(VarDef *stmt, struct xkb_keymap *keymap, SymbolsInfo *info)
{
    ExprResult elem, field;
    ExprDef *arrayNdx;
    bool ret;

    if (ExprResolveLhs(keymap, stmt->name, &elem, &field, &arrayNdx) == 0)
        return 0;               /* internal error, already reported */
    if (elem.str && (strcasecmp(elem.str, "key") == 0)) {
        ret = SetSymbolsField(&info->dflt, keymap, field.str, arrayNdx,
                              stmt->value, info);
    }
    else if ((elem.str == NULL) && ((strcasecmp(field.str, "name") == 0) ||
                                    (strcasecmp(field.str, "groupname") ==
                                     0))) {
        ret = SetGroupName(info, keymap, arrayNdx, stmt->value);
    }
    else if ((elem.str == NULL)
             && ((strcasecmp(field.str, "groupswrap") == 0) ||
                 (strcasecmp(field.str, "wrapgroups") == 0))) {
        ERROR("Global \"groupswrap\" not supported\n");
        ACTION("Ignored\n");
        ret = true;
    }
    else if ((elem.str == NULL)
             && ((strcasecmp(field.str, "groupsclamp") == 0) ||
                 (strcasecmp(field.str, "clampgroups") == 0))) {
        ERROR("Global \"groupsclamp\" not supported\n");
        ACTION("Ignored\n");
        ret = true;
    }
    else if ((elem.str == NULL)
             && ((strcasecmp(field.str, "groupsredirect") == 0) ||
                 (strcasecmp(field.str, "redirectgroups") == 0))) {
        ERROR("Global \"groupsredirect\" not supported\n");
        ACTION("Ignored\n");
        ret = true;
    }
    else if ((elem.str == NULL) &&
             (strcasecmp(field.str, "allownone") == 0)) {
        ERROR("Radio groups not supported\n");
        ACTION("Ignoring \"allownone\" specification\n");
        ret = true;
    }
    else {
        ret = SetActionField(keymap, elem.str, field.str, arrayNdx,
                             stmt->value, &info->action);
    }

    free(elem.str);
    free(field.str);
    return ret;
}

static bool
HandleSymbolsBody(VarDef *def, struct xkb_keymap *keymap, KeyInfo *key,
                  SymbolsInfo *info)
{
    bool ok = true;
    ExprResult tmp, field;
    ExprDef *arrayNdx;

    for (; def != NULL; def = (VarDef *) def->common.next) {
        if ((def->name) && (def->name->type == ExprFieldRef)) {
            ok = HandleSymbolsVar(def, keymap, info);
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
                ok = ExprResolveLhs(keymap, def->name, &tmp, &field,
                                    &arrayNdx);
            }
            if (ok)
                ok = SetSymbolsField(key, keymap, field.str, arrayNdx,
                                     def->value, info);
            free(field.str);
        }
    }
    return ok;
}

static bool
SetExplicitGroup(SymbolsInfo *info, KeyInfo *key)
{
    unsigned group = info->explicit_group;

    if (group == 0)
        return true;

    if ((key->typesDefined | key->symsDefined | key->actsDefined) & ~1) {
        int i;
        WARN("For the map %s an explicit group specified\n", info->name);
        WARN("but key %s has more than one group defined\n",
             longText(key->name));
        ACTION("All groups except first one will be ignored\n");
        for (i = 1; i < XkbNumKbdGroups; i++) {
            key->numLevels[i] = 0;
            darray_free(key->syms[i]);
            darray_free(key->acts[i]);
            key->types[i] = 0;
        }
    }
    key->typesDefined = key->symsDefined = key->actsDefined = 1 << group;

    key->numLevels[group] = key->numLevels[0];
    key->numLevels[0] = 0;
    key->syms[group] = key->syms[0];
    darray_init(key->syms[0]);
    key->symsMapIndex[group] = key->symsMapIndex[0];
    darray_init(key->symsMapIndex[0]);
    key->symsMapNumEntries[group] = key->symsMapNumEntries[0];
    darray_init(key->symsMapNumEntries[0]);
    key->acts[group] = key->acts[0];
    darray_init(key->acts[0]);
    key->types[group] = key->types[0];
    key->types[0] = 0;
    return true;
}

static int
HandleSymbolsDef(SymbolsDef *stmt, struct xkb_keymap *keymap,
                 SymbolsInfo *info)
{
    KeyInfo key;

    InitKeyInfo(&key, info->file_id);
    CopyKeyInfo(&info->dflt, &key, false);
    key.defs.merge = stmt->merge;
    key.name = KeyNameToLong(stmt->keyName);
    if (!HandleSymbolsBody((VarDef *) stmt->symbols, keymap, &key, info)) {
        info->errorCount++;
        return false;
    }

    if (!SetExplicitGroup(info, &key)) {
        info->errorCount++;
        return false;
    }

    if (!AddKeySymbols(info, &key, keymap)) {
        info->errorCount++;
        return false;
    }
    return true;
}

static bool
HandleModMapDef(ModMapDef *def, struct xkb_keymap *keymap, SymbolsInfo *info)
{
    ExprDef *key;
    ModMapEntry tmp;
    ExprResult rtrn;
    bool ok;

    if (!LookupModIndex(keymap->ctx, NULL, def->modifier, TypeInt, &rtrn)) {
        ERROR("Illegal modifier map definition\n");
        ACTION("Ignoring map for non-modifier \"%s\"\n",
               xkb_atom_text(keymap->ctx, def->modifier));
        return false;
    }
    ok = true;
    tmp.modifier = rtrn.uval;
    for (key = def->keys; key != NULL; key = (ExprDef *) key->common.next) {
        if ((key->op == ExprValue) && (key->type == TypeKeyName)) {
            tmp.haveSymbol = false;
            tmp.u.keyName = KeyNameToLong(key->value.keyName);
        }
        else if (ExprResolveKeySym(keymap->ctx, key, &rtrn)) {
            tmp.haveSymbol = true;
            tmp.u.keySym = rtrn.uval;
        }
        else {
            ERROR("Modmap entries may contain only key names or keysyms\n");
            ACTION("Illegal definition for %s modifier ignored\n",
                   XkbcModIndexText(tmp.modifier));
            continue;
        }

        ok = AddModMapEntry(info, &tmp) && ok;
    }
    return ok;
}

static void
HandleSymbolsFile(XkbFile *file, struct xkb_keymap *keymap,
                  enum merge_mode merge, SymbolsInfo *info)
{
    ParseCommon *stmt;

    free(info->name);
    info->name = uDupString(file->name);
    stmt = file->defs;
    while (stmt)
    {
        switch (stmt->stmtType) {
        case StmtInclude:
            if (!HandleIncludeSymbols((IncludeStmt *) stmt, keymap, info))
                info->errorCount++;
            break;
        case StmtSymbolsDef:
            if (!HandleSymbolsDef((SymbolsDef *) stmt, keymap, info))
                info->errorCount++;
            break;
        case StmtVarDef:
            if (!HandleSymbolsVar((VarDef *) stmt, keymap, info))
                info->errorCount++;
            break;
        case StmtVModDef:
            if (!HandleVModDef((VModDef *) stmt, keymap, merge, &info->vmods))
                info->errorCount++;
            break;
        case StmtInterpDef:
            ERROR("Interpretation files may not include other types\n");
            ACTION("Ignoring definition of symbol interpretation\n");
            info->errorCount++;
            break;
        case StmtKeycodeDef:
            ERROR("Interpretation files may not include other types\n");
            ACTION("Ignoring definition of key name\n");
            info->errorCount++;
            break;
        case StmtModMapDef:
            if (!HandleModMapDef((ModMapDef *) stmt, keymap, info))
                info->errorCount++;
            break;
        default:
            WSGO("Unexpected statement type %d in HandleSymbolsFile\n",
                 stmt->stmtType);
            break;
        }
        stmt = stmt->next;
        if (info->errorCount > 10) {
#ifdef NOISY
            ERROR("Too many errors\n");
#endif
            ACTION("Abandoning symbols file \"%s\"\n", file->topName);
            break;
        }
    }
}

/**
 * Given a keysym @sym, find the keycode which generates it
 * (returned in @kc_rtrn). This is used for example in a modifier
 * map definition, such as:
 *      modifier_map Lock           { Caps_Lock };
 * where we want to add the Lock modifier to the modmap of the key
 * which matches the keysym Caps_Lock.
 * Since there can be many keys which generates the keysym, the key
 * is chosen first by lowest group in which the keysym appears, than
 * by lowest level and than by lowest key code.
 */
static bool
FindKeyForSymbol(struct xkb_keymap *keymap, xkb_keysym_t sym,
                 xkb_keycode_t *kc_rtrn)
{
    xkb_keycode_t kc;
    unsigned int group, level, min_group = UINT_MAX, min_level = UINT_MAX;

    for (kc = keymap->min_key_code; kc <= keymap->max_key_code; kc++) {
        for (group = 0; group < XkbKeyNumGroups(keymap, kc); group++) {
            for (level = 0; level < XkbKeyGroupWidth(keymap, kc, group);
                 level++) {
                if (XkbKeyNumSyms(keymap, kc, group, level) != 1 ||
                    (XkbKeySymEntry(keymap, kc, group, level))[0] != sym)
                    continue;

                /*
                 * If the keysym was found in a group or level > 0, we must
                 * keep looking since we might find a key in which the keysym
                 * is in a lower group or level.
                 */
                if (group < min_group ||
                    (group == min_group && level < min_level)) {
                    *kc_rtrn = kc;
                    if (group == 0 && level == 0) {
                        return true;
                    }
                    else {
                        min_group = group;
                        min_level = level;
                    }
                }
            }
        }
    }

    return min_group != UINT_MAX;
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
PrepareKeyDef(KeyInfo * key)
{
    int i, j, width, defined, lastGroup;
    bool identical;

    defined = key->symsDefined | key->actsDefined | key->typesDefined;
    /* get highest group number */
    for (i = XkbNumKbdGroups - 1; i >= 0; i--) {
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
        width = key->numLevels[0];
        if (key->typesDefined & 1) {
            for (j = 0; j < width; j++) {
                key->types[i] = key->types[0];
            }
            key->typesDefined |= 1 << i;
        }
        if ((key->actsDefined & 1) && !darray_empty(key->acts[0])) {
            darray_copy(key->acts[i], key->acts[0]);
            key->actsDefined |= 1 << i;
        }
        if ((key->symsDefined & 1) && !darray_empty(key->syms[0])) {
            darray_copy(key->syms[i], key->syms[0]);
            darray_copy(key->symsMapIndex[i], key->symsMapIndex[0]);
            darray_copy(key->symsMapNumEntries[i], key->symsMapNumEntries[0]);
            key->symsDefined |= 1 << i;
        }
        if (defined & 1) {
            key->numLevels[i] = key->numLevels[0];
        }
    }
    /* If all groups are completely identical remove them all */
    /* exept the first one. */
    identical = true;
    for (i = lastGroup; i > 0; i--) {
        if ((key->numLevels[i] != key->numLevels[0]) ||
            (key->types[i] != key->types[0])) {
            identical = false;
            break;
        }
        if (!darray_same(key->syms[i], key->syms[0]) &&
            (darray_empty(key->syms[i]) || darray_empty(key->syms[0]) ||
             darray_size(key->syms[i]) != darray_size(key->syms[0]) ||
             memcmp(darray_mem(key->syms[i], 0),
                    darray_mem(key->syms[0], 0),
                    sizeof(xkb_keysym_t) * darray_size(key->syms[0])))) {
            identical = false;
            break;
        }
        if (!darray_same(key->symsMapIndex[i], key->symsMapIndex[0]) &&
            (darray_empty(key->symsMapIndex[i]) ||
             darray_empty(key->symsMapIndex[0]) ||
             memcmp(darray_mem(key->symsMapIndex[i], 0),
                    darray_mem(key->symsMapIndex[0], 0),
                    key->numLevels[0] * sizeof(int)))) {
            identical = false;
            continue;
        }
        if (!darray_same(key->symsMapNumEntries[i],
                         key->symsMapNumEntries[0]) &&
            (darray_empty(key->symsMapNumEntries[i]) ||
             darray_empty(key->symsMapNumEntries[0]) ||
             memcmp(darray_mem(key->symsMapNumEntries[i], 0),
                    darray_mem(key->symsMapNumEntries[0], 0),
                    key->numLevels[0] * sizeof(size_t)))) {
            identical = false;
            continue;
        }
        if (!darray_same(key->acts[i], key->acts[0]) &&
            (darray_empty(key->acts[i]) || darray_empty(key->acts[0]) ||
             memcmp(darray_mem(key->acts[i], 0),
                    darray_mem(key->acts[0], 0),
                    key->numLevels[0] * sizeof(union xkb_action)))) {
            identical = false;
            break;
        }
    }
    if (identical) {
        for (i = lastGroup; i > 0; i--) {
            key->numLevels[i] = 0;
            darray_free(key->syms[i]);
            darray_free(key->symsMapIndex[i]);
            darray_free(key->symsMapNumEntries[i]);
            darray_free(key->acts[i]);
            key->types[i] = 0;
        }
        key->symsDefined &= 1;
        key->actsDefined &= 1;
        key->typesDefined &= 1;
    }
}

/**
 * Copy the KeyInfo into the keyboard description.
 *
 * This function recurses.
 */
static bool
CopySymbolsDef(struct xkb_keymap *keymap, KeyInfo *key, int start_from)
{
    unsigned int i;
    xkb_keycode_t kc;
    unsigned int sizeSyms = 0;
    unsigned width, tmp, nGroups;
    struct xkb_key_type * type;
    bool haveActions, autoType, useAlias;
    unsigned types[XkbNumKbdGroups];
    union xkb_action *outActs;
    unsigned int symIndex = 0;
    struct xkb_sym_map *sym_map;

    useAlias = (start_from == 0);

    /* get the keycode for the key. */
    if (!FindNamedKey(keymap, key->name, &kc, useAlias,
                      CreateKeyNames(keymap), start_from)) {
        if ((start_from == 0) && (warningLevel >= 5)) {
            WARN("Key %s not found in keycodes\n", longText(key->name));
            ACTION("Symbols ignored\n");
        }
        return false;
    }

    haveActions = false;
    for (i = width = nGroups = 0; i < XkbNumKbdGroups; i++) {
        if (((i + 1) > nGroups)
            && (((key->symsDefined | key->actsDefined) & (1 << i))
                || (key->typesDefined) & (1 << i)))
            nGroups = i + 1;
        if (!darray_empty(key->acts[i]))
            haveActions = true;
        autoType = false;
        /* Assign the type to the key, if it is missing. */
        if (key->types[i] == XKB_ATOM_NONE) {
            if (key->dfltType != XKB_ATOM_NONE)
                key->types[i] = key->dfltType;
            else if (FindAutomaticType(keymap, key->numLevels[i],
                                       darray_mem(key->syms[i], 0),
                                       &key->types[i], &autoType)) { }
            else {
                if (warningLevel >= 5) {
                    WARN("No automatic type for %d symbols\n",
                         (unsigned int) key->numLevels[i]);
                    ACTION("Using %s for the %s key (keycode %d)\n",
                           xkb_atom_text(keymap->ctx, key->types[i]),
                           longText(key->name), kc);
                }
            }
        }
        if (FindNamedType(keymap, key->types[i], &types[i])) {
            if (!autoType || key->numLevels[i] > 2)
                keymap->explicit[kc] |= (1 << i);
        }
        else {
            if (warningLevel >= 3) {
                WARN("Type \"%s\" is not defined\n",
                     xkb_atom_text(keymap->ctx, key->types[i]));
                ACTION("Using TWO_LEVEL for the %s key (keycode %d)\n",
                       longText(key->name), kc);
            }
            types[i] = XkbTwoLevelIndex;
        }
        /* if the type specifies fewer levels than the key has, shrink the key */
        type = &darray_item(keymap->types, types[i]);
        if (type->num_levels < key->numLevels[i]) {
            if (warningLevel > 0) {
                WARN("Type \"%s\" has %d levels, but %s has %d symbols\n",
                     type->name, type->num_levels,
                     xkb_atom_text(keymap->ctx, key->name), key->numLevels[i]);
                ACTION("Ignoring extra symbols\n");
            }
            key->numLevels[i] = type->num_levels;
        }
        if (key->numLevels[i] > width)
            width = key->numLevels[i];
        if (type->num_levels > width)
            width = type->num_levels;
        sizeSyms += darray_size(key->syms[i]);
    }

    if (!XkbcResizeKeySyms(keymap, kc, sizeSyms)) {
        WSGO("Could not enlarge symbols for %s (keycode %d)\n",
             longText(key->name), kc);
        return false;
    }
    if (haveActions) {
        outActs = XkbcResizeKeyActions(keymap, kc, width * nGroups);
        if (outActs == NULL) {
            WSGO("Could not enlarge actions for %s (key %d)\n",
                 longText(key->name), kc);
            return false;
        }
        keymap->explicit[kc] |= XkbExplicitInterpretMask;
    }
    else
        outActs = NULL;

    sym_map = &darray_item(keymap->key_sym_map, kc);

    if (key->defs.defined & _Key_GroupInfo)
        i = key->groupInfo;
    else
        i = sym_map->group_info;

    sym_map->group_info = XkbSetNumGroups(i, nGroups);
    sym_map->width = width;
    sym_map->sym_index = uTypedCalloc(nGroups * width, int);
    sym_map->num_syms = uTypedCalloc(nGroups * width, unsigned int);

    for (i = 0; i < nGroups; i++) {
        /* assign kt_index[i] to the index of the type in map->types.
         * kt_index[i] may have been set by a previous run (if we have two
         * layouts specified). Let's not overwrite it with the ONE_LEVEL
         * default group if we dont even have keys for this group anyway.
         *
         * FIXME: There should be a better fix for this.
         */
        if (key->numLevels[i])
            sym_map->kt_index[i] = types[i];
        if (!darray_empty(key->syms[i])) {
            /* fill key to "width" symbols*/
            for (tmp = 0; tmp < width; tmp++) {
                if (tmp < key->numLevels[i] &&
                    darray_item(key->symsMapNumEntries[i], tmp) != 0) {
                    memcpy(darray_mem(sym_map->syms, symIndex),
                           darray_mem(key->syms[i],
                                      darray_item(key->symsMapIndex[i], tmp)),
                           darray_item(key->symsMapNumEntries[i],
                                       tmp) * sizeof(xkb_keysym_t));
                    sym_map->sym_index[(i * width) + tmp] = symIndex;
                    sym_map->num_syms[(i * width) + tmp] =
                        darray_item(key->symsMapNumEntries[i], tmp);
                    symIndex += sym_map->num_syms[(i * width) + tmp];
                }
                else {
                    sym_map->sym_index[(i * width) + tmp] = -1;
                    sym_map->num_syms[(i * width) + tmp] = 0;
                }
                if (outActs != NULL && !darray_empty(key->acts[i])) {
                    if (tmp < key->numLevels[i])
                        outActs[tmp] = darray_item(key->acts[i], tmp);
                    else
                        outActs[tmp].type = XkbSA_NoAction;
                }
            }
        }
    }
    switch (key->behavior.type & XkbKB_OpMask) {
    case XkbKB_Default:
        break;

    default:
        keymap->behaviors[kc] = key->behavior;
        keymap->explicit[kc] |= XkbExplicitBehaviorMask;
        break;
    }
    if (key->defs.defined & _Key_VModMap) {
        keymap->vmodmap[kc] = key->vmodmap;
        keymap->explicit[kc] |= XkbExplicitVModMapMask;
    }
    if (key->repeat != RepeatUndefined) {
        if (key->repeat == RepeatYes)
            keymap->per_key_repeat[kc / 8] |= (1 << (kc % 8));
        else
            keymap->per_key_repeat[kc / 8] &= ~(1 << (kc % 8));
        keymap->explicit[kc] |= XkbExplicitAutoRepeatMask;
    }

    /* do the same thing for the next key */
    CopySymbolsDef(keymap, key, kc + 1);
    return true;
}

static bool
CopyModMapDef(struct xkb_keymap *keymap, ModMapEntry *entry)
{
    xkb_keycode_t kc;

    if (!entry->haveSymbol &&
        !FindNamedKey(keymap, entry->u.keyName, &kc, true,
                      CreateKeyNames(keymap), 0)) {
        if (warningLevel >= 5) {
            WARN("Key %s not found in keycodes\n",
                 longText(entry->u.keyName));
            ACTION("Modifier map entry for %s not updated\n",
                   XkbcModIndexText(entry->modifier));
        }
        return false;
    }
    else if (entry->haveSymbol &&
             !FindKeyForSymbol(keymap, entry->u.keySym, &kc)) {
        if (warningLevel > 5) {
            WARN("Key \"%s\" not found in symbol map\n",
                 XkbcKeysymText(entry->u.keySym));
            ACTION("Modifier map entry for %s not updated\n",
                   XkbcModIndexText(entry->modifier));
        }
        return false;
    }
    keymap->modmap[kc] |= (1 << entry->modifier);
    return true;
}

static bool
InitKeymapForSymbols(struct xkb_keymap *keymap)
{
    size_t nKeys = keymap->max_key_code + 1;

    darray_resize0(keymap->key_sym_map, nKeys);

    keymap->modmap = calloc(nKeys, sizeof(*keymap->modmap));
    if (!keymap->modmap)
        return false;

    keymap->explicit = calloc(nKeys, sizeof(*keymap->explicit));
    if (!keymap->explicit)
        return false;

    darray_resize0(keymap->acts, darray_size(keymap->acts) + 32 + 1);
    darray_resize0(keymap->key_acts, nKeys);

    keymap->behaviors = calloc(nKeys, sizeof(*keymap->behaviors));
    if (!keymap->behaviors)
        return false;

    keymap->vmodmap = calloc(nKeys, sizeof(*keymap->vmodmap));
    if (!keymap->vmodmap)
        return false;

    keymap->per_key_repeat = calloc(nKeys / 8, 1);
    if (!keymap->per_key_repeat)
        return false;

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
    int i;
    bool ok;
    xkb_keycode_t kc;
    SymbolsInfo info;
    KeyInfo *key;

    InitSymbolsInfo(&info, keymap, file->id);
    info.dflt.defs.merge = merge;

    HandleSymbolsFile(file, keymap, merge, &info);

    if (darray_empty(info.keys))
        goto err_info;

    if (info.errorCount != 0)
        goto err_info;

    ok = InitKeymapForSymbols(keymap);
    if (!ok)
        goto err_info;

    if (info.name)
        keymap->symbols_section_name = strdup(info.name);

    /* now copy info into xkb. */
    ApplyAliases(keymap, &info.aliases);

    for (i = 0; i < XkbNumKbdGroups; i++) {
        if (info.groupNames[i] != XKB_ATOM_NONE) {
            free(keymap->group_names[i]);
            keymap->group_names[i] = xkb_atom_strdup(keymap->ctx,
                                                     info.groupNames[i]);
        }
    }

    /* sanitize keys */
    darray_foreach(key, info.keys)
    PrepareKeyDef(key);

    /* copy! */
    darray_foreach(key, info.keys)
    if (!CopySymbolsDef(keymap, key, 0))
        info.errorCount++;

    if (warningLevel > 3) {
        for (kc = keymap->min_key_code; kc <= keymap->max_key_code; kc++) {
            if (darray_item(keymap->key_names, kc).name[0] == '\0')
                continue;

            if (XkbKeyNumGroups(keymap, kc) < 1) {
                char buf[5];
                memcpy(buf, darray_item(keymap->key_names, kc).name, 4);
                buf[4] = '\0';
                WARN("No symbols defined for <%s> (keycode %d)\n", buf, kc);
            }
        }
    }

    if (info.modMap) {
        ModMapEntry *mm, *next;
        for (mm = info.modMap; mm != NULL; mm = next) {
            if (!CopyModMapDef(keymap, mm))
                info.errorCount++;
            next = (ModMapEntry *) mm->defs.next;
        }
    }

    FreeSymbolsInfo(&info);
    return true;

err_info:
    FreeSymbolsInfo(&info);
    return false;
}
