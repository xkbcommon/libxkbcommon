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
#include "parseutils.h"
#include "action.h"
#include "alias.h"
#include "keycodes.h"
#include "vmod.h"

/***====================================================================***/

#define	RepeatYes	1
#define	RepeatNo	0
#define	RepeatUndefined	~((unsigned)0)

#define	_Key_Syms	(1<<0)
#define	_Key_Acts	(1<<1)
#define	_Key_Repeat	(1<<2)
#define	_Key_Behavior	(1<<3)
#define	_Key_Type_Dflt	(1<<4)
#define	_Key_Types	(1<<5)
#define	_Key_GroupInfo	(1<<6)
#define	_Key_VModMap	(1<<7)

typedef struct _KeyInfo
{
    CommonInfo defs;
    unsigned long name; /* the 4 chars of the key name, as long */
    unsigned char groupInfo;
    unsigned char typesDefined;
    unsigned char symsDefined;
    unsigned char actsDefined;
    unsigned int numLevels[XkbNumKbdGroups];

    /* syms[group] -> Single array for all the keysyms in the group. */
    xkb_keysym_t *syms[XkbNumKbdGroups];
    /* sizeSyms[group] -> The size of the syms[group] array. */
    int sizeSyms[XkbNumKbdGroups];
    /*
     * symsMapIndex[group][level] -> The index from which the syms for
     * the level begin in the syms[group] array. Remember each keycode
     * can have multiple keysyms in each level (that is, each key press
     * can result in multiple keysyms).
     */
    int *symsMapIndex[XkbNumKbdGroups];
    /*
     * symsMapNumEntries[group][level] -> How many syms are in
     * syms[group][symsMapIndex[group][level]].
     */
    unsigned int *symsMapNumEntries[XkbNumKbdGroups];

    union xkb_action *acts[XkbNumKbdGroups];
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
InitKeyInfo(KeyInfo * info)
{
    int i;
    static const char dflt[4] = "*";

    info->defs.defined = 0;
    info->defs.fileID = 0;
    info->defs.merge = MergeOverride;
    info->defs.next = NULL;
    info->name = KeyNameToLong(dflt);
    info->groupInfo = 0;
    info->typesDefined = info->symsDefined = info->actsDefined = 0;
    for (i = 0; i < XkbNumKbdGroups; i++)
    {
        info->numLevels[i] = 0;
        info->types[i] = XKB_ATOM_NONE;
        info->syms[i] = NULL;
        info->sizeSyms[i] = 0;
        info->symsMapIndex[i] = NULL;
        info->symsMapNumEntries[i] = NULL;
        info->acts[i] = NULL;
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
    info->defs.fileID = 0;
    info->defs.merge = MergeOverride;
    info->defs.next = NULL;
    info->groupInfo = 0;
    info->typesDefined = info->symsDefined = info->actsDefined = 0;
    for (i = 0; i < XkbNumKbdGroups; i++)
    {
        info->numLevels[i] = 0;
        info->types[i] = XKB_ATOM_NONE;
        free(info->syms[i]);
        info->syms[i] = NULL;
        info->sizeSyms[i] = 0;
        free(info->symsMapIndex[i]);
        info->symsMapIndex[i] = NULL;
        free(info->symsMapNumEntries[i]);
        info->symsMapNumEntries[i] = NULL;
        free(info->acts[i]);
        info->acts[i] = NULL;
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
    if (clearOld)
    {
        for (i = 0; i < XkbNumKbdGroups; i++)
        {
            old->numLevels[i] = 0;
            old->symsMapIndex[i] = NULL;
            old->symsMapNumEntries[i] = NULL;
            old->syms[i] = NULL;
            old->sizeSyms[i] = 0;
            old->acts[i] = NULL;
        }
    }
    else
    {
        unsigned int width;
        for (i = 0; i < XkbNumKbdGroups; i++)
        {
            width = new->numLevels[i];
            if (old->syms[i] != NULL)
            {
                new->syms[i] = uTypedCalloc(new->sizeSyms[i], xkb_keysym_t);
                if (!new->syms[i])
                {
                    new->syms[i] = NULL;
                    new->sizeSyms[i] = 0;
                    new->numLevels[i] = 0;
                    new->acts[i] = NULL;
                    return false;
                }
                memcpy(new->syms[i], old->syms[i],
                       new->sizeSyms[i] * sizeof(xkb_keysym_t));
                new->symsMapIndex[i] = uTypedCalloc(width, int);
                if (!new->symsMapIndex[i])
                {
                    free(new->syms[i]);
                    new->syms[i] = NULL;
                    new->sizeSyms[i] = 0;
                    new->numLevels[i] = 0;
                    new->acts[i] = NULL;
                    return false;
                }
                memcpy(new->symsMapIndex[i], old->symsMapIndex[i],
                       width * sizeof(int));
                new->symsMapNumEntries[i] = uTypedCalloc(width, unsigned int);
                if (!new->symsMapNumEntries[i])
                {
                    free(new->syms[i]);
                    new->syms[i] = NULL;
                    new->sizeSyms[i] = 0;
                    free(new->symsMapIndex[i]);
                    new->symsMapIndex[i] = NULL;
                    new->numLevels[i] = 0;
                    new->acts[i] = NULL;
                    return false;
                }
                memcpy(new->symsMapNumEntries[i], old->symsMapNumEntries[i],
                       width * sizeof(unsigned int));
            }
            if (old->acts[i] != NULL)
            {
                new->acts[i] = uTypedCalloc(width, union xkb_action);
                if (!new->acts[i])
                {
                    free(new->syms[i]);
                    new->syms[i] = NULL;
                    new->sizeSyms[i] = 0;
                    free(new->symsMapIndex[i]);
                    new->symsMapIndex[i] = NULL;
                    free(new->symsMapNumEntries[i]);
                    new->symsMapNumEntries[i] = NULL;
                    new->numLevels[i] = 0;
                    return false;
                }
                memcpy(new->acts[i], old->acts[i],
                       width * sizeof(union xkb_action));
            }
        }
    }
    return true;
}

/***====================================================================***/

typedef struct _ModMapEntry
{
    CommonInfo defs;
    bool haveSymbol;
    int modifier;
    union
    {
        unsigned long keyName;
        xkb_keysym_t keySym;
    } u;
} ModMapEntry;

#define	SYMBOLS_INIT_SIZE	110
#define	SYMBOLS_CHUNK		20
typedef struct _SymbolsInfo
{
    char *name;         /* e.g. pc+us+inet(evdev) */
    int errorCount;
    unsigned fileID;
    unsigned merge;
    unsigned explicit_group;
    unsigned groupInfo;
    unsigned szKeys;
    unsigned nKeys;
    KeyInfo *keys;
    KeyInfo dflt;
    VModInfo vmods;
    ActionInfo *action;
    xkb_atom_t groupNames[XkbNumKbdGroups];

    ModMapEntry *modMap;
    AliasInfo *aliases;
} SymbolsInfo;

static void
InitSymbolsInfo(SymbolsInfo * info, struct xkb_keymap *keymap)
{
    int i;

    info->name = NULL;
    info->explicit_group = 0;
    info->errorCount = 0;
    info->fileID = 0;
    info->merge = MergeOverride;
    info->groupInfo = 0;
    info->szKeys = SYMBOLS_INIT_SIZE;
    info->nKeys = 0;
    info->keys = uTypedCalloc(SYMBOLS_INIT_SIZE, KeyInfo);
    info->modMap = NULL;
    for (i = 0; i < XkbNumKbdGroups; i++)
        info->groupNames[i] = XKB_ATOM_NONE;
    InitKeyInfo(&info->dflt);
    InitVModInfo(&info->vmods, keymap);
    info->action = NULL;
    info->aliases = NULL;
}

static void
FreeSymbolsInfo(SymbolsInfo * info)
{
    unsigned int i;

    free(info->name);
    if (info->keys)
    {
        for (i = 0; i < info->nKeys; i++)
            FreeKeyInfo(&info->keys[i]);
        free(info->keys);
    }
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

    if (key->syms[group] == NULL || key->sizeSyms[group] < sizeSyms)
    {
        key->syms[group] = uTypedRecalloc(key->syms[group],
                                          key->sizeSyms[group],
                                          sizeSyms,
                                          xkb_keysym_t);
        if (!key->syms[group]) {
            key->sizeSyms[group] = 0;
            return false;
        }
        key->sizeSyms[group] = sizeSyms;
    }
    if (!key->symsMapIndex[group] || key->numLevels[group] < numLevels)
    {
        key->symsMapIndex[group] = uTypedRealloc(key->symsMapIndex[group],
                                                 numLevels,
                                                 int);
        if (!key->symsMapIndex[group])
            return false;
        for (i = key->numLevels[group]; i < numLevels; i++)
            key->symsMapIndex[group][i] = -1;
    }
    if (!key->symsMapNumEntries[group] || key->numLevels[group] < numLevels)
    {
        key->symsMapNumEntries[group] =
            uTypedRecalloc(key->symsMapNumEntries[group],
                           key->numLevels[group],
                           numLevels,
                           unsigned int);
        if (!key->symsMapNumEntries[group])
            return false;
    }
    if ((forceActions &&
         (key->numLevels[group] < numLevels || (key->acts[group] == NULL))) ||
        (key->numLevels[group] < numLevels && (key->acts[group] != NULL)))
    {
        key->acts[group] = uTypedRecalloc(key->acts[group],
                                          key->numLevels[group],
                                          numLevels,
                                          union xkb_action);
        if (!key->acts[group])
            return false;
    }
    if (key->numLevels[group] < numLevels)
        key->numLevels[group] = numLevels;
    return true;
}

static bool
MergeKeyGroups(SymbolsInfo * info,
               KeyInfo * into, KeyInfo * from, unsigned group)
{
    xkb_keysym_t *resultSyms = NULL;
    union xkb_action *resultActs;
    unsigned int resultWidth;
    unsigned int resultSize = 0;
    int cur_idx = 0;
    int i;
    bool report, clobber;

    clobber = (from->defs.merge != MergeAugment);
    report = (warningLevel > 9) ||
        ((into->defs.fileID == from->defs.fileID) && (warningLevel > 0));
    if (into->numLevels[group] >= from->numLevels[group])
    {
        resultActs = into->acts[group];
        resultWidth = into->numLevels[group];
    }
    else
    {
        resultActs = from->acts[group];
        resultWidth = from->numLevels[group];
        into->symsMapIndex[group] = uTypedRealloc(into->symsMapIndex[group],
                                                  from->numLevels[group],
                                                  int);
        into->symsMapNumEntries[group] =
            uTypedRecalloc(into->symsMapNumEntries[group],
                           into->numLevels[group],
                           from->numLevels[group],
                           unsigned int);
        if (!into->symsMapIndex[group] || !into->symsMapNumEntries[group])
        {
            WSGO("Could not allocate level indices for key info merge\n");
            ACTION("Group %d of key %s not merged\n", group,
                   longText(into->name));

            return false;
        }
        for (i = into->numLevels[group]; i < from->numLevels[group]; i++)
            into->symsMapIndex[group][i] = -1;
    }

    if ((resultActs == NULL) && (into->acts[group] || from->acts[group]))
    {
        resultActs = uTypedCalloc(resultWidth, union xkb_action);
        if (!resultActs)
        {
            WSGO("Could not allocate actions for group merge\n");
            ACTION("Group %d of key %s not merged\n", group,
                    longText(into->name));
            return false;
        }
        for (i = 0; i < resultWidth; i++)
        {
            union xkb_action *fromAct, *toAct;
            fromAct = (from->acts[group] ? &from->acts[group][i] : NULL);
            toAct = (into->acts[group] ? &into->acts[group][i] : NULL);
            if (((fromAct == NULL) || (fromAct->type == XkbSA_NoAction))
                && (toAct != NULL))
            {
                resultActs[i] = *toAct;
            }
            else if (((toAct == NULL) || (toAct->type == XkbSA_NoAction))
                     && (fromAct != NULL))
            {
                resultActs[i] = *fromAct;
            }
            else
            {
                union xkb_action *use, *ignore;
                if (clobber)
                {
                    use = fromAct;
                    ignore = toAct;
                }
                else
                {
                    use = toAct;
                    ignore = fromAct;
                }
                if (report)
                {
                    WARN
                        ("Multiple actions for level %d/group %d on key %s\n",
                         i + 1, group + 1, longText(into->name));
                    ACTION("Using %s, ignoring %s\n",
                            XkbcActionTypeText(use->type),
                            XkbcActionTypeText(ignore->type));
                }
                if (use)
                    resultActs[i] = *use;
            }
        }
    }

    for (i = 0; i < resultWidth; i++)
    {
        unsigned int fromSize = 0;
        unsigned toSize = 0;

        if (from->symsMapNumEntries[group] && (i < from->numLevels[group]))
            fromSize = from->symsMapNumEntries[group][i];
        if (into->symsMapNumEntries[group] && (i < into->numLevels[group]))
            toSize = into->symsMapNumEntries[group][i];

        if (fromSize == 0 || fromSize == toSize || clobber)
        {
            fromSize += toSize;
        }
        else if (toSize == 0)
        {
            resultSize += fromSize;
        }
    }

    if (resultSize == 0)
        goto out;

    resultSyms = uTypedCalloc(resultSize, xkb_keysym_t);
    if (!resultSyms)
    {
        WSGO("Could not allocate symbols for group merge\n");
        ACTION("Group %d of key %s not merged\n", group, longText(into->name));
        return false;
    }

    for (i = 0; i < resultWidth; i++)
    {
        enum { NONE, FROM, TO } use;
        unsigned int fromSize = 0;
        unsigned int toSize = 0;

        if (from->symsMapNumEntries[group] && (i < from->numLevels[group]))
            fromSize = from->symsMapNumEntries[group][i];
        if (into->symsMapNumEntries[group] && (i < into->numLevels[group]))
            toSize = into->symsMapNumEntries[group][i];

        if (!fromSize && !toSize)
        {
            into->symsMapIndex[group][i] = -1;
            into->symsMapNumEntries[group][i] = 0;
            continue;
        }

        if ((fromSize && !toSize) || clobber)
            use = FROM;
        else
            use = TO;

        if (toSize && fromSize && report)
        {
            INFO("Multiple symbols for group %d, level %d on key %s\n",
                 group + 1, i + 1, longText(into->name));
            ACTION("Using %s, ignoring %s\n",
                   (use == FROM ? "from" : "to"),
                   (use == FROM ? "to" : "from"));
        }

        if (use == FROM)
        {
            memcpy(&resultSyms[cur_idx],
                   &from->syms[group][from->symsMapIndex[group][i]],
                   from->symsMapNumEntries[group][i] * sizeof(xkb_keysym_t));
            into->symsMapIndex[group][i] = cur_idx;
            into->symsMapNumEntries[group][i] =
                from->symsMapNumEntries[group][i];
        }
        else
        {
            memcpy(&resultSyms[cur_idx],
                   &into->syms[group][from->symsMapIndex[group][i]],
                   into->symsMapNumEntries[group][i] * sizeof(xkb_keysym_t));
            into->symsMapIndex[group][i] = cur_idx;
        }
        cur_idx += into->symsMapNumEntries[group][i];
    }

out:
    if (resultActs != into->acts[group])
        free(into->acts[group]);
    if (resultActs != from->acts[group])
        free(from->acts[group]);
    into->numLevels[group] = resultWidth;
    free(into->syms[group]);
    into->syms[group] = resultSyms;
    free(from->syms[group]);
    from->syms[group] = NULL;
    from->sizeSyms[group] = 0;
    into->sizeSyms[group] = resultSize;
    free(from->symsMapIndex[group]);
    from->symsMapIndex[group] = NULL;
    free(from->symsMapNumEntries[group]);
    from->symsMapNumEntries[group] = NULL;
    into->acts[group] = resultActs;
    from->acts[group] = NULL;
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

    if (from->defs.merge == MergeReplace)
    {
        for (i = 0; i < XkbNumKbdGroups; i++)
        {
            if (into->numLevels[i] != 0)
            {
                free(into->syms[i]);
                free(into->acts[i]);
            }
        }
        *into = *from;
        memset(from, 0, sizeof(KeyInfo));
        return true;
    }
    report = ((warningLevel > 9) ||
              ((into->defs.fileID == from->defs.fileID)
               && (warningLevel > 0)));
    for (i = 0; i < XkbNumKbdGroups; i++)
    {
        if (from->numLevels[i] > 0)
        {
            if (into->numLevels[i] == 0)
            {
                into->numLevels[i] = from->numLevels[i];
                into->syms[i] = from->syms[i];
                into->sizeSyms[i] = from->sizeSyms[i];
                into->symsMapIndex[i] = from->symsMapIndex[i];
                into->symsMapNumEntries[i] = from->symsMapNumEntries[i];
                into->acts[i] = from->acts[i];
                into->symsDefined |= (1 << i);
                from->syms[i] = NULL;
                from->sizeSyms[i] = 0;
                from->symsMapIndex[i] = NULL;
                from->symsMapNumEntries[i] = NULL;
                from->acts[i] = NULL;
                from->numLevels[i] = 0;
                from->symsDefined &= ~(1 << i);
                if (into->syms[i])
                    into->defs.defined |= _Key_Syms;
                if (into->acts[i])
                    into->defs.defined |= _Key_Acts;
            }
            else
            {
                if (report)
                {
                    if (into->syms[i])
                        collide |= _Key_Syms;
                    if (into->acts[i])
                        collide |= _Key_Acts;
                }
                MergeKeyGroups(info, into, from, (unsigned) i);
            }
        }
        if (from->types[i] != XKB_ATOM_NONE)
        {
            if ((into->types[i] != XKB_ATOM_NONE) && report &&
                (into->types[i] != from->types[i]))
            {
                xkb_atom_t use, ignore;
                collide |= _Key_Types;
                if (from->defs.merge != MergeAugment)
                {
                    use = from->types[i];
                    ignore = into->types[i];
                }
                else
                {
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
            if ((from->defs.merge != MergeAugment)
                || (into->types[i] == XKB_ATOM_NONE))
            {
                into->types[i] = from->types[i];
            }
        }
    }
    if (UseNewField(_Key_Behavior, &into->defs, &from->defs, &collide))
    {
        into->behavior = from->behavior;
        into->defs.defined |= _Key_Behavior;
    }
    if (UseNewField(_Key_VModMap, &into->defs, &from->defs, &collide))
    {
        into->vmodmap = from->vmodmap;
        into->defs.defined |= _Key_VModMap;
    }
    if (UseNewField(_Key_Repeat, &into->defs, &from->defs, &collide))
    {
        into->repeat = from->repeat;
        into->defs.defined |= _Key_Repeat;
    }
    if (UseNewField(_Key_Type_Dflt, &into->defs, &from->defs, &collide))
    {
        into->dfltType = from->dfltType;
        into->defs.defined |= _Key_Type_Dflt;
    }
    if (UseNewField(_Key_GroupInfo, &into->defs, &from->defs, &collide))
    {
        into->groupInfo = from->groupInfo;
        into->defs.defined |= _Key_GroupInfo;
    }
    if (collide)
    {
        WARN("Symbol map for key %s redefined\n",
              longText(into->name));
        ACTION("Using %s definition for conflicting fields\n",
                (from->defs.merge == MergeAugment ? "first" : "last"));
    }
    return true;
}

static bool
AddKeySymbols(SymbolsInfo *info, KeyInfo *key, struct xkb_keymap *keymap)
{
    unsigned int i;
    unsigned long real_name;

    for (i = 0; i < info->nKeys; i++)
    {
        if (info->keys[i].name == key->name)
            return MergeKeys(info, keymap, &info->keys[i], key);
    }
    if (FindKeyNameForAlias(keymap, key->name, &real_name))
    {
        for (i = 0; i < info->nKeys; i++)
        {
            if (info->keys[i].name == real_name)
                return MergeKeys(info, keymap, &info->keys[i], key);
        }
    }
    if (info->nKeys >= info->szKeys)
    {
        info->szKeys += SYMBOLS_CHUNK;
        info->keys =
            uTypedRecalloc(info->keys, info->nKeys, info->szKeys, KeyInfo);
        if (!info->keys)
        {
            WSGO("Could not allocate key symbols descriptions\n");
            ACTION("Some key symbols definitions may be lost\n");
            return false;
        }
    }
    return CopyKeyInfo(key, &info->keys[info->nKeys++], true);
}

static bool
AddModMapEntry(SymbolsInfo * info, ModMapEntry * new)
{
    ModMapEntry *mm;
    bool clobber;

    clobber = (new->defs.merge != MergeAugment);
    for (mm = info->modMap; mm != NULL; mm = (ModMapEntry *) mm->defs.next)
    {
        if (new->haveSymbol && mm->haveSymbol
            && (new->u.keySym == mm->u.keySym))
        {
            unsigned use, ignore;
            if (mm->modifier != new->modifier)
            {
                if (clobber)
                {
                    use = new->modifier;
                    ignore = mm->modifier;
                }
                else
                {
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
            (new->u.keyName == mm->u.keyName))
        {
            unsigned use, ignore;
            if (mm->modifier != new->modifier)
            {
                if (clobber)
                {
                    use = new->modifier;
                    ignore = mm->modifier;
                }
                else
                {
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
    if (mm == NULL)
    {
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
                     unsigned merge, struct xkb_keymap *keymap)
{
    unsigned int i;
    KeyInfo *key;

    if (from->errorCount > 0)
    {
        into->errorCount += from->errorCount;
        return;
    }
    if (into->name == NULL)
    {
        into->name = from->name;
        from->name = NULL;
    }
    for (i = 0; i < XkbNumKbdGroups; i++)
    {
        if (from->groupNames[i] != XKB_ATOM_NONE)
        {
            if ((merge != MergeAugment) ||
                (into->groupNames[i] == XKB_ATOM_NONE))
                into->groupNames[i] = from->groupNames[i];
        }
    }
    for (i = 0, key = from->keys; i < from->nKeys; i++, key++)
    {
        if (merge != MergeDefault)
            key->defs.merge = merge;
        if (!AddKeySymbols(into, key, keymap))
            into->errorCount++;
    }
    if (from->modMap != NULL)
    {
        ModMapEntry *mm, *next;
        for (mm = from->modMap; mm != NULL; mm = next)
        {
            if (merge != MergeDefault)
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
                  unsigned merge, SymbolsInfo *info);

static bool
HandleIncludeSymbols(IncludeStmt *stmt, struct xkb_keymap *keymap,
                     SymbolsInfo *info)
{
    unsigned newMerge;
    XkbFile *rtrn;
    SymbolsInfo included;
    bool haveSelf;

    haveSelf = false;
    if ((stmt->file == NULL) && (stmt->map == NULL))
    {
        haveSelf = true;
        included = *info;
        memset(info, 0, sizeof(SymbolsInfo));
    }
    else if (ProcessIncludeFile(keymap->ctx, stmt, XkmSymbolsIndex, &rtrn,
                                &newMerge))
    {
        InitSymbolsInfo(&included, keymap);
        included.fileID = included.dflt.defs.fileID = rtrn->id;
        included.merge = included.dflt.defs.merge = MergeOverride;
        if (stmt->modifier)
        {
            included.explicit_group = atoi(stmt->modifier) - 1;
        }
        else
        {
            included.explicit_group = info->explicit_group;
        }
        HandleSymbolsFile(rtrn, keymap, MergeOverride, &included);
        if (stmt->stmt != NULL)
        {
            free(included.name);
            included.name = stmt->stmt;
            stmt->stmt = NULL;
        }
        FreeXKBFile(rtrn);
    }
    else
    {
        info->errorCount += 10;
        return false;
    }
    if ((stmt->next != NULL) && (included.errorCount < 1))
    {
        IncludeStmt *next;
        unsigned op;
        SymbolsInfo next_incl;

        for (next = stmt->next; next != NULL; next = next->next)
        {
            if ((next->file == NULL) && (next->map == NULL))
            {
                haveSelf = true;
                MergeIncludedSymbols(&included, info, next->merge, keymap);
                FreeSymbolsInfo(info);
            }
            else if (ProcessIncludeFile(keymap->ctx, next, XkmSymbolsIndex,
                                        &rtrn, &op))
            {
                InitSymbolsInfo(&next_incl, keymap);
                next_incl.fileID = next_incl.dflt.defs.fileID = rtrn->id;
                next_incl.merge = next_incl.dflt.defs.merge = MergeOverride;
                if (next->modifier)
                {
                    next_incl.explicit_group = atoi(next->modifier) - 1;
                }
                else
                {
                    next_incl.explicit_group = info->explicit_group;
                }
                HandleSymbolsFile(rtrn, keymap, MergeOverride, &next_incl);
                MergeIncludedSymbols(&included, &next_incl, op, keymap);
                FreeSymbolsInfo(&next_incl);
                FreeXKBFile(rtrn);
            }
            else
            {
                info->errorCount += 10;
                FreeSymbolsInfo(&included);
                return false;
            }
        }
    }
    else if (stmt->next)
    {
        info->errorCount += included.errorCount;
    }
    if (haveSelf)
        *info = included;
    else
    {
        MergeIncludedSymbols(info, &included, newMerge, keymap);
        FreeSymbolsInfo(&included);
    }
    return (info->errorCount == 0);
}

#define	SYMBOLS 1
#define	ACTIONS	2

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

    if (arrayNdx == NULL)
    {
        int i;
        unsigned defined;
        if (what == SYMBOLS)
            defined = key->symsDefined;
        else
            defined = key->actsDefined;

        for (i = 0; i < XkbNumKbdGroups; i++)
        {
            if ((defined & (1 << i)) == 0)
            {
                *ndx_rtrn = i;
                return true;
            }
        }
        ERROR("Too many groups of %s for key %s (max %d)\n", name,
               longText(key->name), XkbNumKbdGroups + 1);
        ACTION("Ignoring %s defined for extra groups\n", name);
        return false;
    }
    if (!ExprResolveGroup(keymap->ctx, arrayNdx, &tmp))
    {
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
    if (value == NULL)
    {
        key->symsDefined |= (1 << ndx);
        return true;
    }
    if (value->op != ExprKeysymList)
    {
        ERROR("Expected a list of symbols, found %s\n", exprOpText(value->op));
        ACTION("Ignoring symbols for group %d of %s\n", ndx + 1,
                longText(key->name));
        return false;
    }
    if (key->sizeSyms[ndx] != 0)
    {
        ERROR("Symbols for key %s, group %d already defined\n",
               longText(key->name), ndx + 1);
        ACTION("Ignoring duplicate definition\n");
        return false;
    }
    nSyms = value->value.list.nSyms;
    nLevels = value->value.list.nLevels;
    if (((key->numLevels[ndx] < nSyms) || (key->syms[ndx] == NULL)) &&
        (!ResizeKeyGroup(key, ndx, nLevels, nSyms, false)))
    {
        WSGO("Could not resize group %d of key %s to contain %d levels\n",
             ndx + 1, longText(key->name), nSyms);
        ACTION("Symbols lost\n");
        return false;
    }
    key->symsDefined |= (1 << ndx);
    for (i = 0; i < nLevels; i++) {
        key->symsMapIndex[ndx][i] = value->value.list.symsMapIndex[i];
        key->symsMapNumEntries[ndx][i] = value->value.list.symsNumEntries[i];
        for (j = 0; j < key->symsMapNumEntries[ndx][i]; j++) {
            if (key->symsMapIndex[ndx][i] + j >= nSyms)
                abort();
            if (!LookupKeysym(value->value.list.syms[value->value.list.symsMapIndex[i] + j],
                              &key->syms[ndx][key->symsMapIndex[ndx][i] + j])) {
                WARN("Could not resolve keysym %s for key %s, group %d (%s), level %d\n",
                     value->value.list.syms[i], longText(key->name), ndx + 1,
                     xkb_atom_text(keymap->ctx, info->groupNames[ndx]), nSyms);
                while (--j >= 0)
                    key->syms[ndx][key->symsMapIndex[ndx][i] + j] = XKB_KEY_NoSymbol;
                key->symsMapIndex[ndx][i] = -1;
                key->symsMapNumEntries[ndx][i] = 0;
                break;
            }
        }
    }
    for (j = key->numLevels[ndx] - 1;
         j >= 0 && key->symsMapNumEntries[ndx][j] == 0; j--)
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

    if (value == NULL)
    {
        key->actsDefined |= (1 << ndx);
        return true;
    }
    if (value->op != ExprActionList)
    {
        WSGO("Bad expression type (%d) for action list value\n", value->op);
        ACTION("Ignoring actions for group %d of %s\n", ndx,
                longText(key->name));
        return false;
    }
    if (key->acts[ndx] != NULL)
    {
        WSGO("Actions for key %s, group %d already defined\n",
              longText(key->name), ndx);
        return false;
    }
    for (nActs = 0, act = value->value.child; act != NULL; nActs++)
    {
        act = (ExprDef *) act->common.next;
    }
    if (nActs < 1)
    {
        WSGO("Action list but not actions in AddActionsToKey\n");
        return false;
    }
    if (((key->numLevels[ndx] < nActs) || (key->acts[ndx] == NULL)) &&
        (!ResizeKeyGroup(key, ndx, nActs, nActs, true)))
    {
        WSGO("Could not resize group %d of key %s\n", ndx,
              longText(key->name));
        ACTION("Actions lost\n");
        return false;
    }
    key->actsDefined |= (1 << ndx);

    toAct = (struct xkb_any_action *) key->acts[ndx];
    act = value->value.child;
    for (i = 0; i < nActs; i++, toAct++)
    {
        if (!HandleActionDef(act, keymap, toAct, info->action))
        {
            ERROR("Illegal action definition for %s\n",
                   longText(key->name));
            ACTION("Action for group %d/level %d ignored\n", ndx + 1, i + 1);
        }
        act = (ExprDef *) act->common.next;
    }
    return true;
}

static const LookupEntry lockingEntries[] = {
    {"true", XkbKB_Lock},
    {"yes", XkbKB_Lock},
    {"on", XkbKB_Lock},
    {"false", XkbKB_Default},
    {"no", XkbKB_Default},
    {"off", XkbKB_Default},
    {"permanent", XkbKB_Lock | XkbKB_Permanent},
    {NULL, 0}
};

static const LookupEntry repeatEntries[] = {
    {"true", RepeatYes},
    {"yes", RepeatYes},
    {"on", RepeatYes},
    {"false", RepeatNo},
    {"no", RepeatNo},
    {"off", RepeatNo},
    {"default", RepeatUndefined},
    {NULL, 0}
};

static bool
SetSymbolsField(KeyInfo *key, struct xkb_keymap *keymap, char *field,
                ExprDef *arrayNdx, ExprDef *value, SymbolsInfo *info)
{
    bool ok = true;
    ExprResult tmp;

    if (strcasecmp(field, "type") == 0)
    {
        ExprResult ndx;
        if ((!ExprResolveString(keymap->ctx, value, &tmp))
            && (warningLevel > 0))
        {
            WARN("The type field of a key symbol map must be a string\n");
            ACTION("Ignoring illegal type definition\n");
        }
        if (arrayNdx == NULL)
        {
            key->dfltType = xkb_atom_intern(keymap->ctx, tmp.str);
            key->defs.defined |= _Key_Type_Dflt;
        }
        else if (!ExprResolveGroup(keymap->ctx, arrayNdx, &ndx))
        {
            ERROR("Illegal group index for type of key %s\n",
                   longText(key->name));
            ACTION("Definition with non-integer array index ignored\n");
            free(tmp.str);
            return false;
        }
        else
        {
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
             (strcasecmp(field, "virtualmodifiers") == 0))
    {
        ok = ExprResolveVModMask(value, &tmp, keymap);
        if (ok)
        {
            key->vmodmap = (tmp.uval >> 8);
            key->defs.defined |= _Key_VModMap;
        }
        else
        {
            ERROR("Expected a virtual modifier mask, found %s\n",
                   exprOpText(value->op));
            ACTION("Ignoring virtual modifiers definition for key %s\n",
                    longText(key->name));
        }
    }
    else if ((strcasecmp(field, "locking") == 0) ||
             (strcasecmp(field, "lock") == 0) ||
             (strcasecmp(field, "locks") == 0))
    {
        ok = ExprResolveEnum(keymap->ctx, value, &tmp, lockingEntries);
        if (ok)
            key->behavior.type = tmp.uval;
        key->defs.defined |= _Key_Behavior;
    }
    else if ((strcasecmp(field, "radiogroup") == 0) ||
             (strcasecmp(field, "permanentradiogroup") == 0) ||
             (strcasecmp(field, "allownone") == 0))
    {
        ERROR("Radio groups not supported\n");
        ACTION("Ignoring radio group specification for key %s\n", longText(key->name));
        return false;
    }
    else if (uStrCasePrefix("overlay", field) ||
             uStrCasePrefix("permanentoverlay", field))
    {
        ERROR("Overlays not supported\n");
        ACTION("Ignoring overlay specification for key %s\n", longText(key->name));
    }
    else if ((strcasecmp(field, "repeating") == 0) ||
             (strcasecmp(field, "repeats") == 0) ||
             (strcasecmp(field, "repeat") == 0))
    {
        ok = ExprResolveEnum(keymap->ctx, value, &tmp, repeatEntries);
        if (!ok)
        {
            ERROR("Illegal repeat setting for %s\n",
                   longText(key->name));
            ACTION("Non-boolean repeat setting ignored\n");
            return false;
        }
        key->repeat = tmp.uval;
        key->defs.defined |= _Key_Repeat;
    }
    else if ((strcasecmp(field, "groupswrap") == 0) ||
             (strcasecmp(field, "wrapgroups") == 0))
    {
        ok = ExprResolveBoolean(keymap->ctx, value, &tmp);
        if (!ok)
        {
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
             (strcasecmp(field, "clampgroups") == 0))
    {
        ok = ExprResolveBoolean(keymap->ctx, value, &tmp);
        if (!ok)
        {
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
             (strcasecmp(field, "redirectgroups") == 0))
    {
        if (!ExprResolveGroup(keymap->ctx, value, &tmp))
        {
            ERROR("Illegal group index for redirect of key %s\n",
                   longText(key->name));
            ACTION("Definition with non-integer group ignored\n");
            return false;
        }
        key->groupInfo =
            XkbSetGroupInfo(0, XkbRedirectIntoRange, tmp.uval - 1);
        key->defs.defined |= _Key_GroupInfo;
    }
    else
    {
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

    if ((arrayNdx == NULL) && (warningLevel > 0))
    {
        WARN("You must specify an index when specifying a group name\n");
        ACTION("Group name definition without array subscript ignored\n");
        return false;
    }
    if (!ExprResolveGroup(keymap->ctx, arrayNdx, &tmp))
    {
        ERROR("Illegal index in group name definition\n");
        ACTION("Definition with non-integer array index ignored\n");
        return false;
    }
    if (!ExprResolveString(keymap->ctx, value, &name))
    {
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
    ExprResult elem, field, tmp;
    ExprDef *arrayNdx;
    bool ret;

    if (ExprResolveLhs(keymap, stmt->name, &elem, &field, &arrayNdx) == 0)
        return 0;               /* internal error, already reported */
    if (elem.str && (strcasecmp(elem.str, "key") == 0))
    {
        ret = SetSymbolsField(&info->dflt, keymap, field.str, arrayNdx,
                              stmt->value, info);
    }
    else if ((elem.str == NULL) && ((strcasecmp(field.str, "name") == 0) ||
                                    (strcasecmp(field.str, "groupname") ==
                                     0)))
    {
        ret = SetGroupName(info, keymap, arrayNdx, stmt->value);
    }
    else if ((elem.str == NULL)
             && ((strcasecmp(field.str, "groupswrap") == 0) ||
                 (strcasecmp(field.str, "wrapgroups") == 0)))
    {
        if (!ExprResolveBoolean(keymap->ctx, stmt->value, &tmp))
        {
            ERROR("Illegal setting for global groupsWrap\n");
            ACTION("Non-boolean value ignored\n");
            ret = false;
        }
        else {
            if (tmp.uval)
                info->groupInfo = XkbWrapIntoRange;
            else
                info->groupInfo = XkbClampIntoRange;
            ret = true;
        }
    }
    else if ((elem.str == NULL)
             && ((strcasecmp(field.str, "groupsclamp") == 0) ||
                 (strcasecmp(field.str, "clampgroups") == 0)))
    {
        if (!ExprResolveBoolean(keymap->ctx, stmt->value, &tmp))
        {
            ERROR("Illegal setting for global groupsClamp\n");
            ACTION("Non-boolean value ignored\n");
            return false;
        }
        else {
            if (tmp.uval)
                info->groupInfo = XkbClampIntoRange;
            else
                info->groupInfo = XkbWrapIntoRange;
            ret = true;
        }
    }
    else if ((elem.str == NULL)
             && ((strcasecmp(field.str, "groupsredirect") == 0) ||
                 (strcasecmp(field.str, "redirectgroups") == 0)))
    {
        if (!ExprResolveGroup(keymap->ctx, stmt->value, &tmp))
        {
            ERROR("Illegal group index for global groupsRedirect\n");
            ACTION("Definition with non-integer group ignored\n");
            ret = false;
        }
        else {
            info->groupInfo = XkbSetGroupInfo(0, XkbRedirectIntoRange,
                                              tmp.uval);
            ret = true;
        }
    }
    else if ((elem.str == NULL) && (strcasecmp(field.str, "allownone") == 0))
    {
        ERROR("Radio groups not supported\n");
        ACTION("Ignoring \"allow none\" specification\n");
        ret = false;
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

    for (; def != NULL; def = (VarDef *) def->common.next)
    {
        if ((def->name) && (def->name->type == ExprFieldRef))
        {
            ok = HandleSymbolsVar(def, keymap, info);
            continue;
        }
        else
        {
            if (def->name == NULL)
            {
                if ((def->value == NULL)
                    || (def->value->op == ExprKeysymList))
                    field.str = strdup("symbols");
                else
                    field.str = strdup("actions");
                arrayNdx = NULL;
            }
            else
            {
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

    if ((key->typesDefined | key->symsDefined | key->actsDefined) & ~1)
    {
        int i;
        WARN("For the map %s an explicit group specified\n", info->name);
        WARN("but key %s has more than one group defined\n",
              longText(key->name));
        ACTION("All groups except first one will be ignored\n");
        for (i = 1; i < XkbNumKbdGroups; i++)
        {
            key->numLevels[i] = 0;
            free(key->syms[i]);
            key->syms[i] = NULL;
            free(key->acts[i]);
            key->acts[i] = NULL;
            key->types[i] = 0;
        }
    }
    key->typesDefined = key->symsDefined = key->actsDefined = 1 << group;

    key->numLevels[group] = key->numLevels[0];
    key->numLevels[0] = 0;
    key->syms[group] = key->syms[0];
    key->syms[0] = NULL;
    key->sizeSyms[group] = key->sizeSyms[0];
    key->sizeSyms[0] = 0;
    key->symsMapIndex[group] = key->symsMapIndex[0];
    key->symsMapIndex[0] = NULL;
    key->symsMapNumEntries[group] = key->symsMapNumEntries[0];
    key->symsMapNumEntries[0] = NULL;
    key->acts[group] = key->acts[0];
    key->acts[0] = NULL;
    key->types[group] = key->types[0];
    key->types[0] = 0;
    return true;
}

static int
HandleSymbolsDef(SymbolsDef *stmt, struct xkb_keymap *keymap,
                 SymbolsInfo *info)
{
    KeyInfo key;

    InitKeyInfo(&key);
    CopyKeyInfo(&info->dflt, &key, false);
    key.defs.merge = stmt->merge;
    key.name = KeyNameToLong(stmt->keyName);
    if (!HandleSymbolsBody((VarDef *) stmt->symbols, keymap, &key, info))
    {
        info->errorCount++;
        return false;
    }

    if (!SetExplicitGroup(info, &key))
    {
        info->errorCount++;
        return false;
    }

    if (!AddKeySymbols(info, &key, keymap))
    {
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

    if (!LookupModIndex(keymap->ctx, NULL, def->modifier, TypeInt, &rtrn))
    {
        ERROR("Illegal modifier map definition\n");
        ACTION("Ignoring map for non-modifier \"%s\"\n",
                xkb_atom_text(keymap->ctx, def->modifier));
        return false;
    }
    ok = true;
    tmp.modifier = rtrn.uval;
    for (key = def->keys; key != NULL; key = (ExprDef *) key->common.next)
    {
        if ((key->op == ExprValue) && (key->type == TypeKeyName))
        {
            tmp.haveSymbol = false;
            tmp.u.keyName = KeyNameToLong(key->value.keyName);
        }
        else if (ExprResolveKeySym(keymap->ctx, key, &rtrn))
        {
            tmp.haveSymbol = true;
            tmp.u.keySym = rtrn.uval;
        }
        else
        {
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
                  unsigned merge, SymbolsInfo *info)
{
    ParseCommon *stmt;

    free(info->name);
    info->name = uDupString(file->name);
    stmt = file->defs;
    while (stmt)
    {
        switch (stmt->stmtType)
        {
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
        if (info->errorCount > 10)
        {
#ifdef NOISY
            ERROR("Too many errors\n");
#endif
            ACTION("Abandoning symbols file \"%s\"\n", file->topName);
            break;
        }
    }
}

static bool
FindKeyForSymbol(struct xkb_keymap *keymap, xkb_keysym_t sym,
                 xkb_keycode_t *kc_rtrn)
{
    xkb_keycode_t key;
    unsigned int group, level;

    for (key = keymap->min_key_code; key <= keymap->max_key_code; key++)
    {
        for (group = 0; group < XkbKeyNumGroups(keymap, key); group++)
        {
            for (level = 0; level < XkbKeyGroupWidth(keymap, key, group);
                 level++)
            {
                if (XkbKeyNumSyms(keymap, key, group, level) != 1 ||
                    (XkbKeySymEntry(keymap, key, group, level))[0] != sym)
                    continue;
                *kc_rtrn = key;
                return true;
            }
        }
    }

    return false;
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
    unsigned n;
    const char *name = xkb_atom_text(keymap->ctx, atom);

    if (keymap && keymap->map && keymap->map->types)
    {
        for (n = 0; n < keymap->map->num_types; n++)
        {
            if (strcmp(keymap->map->types[n].name, name) == 0)
            {
                *type_rtrn = n;
                return true;
            }
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
 */
static bool
FindAutomaticType(struct xkb_keymap *keymap, int width, xkb_keysym_t *syms,
                  xkb_atom_t *typeNameRtrn, bool *autoType)
{
    *autoType = false;
    if ((width == 1) || (width == 0))
    {
        *typeNameRtrn = xkb_atom_intern(keymap->ctx, "ONE_LEVEL");
        *autoType = true;
    }
    else if (width == 2)
    {
        if (syms && XkbcKSIsLower(syms[0]) && XkbcKSIsUpper(syms[1]))
        {
            *typeNameRtrn = xkb_atom_intern(keymap->ctx, "ALPHABETIC");
        }
        else if (syms && (XkbKSIsKeypad(syms[0]) || XkbKSIsKeypad(syms[1])))
        {
            *typeNameRtrn = xkb_atom_intern(keymap->ctx, "KEYPAD");
            *autoType = true;
        }
        else
        {
            *typeNameRtrn = xkb_atom_intern(keymap->ctx, "TWO_LEVEL");
            *autoType = true;
        }
    }
    else if (width <= 4)
    {
        if (syms && XkbcKSIsLower(syms[0]) && XkbcKSIsUpper(syms[1]))
            if (XkbcKSIsLower(syms[2]) && XkbcKSIsUpper(syms[3]))
                *typeNameRtrn =
                    xkb_atom_intern(keymap->ctx, "FOUR_LEVEL_ALPHABETIC");
            else
                *typeNameRtrn = xkb_atom_intern(keymap->ctx,
                                                "FOUR_LEVEL_SEMIALPHABETIC");

        else if (syms && (XkbKSIsKeypad(syms[0]) || XkbKSIsKeypad(syms[1])))
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
    for (i = XkbNumKbdGroups - 1; i >= 0; i--)
    {
        if (defined & (1 << i))
            break;
    }
    lastGroup = i;

    if (lastGroup == 0)
        return;

    /* If there are empty groups between non-empty ones fill them with data */
    /* from the first group. */
    /* We can make a wrong assumption here. But leaving gaps is worse. */
    for (i = lastGroup; i > 0; i--)
    {
        if (defined & (1 << i))
            continue;
        width = key->numLevels[0];
        if (key->typesDefined & 1)
        {
            for (j = 0; j < width; j++)
            {
                key->types[i] = key->types[0];
            }
            key->typesDefined |= 1 << i;
        }
        if ((key->actsDefined & 1) && key->acts[0])
        {
            key->acts[i] = uTypedCalloc(width, union xkb_action);
            if (key->acts[i] == NULL)
                continue;
            memcpy(key->acts[i], key->acts[0],
                   width * sizeof(union xkb_action));
            key->actsDefined |= 1 << i;
        }
        if ((key->symsDefined & 1) && key->sizeSyms[0])
        {
            key->syms[i] = uTypedCalloc(key->sizeSyms[0], xkb_keysym_t);
            if (key->syms[i] == NULL)
                continue;
            memcpy(key->syms[i], key->syms[0],
                   key->sizeSyms[0] * sizeof(xkb_keysym_t));
            key->symsMapIndex[i] = uTypedCalloc(width, int);
            if (!key->symsMapIndex[i])
            {
                free(key->syms[i]);
                key->syms[i] = NULL;
                continue;
            }
            memcpy(key->symsMapIndex[i], key->symsMapIndex[0],
                   width * sizeof(int));
            key->symsMapNumEntries[i] = uTypedCalloc(width, unsigned int);
            if (!key->symsMapNumEntries[i])
            {
                free(key->syms[i]);
                key->syms[i] = NULL;
                free(key->symsMapIndex[i]);
                key->symsMapIndex[i] = NULL;
                continue;
            }
            memcpy(key->symsMapNumEntries[i], key->symsMapNumEntries[0],
                   width * sizeof(int));
            key->sizeSyms[i] = key->sizeSyms[0];
            key->symsDefined |= 1 << i;
        }
        if (defined & 1)
        {
            key->numLevels[i] = key->numLevels[0];
        }
    }
    /* If all groups are completely identical remove them all */
    /* exept the first one. */
    identical = true;
    for (i = lastGroup; i > 0; i--)
    {
        if ((key->numLevels[i] != key->numLevels[0]) ||
            (key->types[i] != key->types[0]))
        {
            identical = false;
            break;
        }
        if ((key->syms[i] != key->syms[0]) &&
            (key->syms[i] == NULL || key->syms[0] == NULL ||
             key->sizeSyms[i] != key->sizeSyms[0] ||
             memcmp(key->syms[i], key->syms[0],
                    sizeof(xkb_keysym_t) * key->sizeSyms[0])))
        {
            identical = false;
            break;
        }
        if ((key->symsMapIndex[i] != key->symsMapIndex[i]) &&
            (key->symsMapIndex[i] == NULL || key->symsMapIndex[0] == NULL ||
             memcmp(key->symsMapIndex[i], key->symsMapIndex[0],
                    key->numLevels[0] * sizeof(int))))
        {
            identical = false;
            continue;
        }
        if ((key->symsMapNumEntries[i] != key->symsMapNumEntries[i]) &&
            (key->symsMapNumEntries[i] == NULL ||
             key->symsMapNumEntries[0] == NULL ||
             memcmp(key->symsMapNumEntries[i], key->symsMapNumEntries[0],
                    key->numLevels[0] * sizeof(int))))
        {
            identical = false;
            continue;
        }
        if ((key->acts[i] != key->acts[0]) &&
            (key->acts[i] == NULL || key->acts[0] == NULL ||
             memcmp(key->acts[i], key->acts[0],
                    sizeof(union xkb_action) * key->numLevels[0])))
        {
            identical = false;
            break;
        }
    }
    if (identical)
    {
        for (i = lastGroup; i > 0; i--)
        {
            key->numLevels[i] = 0;
            free(key->syms[i]);
            key->syms[i] = NULL;
            key->sizeSyms[i] = 0;
            free(key->symsMapIndex[i]);
            key->symsMapIndex[i] = NULL;
            free(key->symsMapNumEntries[i]);
            key->symsMapNumEntries[i] = NULL;
            free(key->acts[i]);
            key->acts[i] = NULL;
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

    useAlias = (start_from == 0);

    /* get the keycode for the key. */
    if (!FindNamedKey(keymap, key->name, &kc, useAlias,
                      CreateKeyNames(keymap), start_from))
    {
        if ((start_from == 0) && (warningLevel >= 5))
        {
            WARN("Key %s not found in keycodes\n", longText(key->name));
            ACTION("Symbols ignored\n");
        }
        return false;
    }

    haveActions = false;
    for (i = width = nGroups = 0; i < XkbNumKbdGroups; i++)
    {
        if (((i + 1) > nGroups)
            && (((key->symsDefined | key->actsDefined) & (1 << i))
                || (key->typesDefined) & (1 << i)))
            nGroups = i + 1;
        if (key->acts[i])
            haveActions = true;
        autoType = false;
        /* Assign the type to the key, if it is missing. */
        if (key->types[i] == XKB_ATOM_NONE)
        {
            if (key->dfltType != XKB_ATOM_NONE)
                key->types[i] = key->dfltType;
            else if (FindAutomaticType(keymap, key->numLevels[i], key->syms[i],
                                       &key->types[i], &autoType))
            {
            }
            else
            {
                if (warningLevel >= 5)
                {
                    WARN("No automatic type for %d symbols\n",
                          (unsigned int) key->numLevels[i]);
                    ACTION("Using %s for the %s key (keycode %d)\n",
                            xkb_atom_text(keymap->ctx, key->types[i]),
                            longText(key->name), kc);
                }
            }
        }
        if (FindNamedType(keymap, key->types[i], &types[i]))
        {
            if (!autoType || key->numLevels[i] > 2)
                keymap->server->explicit[kc] |= (1 << i);
        }
        else
        {
            if (warningLevel >= 3)
            {
                WARN("Type \"%s\" is not defined\n",
                      xkb_atom_text(keymap->ctx, key->types[i]));
                ACTION("Using TWO_LEVEL for the %s key (keycode %d)\n",
                        longText(key->name), kc);
            }
            types[i] = XkbTwoLevelIndex;
        }
        /* if the type specifies fewer levels than the key has, shrink the key */
        type = &keymap->map->types[types[i]];
        if (type->num_levels < key->numLevels[i])
        {
            if (warningLevel > 0)
            {
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
        sizeSyms += key->sizeSyms[i];
    }

    if (!XkbcResizeKeySyms(keymap, kc, sizeSyms))
    {
        WSGO("Could not enlarge symbols for %s (keycode %d)\n",
              longText(key->name), kc);
        return false;
    }
    if (haveActions)
    {
        outActs = XkbcResizeKeyActions(keymap, kc, width * nGroups);
        if (outActs == NULL)
        {
            WSGO("Could not enlarge actions for %s (key %d)\n",
                  longText(key->name), kc);
            return false;
        }
        keymap->server->explicit[kc] |= XkbExplicitInterpretMask;
    }
    else
        outActs = NULL;
    if (key->defs.defined & _Key_GroupInfo)
        i = key->groupInfo;
    else
        i = keymap->map->key_sym_map[kc].group_info;

    keymap->map->key_sym_map[kc].group_info = XkbSetNumGroups(i, nGroups);
    keymap->map->key_sym_map[kc].width = width;
    keymap->map->key_sym_map[kc].sym_index = uTypedCalloc(nGroups * width,
                                                          int);
    keymap->map->key_sym_map[kc].num_syms = uTypedCalloc(nGroups * width,
                                                         unsigned int);
    for (i = 0; i < nGroups; i++)
    {
        /* assign kt_index[i] to the index of the type in map->types.
         * kt_index[i] may have been set by a previous run (if we have two
         * layouts specified). Let's not overwrite it with the ONE_LEVEL
         * default group if we dont even have keys for this group anyway.
         *
         * FIXME: There should be a better fix for this.
         */
        if (key->numLevels[i])
            keymap->map->key_sym_map[kc].kt_index[i] = types[i];
        if (key->sizeSyms[i] != 0)
        {
            /* fill key to "width" symbols*/
            for (tmp = 0; tmp < width; tmp++)
            {
                if (tmp < key->numLevels[i] && key->symsMapNumEntries[i][tmp])
                {
                    memcpy(&keymap->map->key_sym_map[kc].syms[symIndex],
                           &key->syms[i][key->symsMapIndex[i][tmp]],
                           key->symsMapNumEntries[i][tmp] *
                            sizeof(xkb_keysym_t));
                    keymap->map->key_sym_map[kc].sym_index[(i * width) + tmp] =
                        symIndex;
                    keymap->map->key_sym_map[kc].num_syms[(i * width) + tmp] =
                        key->symsMapNumEntries[i][tmp];
                    symIndex +=
                        keymap->map->key_sym_map[kc].num_syms[(i * width) + tmp];
                }
                else
                {
                    keymap->map->key_sym_map[kc].sym_index[(i * width) + tmp] = -1;
                    keymap->map->key_sym_map[kc].num_syms[(i * width) + tmp] = 0;
                }
                if ((outActs != NULL) && (key->acts[i] != NULL))
                {
                    if (tmp < key->numLevels[i])
                        outActs[tmp] = key->acts[i][tmp];
                    else
                        outActs[tmp].type = XkbSA_NoAction;
                }
            }
        }
    }
    switch (key->behavior.type & XkbKB_OpMask)
    {
    case XkbKB_Default:
        break;
    default:
        keymap->server->behaviors[kc] = key->behavior;
        keymap->server->explicit[kc] |= XkbExplicitBehaviorMask;
        break;
    }
    if (key->defs.defined & _Key_VModMap)
    {
        keymap->server->vmodmap[kc] = key->vmodmap;
        keymap->server->explicit[kc] |= XkbExplicitVModMapMask;
    }
    if (key->repeat != RepeatUndefined)
    {
        if (key->repeat == RepeatYes)
            keymap->ctrls->per_key_repeat[kc / 8] |= (1 << (kc % 8));
        else
            keymap->ctrls->per_key_repeat[kc / 8] &= ~(1 << (kc % 8));
        keymap->server->explicit[kc] |= XkbExplicitAutoRepeatMask;
    }

    if (nGroups > keymap->ctrls->num_groups)
	keymap->ctrls->num_groups = nGroups;

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
                      CreateKeyNames(keymap), 0))
    {
        if (warningLevel >= 5)
        {
            WARN("Key %s not found in keycodes\n",
                  longText(entry->u.keyName));
            ACTION("Modifier map entry for %s not updated\n",
                    XkbcModIndexText(entry->modifier));
        }
        return false;
    }
    else if (entry->haveSymbol &&
             !FindKeyForSymbol(keymap, entry->u.keySym, &kc))
    {
        if (warningLevel > 5)
        {
            WARN("Key \"%s\" not found in symbol map\n",
                  XkbcKeysymText(entry->u.keySym));
            ACTION("Modifier map entry for %s not updated\n",
                    XkbcModIndexText(entry->modifier));
        }
        return false;
    }
    keymap->map->modmap[kc] |= (1 << entry->modifier);
    return true;
}

/**
 * Handle the xkb_symbols section of an xkb file.
 *
 * @param file The parsed xkb_symbols section of the xkb file.
 * @param keymap Handle to the keyboard description to store the symbols in.
 * @param merge Merge strategy (e.g. MergeOverride).
 */
bool
CompileSymbols(XkbFile *file, struct xkb_keymap *keymap, unsigned merge)
{
    unsigned int i;
    SymbolsInfo info;
    KeyInfo *key;

    InitSymbolsInfo(&info, keymap);
    info.dflt.defs.fileID = file->id;
    info.dflt.defs.merge = merge;

    HandleSymbolsFile(file, keymap, merge, &info);

    if (info.nKeys == 0)
        goto err_info;

    if (info.errorCount != 0)
        goto err_info;

    /* alloc memory in the xkb struct */
    if (XkbcAllocNames(keymap, XkbGroupNamesMask, 0) != Success) {
        WSGO("Can not allocate names in CompileSymbols\n");
        ACTION("Symbols not added\n");
        goto err_info;
    }

    if (XkbcAllocClientMap(keymap, XkbKeySymsMask | XkbModifierMapMask, 0)
        != Success) {
        WSGO("Could not allocate client map in CompileSymbols\n");
        ACTION("Symbols not added\n");
        goto err_info;
    }

    if (XkbcAllocServerMap(keymap, XkbAllServerInfoMask, 32) != Success) {
        WSGO("Could not allocate server map in CompileSymbols\n");
        ACTION("Symbols not added\n");
        goto err_info;
    }

    if (XkbcAllocControls(keymap) != Success) {
        WSGO("Could not allocate controls in CompileSymbols\n");
        ACTION("Symbols not added\n");
        goto err_info;
    }

    /* now copy info into xkb. */
    ApplyAliases(keymap, &info.aliases);

    for (i = 0; i < XkbNumKbdGroups; i++) {
        if (info.groupNames[i] != XKB_ATOM_NONE) {
            free(UNCONSTIFY(keymap->names->groups[i]));
            keymap->names->groups[i] = xkb_atom_strdup(keymap->ctx,
                                                       info.groupNames[i]);
        }
    }

    /* sanitize keys */
    for (key = info.keys, i = 0; i < info.nKeys; i++, key++)
        PrepareKeyDef(key);

    /* copy! */
    for (key = info.keys, i = 0; i < info.nKeys; i++, key++)
        if (!CopySymbolsDef(keymap, key, 0))
            info.errorCount++;

    if (warningLevel > 3) {
        for (i = keymap->min_key_code; i <= keymap->max_key_code; i++) {
            if (keymap->names->keys[i].name[0] == '\0')
                continue;

            if (XkbKeyNumGroups(keymap, i) < 1) {
                char buf[5];
                memcpy(buf, keymap->names->keys[i].name, 4);
                buf[4] = '\0';
                WARN("No symbols defined for <%s> (keycode %d)\n", buf, i);
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
