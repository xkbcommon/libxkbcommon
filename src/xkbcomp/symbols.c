/*
 * For HPND:
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 *
 * For MIT:
 * Copyright © 2012 Intel Corporation
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 *
 * SPDX-License-Identifier: HPND AND MIT
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 * Author: Ran Benita <ran234@gmail.com>
 */

#include "config.h"

#include "xkbcommon/xkbcommon.h"
#include "xkbcommon/xkbcommon-keysyms.h"

#include "darray.h"
#include "keymap.h"
#include "xkbcomp-priv.h"
#include "darray.h"
#include "text.h"
#include "expr.h"
#include "action.h"
#include "vmod.h"
#include "include.h"
#include "keysym.h"
#include "util-mem.h"
#include "xkbcomp/ast.h"


enum key_repeat {
    KEY_REPEAT_UNDEFINED = 0,
    KEY_REPEAT_YES = 1,
    KEY_REPEAT_NO = 2,
};

enum group_field {
    GROUP_FIELD_SYMS = (1 << 0),
    GROUP_FIELD_ACTS = (1 << 1),
    GROUP_FIELD_TYPE = (1 << 2),
};

enum key_field {
    KEY_FIELD_REPEAT = (1 << 0),
    KEY_FIELD_DEFAULT_TYPE = (1 << 1),
    KEY_FIELD_GROUPINFO = (1 << 2),
    KEY_FIELD_VMODMAP = (1 << 3),
};

typedef struct {
    enum group_field defined;
    darray(struct xkb_level) levels;
    xkb_atom_t type;
} GroupInfo;

typedef struct {
    enum key_field defined;
    enum merge_mode merge;

    xkb_atom_t name;

    darray(GroupInfo) groups;

    enum key_repeat repeat;
    xkb_mod_mask_t vmodmap;
    xkb_atom_t default_type;

    enum xkb_range_exceed_type out_of_range_group_action;
    xkb_layout_index_t out_of_range_group_number;
} KeyInfo;

static void
StealLevelInfo(struct xkb_level *into, struct xkb_level *from)
{
    clear_level(into);
    if (from->num_syms > 1) {
        /* Steal */
        into->s.syms = steal(&from->s.syms);
    } else {
        into->s.sym = from->s.sym;
    }
    into->num_syms = from->num_syms;
    from->num_syms = 0;

    if (from->num_actions > 1) {
        into->a.actions = steal(&from->a.actions);
    } else {
        into->a.action = from->a.action;
    }
    into->num_actions = from->num_actions;
    from->num_actions = 0;
}

static void
InitGroupInfo(GroupInfo *groupi)
{
    memset(groupi, 0, sizeof(*groupi));
}

static void
ClearGroupInfo(GroupInfo *groupi)
{
    struct xkb_level *leveli;
    darray_foreach(leveli, groupi->levels)
        clear_level(leveli);
    darray_free(groupi->levels);
}

static void
CopyGroupInfo(GroupInfo *to, const GroupInfo *from)
{
    to->defined = from->defined;
    to->type = from->type;
    darray_init(to->levels);
    darray_copy(to->levels, from->levels);
    for (xkb_level_index_t j = 0; j < darray_size(to->levels); j++) {
        if (darray_item(from->levels, j).num_syms > 1) {
            darray_item(to->levels, j).s.syms =
                memdup(darray_item(from->levels, j).s.syms,
                       darray_item(from->levels, j).num_syms,
                       sizeof(xkb_keysym_t));
        }
        if (darray_item(from->levels, j).num_actions > 1) {
            darray_item(to->levels, j).a.actions =
                memdup(darray_item(from->levels, j).a.actions,
                       darray_item(from->levels, j).num_actions,
                       sizeof(union xkb_action));
        }
    }
}

static void
InitKeyInfo(struct xkb_context *ctx, KeyInfo *keyi)
{
    memset(keyi, 0, sizeof(*keyi));
    keyi->name = xkb_atom_intern_literal(ctx, "*");
    keyi->out_of_range_group_action = RANGE_WRAP;
}

static void
ClearKeyInfo(KeyInfo *keyi)
{
    GroupInfo *groupi;
    darray_foreach(groupi, keyi->groups)
        ClearGroupInfo(groupi);
    darray_free(keyi->groups);
}

/***====================================================================***/

typedef struct {
    enum merge_mode merge;
    bool haveSymbol;
    /* NOTE: Can also be XKB_MOD_NONE, meaning
     *       “don’t add a modifier to the modmap”. */
    xkb_mod_index_t modifier;
    union {
        xkb_atom_t keyName;
        xkb_keysym_t keySym;
    } u;
} ModMapEntry;

typedef struct {
    char *name;         /* e.g. pc+us+inet(evdev) */
    int errorCount;
    unsigned int include_depth;
    xkb_layout_index_t explicit_group;
    darray(KeyInfo) keys;
    KeyInfo default_key;
    ActionsInfo default_actions;
    darray(xkb_atom_t) group_names;
    darray(ModMapEntry) modmaps;
    struct xkb_mod_set mods;

    struct xkb_context *ctx;
    /* Needed for AddKeySymbols. */
    const struct xkb_keymap *keymap;
} SymbolsInfo;

static void
InitSymbolsInfo(SymbolsInfo *info, const struct xkb_keymap *keymap,
                unsigned int include_depth, const struct xkb_mod_set *mods)
{
    memset(info, 0, sizeof(*info));
    info->ctx = keymap->ctx;
    info->include_depth = include_depth;
    info->keymap = keymap;
    InitKeyInfo(keymap->ctx, &info->default_key);
    InitActionsInfo(&info->default_actions);
    InitVMods(&info->mods, mods, include_depth > 0);
    info->explicit_group = XKB_LAYOUT_INVALID;
}

static void
ClearSymbolsInfo(SymbolsInfo *info)
{
    KeyInfo *keyi;
    free(info->name);
    darray_foreach(keyi, info->keys)
        ClearKeyInfo(keyi);
    darray_free(info->keys);
    darray_free(info->group_names);
    darray_free(info->modmaps);
    ClearKeyInfo(&info->default_key);
}

static const char *
KeyInfoText(SymbolsInfo *info, KeyInfo *keyi)
{
    return KeyNameText(info->ctx, keyi->name);
}

static bool
MergeGroups(SymbolsInfo *info, GroupInfo *into, GroupInfo *from, bool clobber,
            bool report, xkb_layout_index_t group, xkb_atom_t key_name)
{
    xkb_level_index_t i, levels_in_both;
    struct xkb_level *level;

    /* First find the type of the merged group. */
    if (into->type != from->type) {
        if (from->type == XKB_ATOM_NONE) {
            /* it's empty for consistency with other comparisons */
        }
        else if (into->type == XKB_ATOM_NONE) {
            into->type = from->type;
        }
        else {
            xkb_atom_t use = (clobber ? from->type : into->type);
            xkb_atom_t ignore = (clobber ? into->type : from->type);

            if (report) {
                log_warn(info->ctx, XKB_WARNING_CONFLICTING_KEY_TYPE_MERGING_GROUPS,
                         "Multiple definitions for group %d type of key %s; "
                         "Using %s, ignoring %s\n",
                         group + 1, KeyNameText(info->ctx, key_name),
                         xkb_atom_text(info->ctx, use),
                         xkb_atom_text(info->ctx, ignore));
            }

            into->type = use;
        }
    }
    into->defined |= (from->defined & GROUP_FIELD_TYPE);

    /* Now look at the levels. */

    if (darray_empty(from->levels)) {
        InitGroupInfo(from);
        return true;
    }

    if (darray_empty(into->levels)) {
        from->type = into->type;
        *into = *from;
        InitGroupInfo(from);
        return true;
    }

    /* Merge the actions and syms. */
    levels_in_both = MIN(darray_size(into->levels), darray_size(from->levels));
    unsigned int fromKeysymsCount = 0;
    unsigned int fromActionsCount = 0;
    for (i = 0; i < levels_in_both; i++) {
        struct xkb_level *intoLevel = &darray_item(into->levels, i);
        struct xkb_level *fromLevel = &darray_item(from->levels, i);

        const bool fromHasNoKeysym = fromLevel->num_syms == 0;
        const bool fromHasNoAction = fromLevel->num_actions == 0;
        if (fromHasNoKeysym && fromHasNoAction) {
            /* Empty `from`: do nothing */
            continue;
        }

        const bool intoHasNoKeysym = intoLevel->num_syms == 0;
        const bool intoHasNoAction = intoLevel->num_actions == 0;
        if (intoHasNoKeysym && intoHasNoAction) {
            /* Empty `into`: use `from` keysyms and actions */
            StealLevelInfo(intoLevel, fromLevel);
            fromKeysymsCount++;
            fromActionsCount++;
            continue;
        }

        /* Possible level conflict */
        assert(intoLevel->num_syms > 0 || intoLevel->num_actions > 0);
        assert(fromLevel->num_syms > 0 || fromLevel->num_actions > 0);

        /* Handle keysyms */
        if (!XkbLevelsSameSyms(fromLevel, intoLevel)) {
            /* Incompatible keysyms */
            if (report && !(intoHasNoKeysym || fromHasNoKeysym)) {
                log_warn(info->ctx, XKB_WARNING_CONFLICTING_KEY_SYMBOL,
                            "Multiple symbols for level %d/group %u on key %s; "
                            "Using %s, ignoring %s\n",
                            i + 1, group + 1, KeyNameText(info->ctx, key_name),
                            (clobber ? "from" : "to"),
                            (clobber ? "to" : "from"));
            }
            if (fromHasNoKeysym) {
                /* No keysym to copy */
            } else if (clobber) {
                /* Override: copy any defined keysym from `from` */
                if (unlikely(fromLevel->num_syms > 1)) {
                    /* Multiple keysyms: always replace, all syms are defined */
                    if (unlikely(intoLevel->num_syms > 1))
                        free(intoLevel->s.syms);
                    /* Steal */
                    intoLevel->s.syms = fromLevel->s.syms;
                    intoLevel->num_syms = fromLevel->num_syms;
                    fromLevel->num_syms = 0;
                    fromKeysymsCount++;
                } else if (fromLevel->s.sym != XKB_KEY_NoSymbol) {
                    /* Single defined keysym */
                    if (unlikely(intoLevel->num_syms > 1))
                        free(intoLevel->s.syms);
                    intoLevel->s.sym = fromLevel->s.sym;
                    intoLevel->num_syms = 1;
                    fromKeysymsCount++;
                }
            } else {
                /* Augment: copy only the keysyms from `from` that are
                 * undefined in `into` */
                if (unlikely(intoLevel->num_syms > 1)) {
                    /* Multiple keysyms: always ignore, all syms are defined */
                } else if (intoLevel->s.sym == XKB_KEY_NoSymbol) {
                    /* Single undefined keysym */
                    if (unlikely(fromLevel->num_syms > 1)) {
                        /* Steal */
                        intoLevel->s.syms = fromLevel->s.syms;
                        intoLevel->num_syms = fromLevel->num_syms;
                        fromLevel->num_syms = 0;
                    } else {
                        intoLevel->s.sym = fromLevel->s.sym;
                        intoLevel->num_syms = 1;
                    }
                    fromKeysymsCount++;
                }
            }
        }

        /* Handle actions */
        if (!XkbLevelsSameActions(intoLevel, fromLevel)) {
            /* Incompatible actions */
            if (report && !(intoHasNoAction || fromHasNoAction)) {
                if (unlikely(intoLevel->num_actions > 1)) {
                    log_warn(
                        info->ctx, XKB_WARNING_CONFLICTING_KEY_ACTION,
                        "Multiple actions for level %d/group %u on key %s; "
                        "%s\n",
                        i + 1, group + 1, KeyNameText(info->ctx, key_name),
                        clobber ? "Using from, ignoring to"
                                : "Using to, ignoring from");
                } else {
                    union xkb_action *use, *ignore;
                    use = clobber
                        ? &fromLevel->a.action
                        : &intoLevel->a.action;
                    ignore = clobber
                        ? &intoLevel->a.action
                        : &fromLevel->a.action;

                    log_warn(info->ctx, XKB_WARNING_CONFLICTING_KEY_ACTION,
                                "Multiple actions for level %d/group %u "
                                "on key %s; Using %s, ignoring %s\n",
                                i + 1, group + 1,
                                KeyNameText(info->ctx, key_name),
                                ActionTypeText(use->type),
                                ActionTypeText(ignore->type));
                }
            }
            if (fromHasNoAction) {
                /* No action to copy */
            } else if (clobber) {
                /* Override: copy any defined action from `from` */
                if (unlikely(fromLevel->num_actions > 1)) {
                    /* Multiple actions: always replace, all syms are defined */
                    if (unlikely(intoLevel->num_actions > 1))
                        free(intoLevel->a.actions);
                    /* Steal */
                    intoLevel->a.actions = fromLevel->a.actions;
                    intoLevel->num_actions = fromLevel->num_actions;
                    fromLevel->num_actions = 0;
                    fromActionsCount++;
                } else if (fromLevel->a.action.type != ACTION_TYPE_NONE) {
                    /* Single defined action */
                    if (unlikely(intoLevel->num_actions > 1))
                        free(intoLevel->a.actions);
                    intoLevel->a.action = fromLevel->a.action;
                    intoLevel->num_actions = 1;
                    fromActionsCount++;
                }
            } else {
                /* Augment: copy only the actions from `from` that are
                 * undefined in `into` */
                if (unlikely(intoLevel->num_actions > 1)) {
                    /* Multiple keysyms: always ignore, all syms are defined */
                } else if (intoLevel->a.action.type == ACTION_TYPE_NONE) {
                    /* Single undefined keysym */
                    if (unlikely(fromLevel->num_actions > 1)) {
                        /* Steal */
                        intoLevel->a.actions = fromLevel->a.actions;
                        intoLevel->num_actions = fromLevel->num_actions;
                        fromLevel->num_actions = 0;
                    } else {
                        intoLevel->a.action = fromLevel->a.action;
                        intoLevel->num_actions = 1;
                    }
                    fromActionsCount++;
                }
            }
        }
    }
    /* If @from has extra levels, get them as well. */
    darray_foreach_from(level, from->levels, levels_in_both) {
        darray_append(into->levels, *level);
        /* We may have stolen keysyms or actions arrays:
         * do not free them when clearing `from` */
        level->num_syms = 0;
        level->num_actions = 0;
        fromKeysymsCount++;
        fromActionsCount++;
    }
    if (fromKeysymsCount) {
        /* Reset defined keysyms field if we used no keysym from `into` */
        if (fromKeysymsCount == darray_size(into->levels))
            into->defined &= ~GROUP_FIELD_SYMS;
        into->defined |= (from->defined & GROUP_FIELD_SYMS);
    }
    if (fromActionsCount) {
        /* Reset defined actions field if we used no action from `into` */
        if (fromActionsCount == darray_size(into->levels))
            into->defined &= ~GROUP_FIELD_ACTS;
        into->defined |= (from->defined & GROUP_FIELD_ACTS);
    }

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

        return clobber;
    }

    return false;
}

static bool
MergeKeys(SymbolsInfo *info, KeyInfo *into, KeyInfo *from, bool same_file)
{
    xkb_layout_index_t i;
    xkb_layout_index_t groups_in_both;
    enum key_field collide = 0;
    const int verbosity = xkb_context_get_log_verbosity(info->ctx);
    const bool clobber = (from->merge != MERGE_AUGMENT);
    const bool report = (same_file && verbosity > 0) || verbosity > 9;

    if (from->merge == MERGE_REPLACE) {
        ClearKeyInfo(into);
        *into = *from;
        InitKeyInfo(info->ctx, from);
        return true;
    }

    groups_in_both = MIN(darray_size(into->groups), darray_size(from->groups));
    for (i = 0; i < groups_in_both; i++)
        MergeGroups(info,
                    &darray_item(into->groups, i),
                    &darray_item(from->groups, i),
                    clobber, report, i, into->name);
    /* If @from has extra groups, just move them to @into. */
    for (i = groups_in_both; i < darray_size(from->groups); i++) {
        darray_append(into->groups, darray_item(from->groups, i));
        InitGroupInfo(&darray_item(from->groups, i));
    }

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
    if (UseNewKeyField(KEY_FIELD_DEFAULT_TYPE, into->defined, from->defined,
                       clobber, report, &collide)) {
        into->default_type = from->default_type;
        into->defined |= KEY_FIELD_DEFAULT_TYPE;
    }
    if (UseNewKeyField(KEY_FIELD_GROUPINFO, into->defined, from->defined,
                       clobber, report, &collide)) {
        into->out_of_range_group_action = from->out_of_range_group_action;
        into->out_of_range_group_number = from->out_of_range_group_number;
        into->defined |= KEY_FIELD_GROUPINFO;
    }

    if (collide) {
        log_warn(info->ctx, XKB_WARNING_CONFLICTING_KEY_FIELDS,
                 "Symbol map for key %s redefined; "
                 "Using %s definition for conflicting fields\n",
                 KeyNameText(info->ctx, into->name),
                 (clobber ? "first" : "last"));
    }

    ClearKeyInfo(from);
    InitKeyInfo(info->ctx, from);
    return true;
}

/* TODO: Make it so this function doesn't need the entire keymap. */
static bool
AddKeySymbols(SymbolsInfo *info, KeyInfo *keyi, bool same_file)
{
    xkb_atom_t real_name;
    KeyInfo *iter;

    /*
     * Don't keep aliases in the keys array; this guarantees that
     * searching for keys to merge with by straight comparison (see the
     * following loop) is enough, and we won't get multiple KeyInfo's
     * for the same key because of aliases.
     */
    real_name = XkbResolveKeyAlias(info->keymap, keyi->name);
    if (real_name != XKB_ATOM_NONE)
        keyi->name = real_name;

    darray_foreach(iter, info->keys)
        if (iter->name == keyi->name)
            return MergeKeys(info, iter, keyi, same_file);

    darray_append(info->keys, *keyi);
    InitKeyInfo(info->ctx, keyi);
    return true;
}

static bool
AddModMapEntry(SymbolsInfo *info, ModMapEntry *new)
{
    ModMapEntry *old;
    bool clobber = (new->merge != MERGE_AUGMENT);

    darray_foreach(old, info->modmaps) {
        xkb_mod_index_t use, ignore;

        if ((new->haveSymbol != old->haveSymbol) ||
            (new->haveSymbol && new->u.keySym != old->u.keySym) ||
            (!new->haveSymbol && new->u.keyName != old->u.keyName))
            continue;

        if (new->modifier == old->modifier)
            return true;

        use = (clobber ? new->modifier : old->modifier);
        ignore = (clobber ? old->modifier : new->modifier);

        if (new->haveSymbol) {
            log_warn(info->ctx, XKB_WARNING_CONFLICTING_MODMAP,
                     "Symbol \"%s\" added to modifier map for multiple modifiers; "
                     "Using %s, ignoring %s\n",
                     KeysymText(info->ctx, new->u.keySym),
                     ModIndexText(info->ctx, &info->mods, use),
                     ModIndexText(info->ctx, &info->mods, ignore));
        } else {
            log_warn(info->ctx, XKB_WARNING_CONFLICTING_MODMAP,
                     "Key \"%s\" added to modifier map for multiple modifiers; "
                     "Using %s, ignoring %s\n",
                     KeyNameText(info->ctx, new->u.keyName),
                     ModIndexText(info->ctx, &info->mods, use),
                     ModIndexText(info->ctx, &info->mods, ignore));
        }
        old->modifier = use;
        return true;
    }

    darray_append(info->modmaps, *new);
    return true;
}

/***====================================================================***/

static void
MergeIncludedSymbols(SymbolsInfo *into, SymbolsInfo *from,
                     enum merge_mode merge)
{
    xkb_atom_t *group_name;
    xkb_layout_index_t group_names_in_both;

    if (from->errorCount > 0) {
        into->errorCount += from->errorCount;
        return;
    }

    MergeModSets(into->ctx, &into->mods, &from->mods, merge);

    if (into->name == NULL) {
        into->name = steal(&from->name);
    }

    group_names_in_both = MIN(darray_size(into->group_names),
                              darray_size(from->group_names));
    for (xkb_layout_index_t i = 0; i < group_names_in_both; i++) {
        if (!darray_item(from->group_names, i))
            continue;

        if (merge == MERGE_AUGMENT && darray_item(into->group_names, i))
            continue;

        darray_item(into->group_names, i) = darray_item(from->group_names, i);
    }
    /* If @from has more, get them as well. */
    darray_foreach_from(group_name, from->group_names, group_names_in_both)
        darray_append(into->group_names, *group_name);

    if (darray_empty(into->keys)) {
        into->keys = from->keys;
        darray_init(from->keys);
    }
    else {
        KeyInfo *keyi;
        darray_foreach(keyi, from->keys) {
            keyi->merge = merge;
            if (!AddKeySymbols(into, keyi, false))
                into->errorCount++;
        }
    }

    if (darray_empty(into->modmaps)) {
        into->modmaps = from->modmaps;
        darray_init(from->modmaps);
    }
    else {
        ModMapEntry *mm;
        darray_foreach(mm, from->modmaps) {
            mm->merge = merge;
            if (!AddModMapEntry(into, mm))
                into->errorCount++;
        }
    }
}

static void
HandleSymbolsFile(SymbolsInfo *info, XkbFile *file);

static bool
HandleIncludeSymbols(SymbolsInfo *info, IncludeStmt *include)
{
    SymbolsInfo included;

    if (ExceedsIncludeMaxDepth(info->ctx, info->include_depth)) {
        info->errorCount += 10;
        return false;
    }

    InitSymbolsInfo(&included, info->keymap, info->include_depth + 1,
                    &info->mods);
    included.name = steal(&include->stmt);

    for (IncludeStmt *stmt = include; stmt; stmt = stmt->next_incl) {
        SymbolsInfo next_incl;
        XkbFile *file;

        file = ProcessIncludeFile(info->ctx, stmt, FILE_TYPE_SYMBOLS);
        if (!file) {
            info->errorCount += 10;
            ClearSymbolsInfo(&included);
            return false;
        }

        InitSymbolsInfo(&next_incl, info->keymap, info->include_depth + 1,
                        &included.mods);
        if (stmt->modifier) {
            next_incl.explicit_group = atoi(stmt->modifier) - 1;
            if (next_incl.explicit_group >= XKB_MAX_GROUPS) {
                log_err(info->ctx, XKB_ERROR_UNSUPPORTED_GROUP_INDEX,
                        "Cannot set explicit group to %d - must be between 1..%d; "
                        "Ignoring group number\n",
                        next_incl.explicit_group + 1, XKB_MAX_GROUPS);
                next_incl.explicit_group = info->explicit_group;
            }
        }
        else if (info->keymap->num_groups != 0 && next_incl.include_depth == 1) {
            /* If keymap is the result of RMLVO resolution and we are at the
             * first include depth, transform e.g. `pc` into `pc:1` in order to
             * force only one group per key using the explicit group.
             *
             * NOTE: X11’s xkbcomp does not apply this rule. */
            next_incl.explicit_group = 0;
        }
        else {
            /* The keymap was not generated from rules or this is not the first
             * level of include: take the parent’s explicit group. */
            next_incl.explicit_group = info->explicit_group;
        }

        HandleSymbolsFile(&next_incl, file);

        MergeIncludedSymbols(&included, &next_incl, stmt->merge);

        ClearSymbolsInfo(&next_incl);
        FreeXkbFile(file);
    }

    MergeIncludedSymbols(info, &included, include->merge);
    ClearSymbolsInfo(&included);

    return (info->errorCount == 0);
}

static bool
GetGroupIndex(SymbolsInfo *info, KeyInfo *keyi, ExprDef *arrayNdx,
              enum group_field field, xkb_layout_index_t *ndx_rtrn)
{
    assert (field == GROUP_FIELD_SYMS || field == GROUP_FIELD_ACTS);
    const char *name = (field == GROUP_FIELD_SYMS ? "symbols" : "actions");

    if (arrayNdx == NULL) {
        xkb_layout_index_t i = 0;
        GroupInfo *groupi;

        darray_enumerate(i, groupi, keyi->groups) {
            if (!(groupi->defined & field)) {
                *ndx_rtrn = i;
                return true;
            }
        }

        if (i >= XKB_MAX_GROUPS) {
            log_err(info->ctx, XKB_ERROR_UNSUPPORTED_GROUP_INDEX,
                    "Too many groups of %s for key %s (max %u); "
                    "Ignoring %s defined for extra groups\n",
                    name, KeyInfoText(info, keyi), XKB_MAX_GROUPS, name);
            return false;
        }

        darray_resize0(keyi->groups, darray_size(keyi->groups) + 1);
        *ndx_rtrn = darray_size(keyi->groups) - 1;
        return true;
    }

    if (!ExprResolveGroup(info->ctx, arrayNdx, ndx_rtrn)) {
        log_err(info->ctx, XKB_ERROR_UNSUPPORTED_GROUP_INDEX,
                "Illegal group index for %s of key %s\n"
                "Definition with non-integer array index ignored\n",
                name, KeyInfoText(info, keyi));
        return false;
    }

    (*ndx_rtrn)--;
    if (*ndx_rtrn >= darray_size(keyi->groups))
        darray_resize0(keyi->groups, *ndx_rtrn + 1);

    return true;
}

static bool
AddSymbolsToKey(SymbolsInfo *info, KeyInfo *keyi, ExprDef *arrayNdx,
                ExprDef *value)
{
    xkb_layout_index_t ndx = 0;

    if (!GetGroupIndex(info, keyi, arrayNdx, GROUP_FIELD_SYMS, &ndx))
        return false;

    GroupInfo *groupi = &darray_item(keyi->groups, ndx);

    if (value->common.type == STMT_EXPR_EMPTY_LIST) {
        groupi->defined |= GROUP_FIELD_SYMS;
        return true;
    }

    if (value->common.type != STMT_EXPR_KEYSYM_LIST) {
        log_err(info->ctx, XKB_ERROR_WRONG_FIELD_TYPE,
                "Expected a list of symbols, found %s; "
                "Ignoring symbols for group %u of %s\n",
                stmt_type_to_string(value->common.type), ndx + 1,
                KeyInfoText(info, keyi));
        return false;
    }

    if (groupi->defined & GROUP_FIELD_SYMS) {
        log_err(info->ctx, XKB_ERROR_CONFLICTING_KEY_SYMBOLS_ENTRY,
                "Symbols for key %s, group %u already defined; "
                "Ignoring duplicate definition\n",
                KeyInfoText(info, keyi), ndx + 1);
        return false;
    }

    xkb_level_index_t nLevels = 0;
    xkb_level_index_t nonEmptyLevels = 0;
    /* Contrary to actions, keysyms are already parsed at this point so we drop
     * trailing symbols by not adding them in the first place. */
    for (ExprKeysymList *keysymList = (ExprKeysymList *) value;
         keysymList;
         keysymList = (ExprKeysymList *) keysymList->common.next) {
        nLevels++;
        /* Drop trailing NoSymbol */
        if (darray_size(keysymList->syms) > 0)
            nonEmptyLevels = nLevels;
    }
    if (nonEmptyLevels < nLevels)
        nLevels = nonEmptyLevels;
    if (darray_size(groupi->levels) < nLevels)
        darray_resize0(groupi->levels, nLevels);

    groupi->defined |= GROUP_FIELD_SYMS;

    xkb_level_index_t level = 0;
    for (ExprKeysymList *keysymList = (ExprKeysymList *) value;
         keysymList && level < nLevels;
         keysymList = (ExprKeysymList *) keysymList->common.next, level++) {
        struct xkb_level *leveli = &darray_item(groupi->levels, level);
        assert(leveli->num_syms == 0);

        if (unlikely(darray_size(keysymList->syms) > MAX_KEYSYMS_PER_LEVEL)) {
            log_err(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                    "Key %s has too many keysyms for group %u, level %u; "
                    "expected max %u, got: %u\n",
                    KeyInfoText(info, keyi), ndx + 1, level + 1,
                    MAX_KEYSYMS_PER_LEVEL, darray_size(keysymList->syms));
            return false;
        }

        leveli->num_syms = (xkb_keysym_count_t) darray_size(keysymList->syms);
        switch (leveli->num_syms) {
        case 0:
            leveli->s.sym = XKB_KEY_NoSymbol;
            break;
        case 1:
            leveli->s.sym = darray_item(keysymList->syms, 0);
            assert(leveli->s.sym != XKB_KEY_NoSymbol);
            break;
        default:
            darray_shrink(keysymList->syms);
            darray_steal(keysymList->syms, &leveli->s.syms, NULL);
#ifndef NDEBUG
            /* Canonical list: all NoSymbol were dropped */
            for (xkb_keysym_count_t k = 0; k < leveli->num_syms; k++)
                assert(leveli->s.syms[k] != XKB_KEY_NoSymbol);
#endif
        }
    }

    return true;
}

static bool
AddActionsToKey(SymbolsInfo *info, KeyInfo *keyi, ExprDef *arrayNdx,
                ExprDef *value)
{
    xkb_layout_index_t ndx = 0;

    if (!GetGroupIndex(info, keyi, arrayNdx, GROUP_FIELD_ACTS, &ndx))
        return false;

    GroupInfo *groupi = &darray_item(keyi->groups, ndx);

    if (value->common.type == STMT_EXPR_EMPTY_LIST) {
        groupi->defined |= GROUP_FIELD_ACTS;
        return true;
    }

    if (value->common.type != STMT_EXPR_ACTION_LIST) {
        log_wsgo(info->ctx, XKB_ERROR_INVALID_EXPRESSION_TYPE,
                 "Bad expression type (%d) for action list value; "
                 "Ignoring actions for group %u of %s\n",
                 value->common.type, ndx, KeyInfoText(info, keyi));
        return false;
    }

    if (groupi->defined & GROUP_FIELD_ACTS) {
        log_wsgo(info->ctx, XKB_WARNING_CONFLICTING_KEY_ACTION,
                 "Actions for key %s, group %u already defined\n",
                 KeyInfoText(info, keyi), ndx);
        return false;
    }

    xkb_level_index_t nLevels = 0;
    /* Contrary to keysyms with trailing `NoSymbol`, we cannot detect trailing
     * `NoAction()` now, because we need to parse the actions first. Jusr count
     * explicit actions for now. */
    for (ParseCommon *p = &value->common; p; p = p->next)
        nLevels++;
    if (darray_size(groupi->levels) < nLevels)
        darray_resize0(groupi->levels, nLevels);

    groupi->defined |= GROUP_FIELD_ACTS;

    xkb_level_index_t level = 0;
    xkb_level_index_t nonEmptyLevels = 0;
    for (ExprActionList *actionList = (ExprActionList *) value;
         actionList;
         actionList = (ExprActionList *) actionList->common.next, level++) {
        struct xkb_level *leveli = &darray_item(groupi->levels, level);
        assert(leveli->num_actions == 0);

        unsigned int num_actions = 0;
        for (ExprDef *act = actionList->actions;
             act; act = (ExprDef *) act->common.next)
            num_actions++;

        if (unlikely(num_actions > MAX_ACTIONS_PER_LEVEL)) {
            log_err(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                    "Key %s has too many actions for group %u, level %u; "
                    "expected max %u, got: %u\n",
                    KeyInfoText(info, keyi), ndx + 1, level + 1,
                    MAX_ACTIONS_PER_LEVEL, num_actions);
            return false;
        }

        /* Parse actions and add only defined actions */
        darray(union xkb_action) actions = darray_new();
        for (ExprDef *act = actionList->actions;
             act; act = (ExprDef *) act->common.next) {
            union xkb_action toAct = { 0 };
            if (!HandleActionDef(info->ctx, &info->default_actions, &info->mods,
                                 act, &toAct)) {
                log_err(info->ctx, XKB_ERROR_INVALID_VALUE,
                        "Illegal action definition for %s; "
                        "Action for group %u/level %u ignored\n",
                        KeyInfoText(info, keyi), ndx + 1, level + 1);
                /* Ensure action type is reset */
                toAct.type = ACTION_TYPE_NONE;
            }
            if (toAct.type == ACTION_TYPE_NONE) {
                /* Drop action */
            } else if (likely(num_actions == 1)) {
                /* Only one action: do not allocate */
                leveli->num_actions = 1;
                leveli->a.action = toAct;
                goto next_level;
            } else {
                darray_append(actions, toAct);
            }
        }
        if (darray_empty(actions)) {
            leveli->num_actions = 0;
        } else if (likely(darray_size(actions) > 1)) {
            leveli->num_actions = (xkb_action_count_t) darray_size(actions);
            darray_shrink(actions);
            darray_steal(actions, &leveli->a.actions, NULL);
#ifndef NDEBUG
            /* Canonical list: all NoAction() were dropped */
            for (xkb_action_count_t k = 0; k < leveli->num_actions; k++)
                assert(leveli->a.actions[k].type != ACTION_TYPE_NONE);
#endif
        } else {
            /* Unlikely: some actions were dropped and only one remains */
            assert(num_actions > darray_size(actions));
            leveli->num_actions = 1;
            leveli->a.action = darray_item(actions, 0);
            assert(leveli->a.action.type != ACTION_TYPE_NONE);
            darray_free(actions);
        }
next_level:
        /*
         * Check trailing `NoAction()`, but count as empty level only if there
         * is no corresponding keysym.
         */
        if (leveli->num_actions > 0 || leveli->num_syms > 0)
            nonEmptyLevels = level + 1;
    }

    if (nonEmptyLevels < nLevels) {
        /* Drop trailing `NoAction()`.
         * No need to clear dropped levels: there are no keysyms nor actions */
        darray_size(groupi->levels) = nonEmptyLevels;
        if (nonEmptyLevels > 0)
            darray_shrink(groupi->levels);
        else
            darray_free(groupi->levels);
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
    if (istreq(field, "type")) {
        xkb_layout_index_t ndx = 0;
        xkb_atom_t val = XKB_ATOM_NONE;

        if (!ExprResolveString(info->ctx, value, &val)) {
            log_err(info->ctx, XKB_ERROR_WRONG_FIELD_TYPE,
                    "The type field of a key symbol map must be a string; "
                    "Ignoring illegal type definition\n");
            return false;
        }

        if (!arrayNdx) {
            keyi->default_type = val;
            keyi->defined |= KEY_FIELD_DEFAULT_TYPE;
        }
        else if (!ExprResolveGroup(info->ctx, arrayNdx, &ndx)) {
            log_err(info->ctx, XKB_ERROR_UNSUPPORTED_GROUP_INDEX,
                    "Illegal group index for type of key %s; "
                    "Definition with non-integer array index ignored\n",
                    KeyInfoText(info, keyi));
            return false;
        }
        else {
            ndx--;
            if (ndx >= darray_size(keyi->groups)) {
                /* Avoid clang-tidy false positive */
                assert(darray_size(keyi->groups) < ndx + 1);
                darray_resize0(keyi->groups, ndx + 1);
            }
            darray_item(keyi->groups, ndx).type = val;
            darray_item(keyi->groups, ndx).defined |= GROUP_FIELD_TYPE;
        }
    }
    else if (istreq(field, "symbols")) {
        return AddSymbolsToKey(info, keyi, arrayNdx, value);
    }
    else if (istreq(field, "actions")) {
        return AddActionsToKey(info, keyi, arrayNdx, value);
    }
    else if (istreq(field, "vmods") ||
             istreq(field, "virtualmods") ||
             istreq(field, "virtualmodifiers")) {
        xkb_mod_mask_t mask = 0;

        if (!ExprResolveModMask(info->ctx, value, MOD_VIRT, &info->mods,
                                &mask)) {
            log_err(info->ctx, XKB_ERROR_UNSUPPORTED_MODIFIER_MASK,
                    "Expected a virtual modifier mask, found %s; "
                    "Ignoring virtual modifiers definition for key %s\n",
                    stmt_type_to_string(value->common.type),
                    KeyInfoText(info, keyi));
            return false;
        }

        keyi->vmodmap = mask;
        keyi->defined |= KEY_FIELD_VMODMAP;
    }
    else if (istreq(field, "locking") ||
             istreq(field, "lock") ||
             istreq(field, "locks")) {
        log_vrb(info->ctx, 1, XKB_WARNING_UNSUPPORTED_SYMBOLS_FIELD,
                "Key behaviors not supported; "
                "Ignoring locking specification for key %s\n",
                KeyInfoText(info, keyi));
    }
    else if (istreq(field, "radiogroup") ||
             istreq(field, "permanentradiogroup") ||
             istreq(field, "allownone")) {
        log_vrb(info->ctx, 1, XKB_WARNING_UNSUPPORTED_SYMBOLS_FIELD,
                "Radio groups not supported; "
                "Ignoring radio group specification for key %s\n",
                KeyInfoText(info, keyi));
    }
    else if (istreq_prefix("overlay", field) ||
             istreq_prefix("permanentoverlay", field)) {
        log_vrb(info->ctx, 1, XKB_WARNING_UNSUPPORTED_SYMBOLS_FIELD,
                "Overlays not supported; "
                "Ignoring overlay specification for key %s\n",
                KeyInfoText(info, keyi));
    }
    else if (istreq(field, "repeating") ||
             istreq(field, "repeats") ||
             istreq(field, "repeat")) {
        uint32_t val = 0;

        if (!ExprResolveEnum(info->ctx, value, &val, repeatEntries)) {
            log_err(info->ctx, XKB_ERROR_INVALID_VALUE,
                    "Illegal repeat setting for %s; "
                    "Non-boolean repeat setting ignored\n",
                    KeyInfoText(info, keyi));
            return false;
        }

        keyi->repeat = (enum key_repeat) val;
        keyi->defined |= KEY_FIELD_REPEAT;
    }
    else if (istreq(field, "groupswrap") ||
             istreq(field, "wrapgroups")) {
        bool set = false;

        if (!ExprResolveBoolean(info->ctx, value, &set)) {
            log_err(info->ctx, XKB_ERROR_INVALID_VALUE,
                    "Illegal groupsWrap setting for %s; "
                    "Non-boolean value ignored\n",
                    KeyInfoText(info, keyi));
            return false;
        }

        keyi->out_of_range_group_action = (set ? RANGE_WRAP : RANGE_SATURATE);
        keyi->defined |= KEY_FIELD_GROUPINFO;
    }
    else if (istreq(field, "groupsclamp") ||
             istreq(field, "clampgroups")) {
        bool set = false;

        if (!ExprResolveBoolean(info->ctx, value, &set)) {
            log_err(info->ctx, XKB_ERROR_INVALID_VALUE,
                    "Illegal groupsClamp setting for %s; "
                    "Non-boolean value ignored\n",
                    KeyInfoText(info, keyi));
            return false;
        }

        keyi->out_of_range_group_action = (set ? RANGE_SATURATE : RANGE_WRAP);
        keyi->defined |= KEY_FIELD_GROUPINFO;
    }
    else if (istreq(field, "groupsredirect") ||
             istreq(field, "redirectgroups")) {
        xkb_layout_index_t grp = 0;

        if (!ExprResolveGroup(info->ctx, value, &grp)) {
            log_err(info->ctx, XKB_ERROR_UNSUPPORTED_GROUP_INDEX,
                    "Illegal group index for redirect of key %s; "
                    "Definition with non-integer group ignored\n",
                    KeyInfoText(info, keyi));
            return false;
        }

        keyi->out_of_range_group_action = RANGE_REDIRECT;
        keyi->out_of_range_group_number = grp - 1;
        keyi->defined |= KEY_FIELD_GROUPINFO;
    }
    else {
        log_err(info->ctx, XKB_ERROR_UNKNOWN_FIELD,
                "Unknown field \"%s\" in a symbol interpretation; "
                "Definition ignored\n",
                field);
        return false;
    }

    return true;
}

static bool
SetGroupName(SymbolsInfo *info, ExprDef *arrayNdx, ExprDef *value,
             enum merge_mode merge)
{
    if (!arrayNdx) {
        log_vrb(info->ctx, 1, XKB_WARNING_MISSING_SYMBOLS_GROUP_NAME_INDEX,
                "You must specify an index when specifying a group name; "
                "Group name definition without array subscript ignored\n");
        return false;
    }

    xkb_layout_index_t group = 0;
    if (!ExprResolveGroup(info->ctx, arrayNdx, &group)) {
        log_err(info->ctx, XKB_ERROR_UNSUPPORTED_GROUP_INDEX,
                "Illegal index in group name definition; "
                "Definition with non-integer array index ignored\n");
        return false;
    }

    xkb_atom_t name = XKB_ATOM_NONE;
    if (!ExprResolveString(info->ctx, value, &name)) {
        log_err(info->ctx, XKB_ERROR_WRONG_FIELD_TYPE,
                "Group name must be a string; "
                "Illegal name for group %d ignored\n", group);
        return false;
    }

    xkb_layout_index_t group_to_use;
    if (info->explicit_group == XKB_LAYOUT_INVALID) {
        group_to_use = group - 1;
    }
    else if (group - 1 == 0) {
        group_to_use = info->explicit_group;
    }
    else {
        log_warn(info->ctx, XKB_WARNING_NON_BASE_GROUP_NAME,
                 "An explicit group was specified for the '%s' map, "
                 "but it provides a name for a group other than Group1 (%d); "
                 "Ignoring group name '%s'\n",
                 info->name, group,
                 xkb_atom_text(info->ctx, name));
        return false;
    }

    if (group_to_use >= darray_size(info->group_names)) {
        /* Avoid clang-tidy false positive */
        assert(darray_size(info->group_names) < group_to_use + 1);
        darray_resize0(info->group_names, group_to_use + 1);
    } else {
        const xkb_atom_t old_name = darray_item(info->group_names, group_to_use);
        if (old_name != XKB_ATOM_NONE && old_name != name) {
            const bool replace = (merge != MERGE_AUGMENT);
            const xkb_atom_t use    = (replace ? name : old_name);
            const xkb_atom_t ignore = (replace ? old_name : name);
            log_warn(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                     "Multiple definitions of group %"PRIu32" name "
                     "in map '%s'; Using '%s', ignoring '%s'\n",
                     group_to_use, info->name,
                     xkb_atom_text(info->ctx, use),
                     xkb_atom_text(info->ctx, ignore));
            name = use;
        }
    }
    darray_item(info->group_names, group_to_use) = name;

    return true;
}

static bool
HandleGlobalVar(SymbolsInfo *info, VarDef *stmt)
{
    const char *elem, *field;
    ExprDef *arrayNdx;
    bool ret;

    if (!ExprResolveLhs(info->ctx, stmt->name, &elem, &field, &arrayNdx))
        return false;

    if (elem && istreq(elem, "key")) {
        KeyInfo temp = {0};
        InitKeyInfo(info->ctx, &temp);
        /* Do not replace the whole key, only the current field */
        temp.merge = (temp.merge == MERGE_REPLACE)
            ? MERGE_OVERRIDE
            : stmt->merge;
        ret = SetSymbolsField(info, &temp, field, arrayNdx, stmt->value);
        MergeKeys(info, &info->default_key, &temp, true);
    }
    else if (!elem && (istreq(field, "name") ||
                       istreq(field, "groupname"))) {
        ret = SetGroupName(info, arrayNdx, stmt->value, stmt->merge);
    }
    else if (!elem && (istreq(field, "groupswrap") ||
                       istreq(field, "wrapgroups"))) {
        log_err(info->ctx, XKB_WARNING_UNSUPPORTED_SYMBOLS_FIELD,
                "Global \"groupswrap\" not supported; Ignored\n");
        ret = true;
    }
    else if (!elem && (istreq(field, "groupsclamp") ||
                       istreq(field, "clampgroups"))) {
        log_err(info->ctx, XKB_WARNING_UNSUPPORTED_SYMBOLS_FIELD,
                "Global \"groupsclamp\" not supported; Ignored\n");
        ret = true;
    }
    else if (!elem && (istreq(field, "groupsredirect") ||
                       istreq(field, "redirectgroups"))) {
        log_err(info->ctx, XKB_WARNING_UNSUPPORTED_SYMBOLS_FIELD,
                "Global \"groupsredirect\" not supported; Ignored\n");
        ret = true;
    }
    else if (!elem && istreq(field, "allownone")) {
        log_err(info->ctx, XKB_WARNING_UNSUPPORTED_SYMBOLS_FIELD,
                "Radio groups not supported; "
                "Ignoring \"allownone\" specification\n");
        ret = true;
    }
    else {
        ret = SetDefaultActionField(info->ctx, &info->default_actions,
                                    &info->mods, elem, field, arrayNdx,
                                    stmt->value, stmt->merge);
    }

    return ret;
}

static bool
HandleSymbolsBody(SymbolsInfo *info, VarDef *def, KeyInfo *keyi)
{
    if (!def)
        return true; /* Empty body */

    bool all_valid_entries = true;

    for (; def; def = (VarDef *) def->common.next) {
        const char *field;
        ExprDef *arrayNdx;
        bool ok = true;

        if (!def->name) {
            if (unlikely(!def->value) || def->value->common.type != STMT_EXPR_ACTION_LIST)
                field = "symbols"; /* Default to symbols field */
            else
                field = "actions";
            arrayNdx = NULL;
        }
        else {
            const char *elem;
            ok = ExprResolveLhs(info->ctx, def->name, &elem, &field,
                                &arrayNdx);
            if (ok && elem) {
                log_err(info->ctx, XKB_ERROR_GLOBAL_DEFAULTS_WRONG_SCOPE,
                        "Cannot set global defaults for \"%s\" element within "
                        "a key statement: move statements to the global file "
                        "scope. Assignment to \"%s.%s\" ignored.\n",
                        elem, elem, field);
                ok = false;
            }
        }

        if (unlikely(!def->value)) {
            log_err(info->ctx, XKB_ERROR_ALLOCATION_ERROR,
                    "Could not allocate the value of field \"%s\". "
                    "Statement ignored.\n", field);
            ok = false;
        }

        if (!ok || !SetSymbolsField(info, keyi, field, arrayNdx, def->value))
            all_valid_entries = false;
    }

    return all_valid_entries;
}

static bool
SetExplicitGroup(SymbolsInfo *info, KeyInfo *keyi)
{
    xkb_layout_index_t i;
    GroupInfo *groupi;
    bool warn = false;

    if (info->explicit_group == XKB_LAYOUT_INVALID)
        return true;

    darray_enumerate_from(i, groupi, keyi->groups, 1) {
        if (groupi->defined) {
            warn = true;
            ClearGroupInfo(groupi);
            InitGroupInfo(groupi);
        }
    }

    if (warn) {
        log_warn(info->ctx, XKB_WARNING_MULTIPLE_GROUPS_AT_ONCE,
                 "For the map %s the explicit group %u is specified, "
                 "but key %s has more than one group defined; "
                 "All groups except first one will be ignored\n",
                 info->name, info->explicit_group + 1, KeyInfoText(info, keyi));
    }

    darray_resize0(keyi->groups, info->explicit_group + 1);
    if (info->explicit_group > 0) {
        darray_item(keyi->groups, info->explicit_group) =
            darray_item(keyi->groups, 0);
        InitGroupInfo(&darray_item(keyi->groups, 0));
    }

    return true;
}

static bool
HandleSymbolsDef(SymbolsInfo *info, SymbolsDef *stmt)
{
    KeyInfo keyi;

    keyi = info->default_key;
    darray_init(keyi.groups);
    darray_copy(keyi.groups, info->default_key.groups);
    for (xkb_layout_index_t i = 0; i < darray_size(keyi.groups); i++)
        CopyGroupInfo(&darray_item(keyi.groups, i),
                      &darray_item(info->default_key.groups, i));
    keyi.merge = stmt->merge;
    keyi.name = stmt->keyName;

    if (HandleSymbolsBody(info, stmt->symbols, &keyi) &&
        SetExplicitGroup(info, &keyi) &&
        AddKeySymbols(info, &keyi, true)) {
        return true;
    }

    ClearKeyInfo(&keyi);
    info->errorCount++;
    return false;
}

static bool
HandleModMapDef(SymbolsInfo *info, ModMapDef *def)
{
    ModMapEntry tmp;
    xkb_mod_index_t ndx;
    bool ok;
    struct xkb_context *ctx = info->ctx;
    const char *modifier_name = xkb_atom_text(ctx, def->modifier);

    if (istreq(modifier_name, "none")) {
        /* Handle special "None" entry */
        ndx = XKB_MOD_NONE;
    } else {
        /* Handle normal entry */
        ndx = XkbModNameToIndex(&info->mods, def->modifier, MOD_REAL);
        if (ndx == XKB_MOD_INVALID) {
            log_err(info->ctx, XKB_ERROR_INVALID_REAL_MODIFIER,
                    "Illegal modifier map definition; "
                    "Ignoring map for non-modifier \"%s\"\n",
                    xkb_atom_text(ctx, def->modifier));
            return false;
        }
    }

    ok = true;
    tmp.modifier = ndx;
    tmp.merge = def->merge;

    for (ExprDef *key = def->keys; key; key = (ExprDef *) key->common.next) {
        if (key->common.type == STMT_EXPR_KEYNAME_LITERAL) {
            tmp.haveSymbol = false;
            tmp.u.keyName = key->key_name.key_name;
        }
        else if (key->common.type == STMT_EXPR_KEYSYM_LITERAL) {
            if (key->keysym.keysym == XKB_KEY_NoSymbol) {
                /* Invalid keysym: ignore. Error message already printed */
                continue;
            } else {
                tmp.haveSymbol = true;
                tmp.u.keySym = key->keysym.keysym;
            }
        }
        else {
            log_err(info->ctx, XKB_ERROR_INVALID_MODMAP_ENTRY,
                    "Modmap entries may contain only key names or keysyms; "
                    "Illegal definition for %s modifier ignored\n",
                    ModIndexText(info->ctx, &info->mods, tmp.modifier));
            continue;
        }

        ok = AddModMapEntry(info, &tmp) && ok;
    }
    return ok;
}

static void
HandleSymbolsFile(SymbolsInfo *info, XkbFile *file)
{
    bool ok;

    free(info->name);
    info->name = strdup_safe(file->name);

    for (ParseCommon *stmt = file->defs; stmt; stmt = stmt->next) {
        switch (stmt->type) {
        case STMT_INCLUDE:
            ok = HandleIncludeSymbols(info, (IncludeStmt *) stmt);
            break;
        case STMT_SYMBOLS:
            ok = HandleSymbolsDef(info, (SymbolsDef *) stmt);
            break;
        case STMT_VAR:
            ok = HandleGlobalVar(info, (VarDef *) stmt);
            break;
        case STMT_VMOD:
            ok = HandleVModDef(info->ctx, &info->mods, (VModDef *) stmt);
            break;
        case STMT_MODMAP:
            ok = HandleModMapDef(info, (ModMapDef *) stmt);
            break;
        default:
            log_err(info->ctx, XKB_ERROR_WRONG_STATEMENT_TYPE,
                    "Symbols files may not include other types; "
                    "Ignoring %s\n", stmt_type_to_string(stmt->type));
            ok = false;
            break;
        }

        if (!ok)
            info->errorCount++;

        if (info->errorCount > 10) {
            log_err(info->ctx, XKB_ERROR_INVALID_XKB_SYNTAX,
                    "Abandoning symbols file \"%s\"\n",
                    safe_map_name(file));
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
    struct xkb_key *key;
    xkb_layout_index_t group;
    bool got_one_group, got_one_level;

    group = 0;
    do {
        xkb_level_index_t level = 0;
        got_one_group = false;
        do {
            got_one_level = false;
            xkb_keys_foreach(key, keymap) {
                if (group < key->num_groups &&
                    level < XkbKeyNumLevels(key, group)) {
                    got_one_group = got_one_level = true;
                    unsigned int num_syms = key->groups[group].levels[level].num_syms;
                    if (num_syms > 1) {
                        for (unsigned int k = 0; k < num_syms; k++) {
                            if (key->groups[group].levels[level].s.syms[k] == sym)
                                return key;
                        }
                    } else if (num_syms &&
                               key->groups[group].levels[level].s.sym == sym) {
                        return key;
                    }
                }
            }
            level++;
        } while (got_one_level);
        group++;
    } while (got_one_group);

    return NULL;
}

/*
 * Find an appropriate type for a group and return its name.
 *
 * Simple recipe:
 * - ONE_LEVEL for width 0/1
 * - ALPHABETIC for 2 shift levels, with lower/upercase keysyms
 * - KEYPAD for keypad keys.
 * - TWO_LEVEL for other 2 shift level keys.
 * and the same for four level keys.
 *
 * FIXME: Decide how to handle multiple-syms-per-level, and do it.
 */
static xkb_atom_t
FindAutomaticType(struct xkb_context *ctx, GroupInfo *groupi)
{
    xkb_keysym_t sym0, sym1;
    const xkb_level_index_t width = darray_size(groupi->levels);

#define GET_SYM(level) \
    (darray_item(groupi->levels, level).num_syms == 0 ? \
        XKB_KEY_NoSymbol : \
     darray_item(groupi->levels, level).num_syms == 1 ? \
        darray_item(groupi->levels, level).s.sym : \
     /* num_syms > 1 */ \
        darray_item(groupi->levels, level).s.syms[0])

    if (width == 1 || width <= 0)
        return xkb_atom_intern_literal(ctx, "ONE_LEVEL");

    sym0 = GET_SYM(0);
    sym1 = GET_SYM(1);

    if (width == 2) {
        if (xkb_keysym_is_lower(sym0) && xkb_keysym_is_upper_or_title(sym1))
            return xkb_atom_intern_literal(ctx, "ALPHABETIC");

        if (xkb_keysym_is_keypad(sym0) || xkb_keysym_is_keypad(sym1))
            return xkb_atom_intern_literal(ctx, "KEYPAD");

        return xkb_atom_intern_literal(ctx, "TWO_LEVEL");
    }

    if (width <= 4) {
        if (xkb_keysym_is_lower(sym0) && xkb_keysym_is_upper_or_title(sym1)) {
            xkb_keysym_t sym2, sym3;
            sym2 = GET_SYM(2);
            sym3 = (width == 4 ? GET_SYM(3) : XKB_KEY_NoSymbol);

            if (xkb_keysym_is_lower(sym2) && xkb_keysym_is_upper_or_title(sym3))
                return xkb_atom_intern_literal(ctx, "FOUR_LEVEL_ALPHABETIC");

            return xkb_atom_intern_literal(ctx, "FOUR_LEVEL_SEMIALPHABETIC");
        }

        if (xkb_keysym_is_keypad(sym0) || xkb_keysym_is_keypad(sym1))
            return xkb_atom_intern_literal(ctx, "FOUR_LEVEL_KEYPAD");

        return xkb_atom_intern_literal(ctx, "FOUR_LEVEL");
    }

    return XKB_ATOM_NONE;

#undef GET_SYM
}

static const struct xkb_key_type *
FindTypeForGroup(struct xkb_keymap *keymap, KeyInfo *keyi,
                 xkb_layout_index_t group, bool *explicit_type)
{
    unsigned int i;
    GroupInfo *groupi = &darray_item(keyi->groups, group);
    xkb_atom_t type_name = groupi->type;

    *explicit_type = true;

    if (type_name == XKB_ATOM_NONE) {
        if (keyi->default_type != XKB_ATOM_NONE) {
            type_name = keyi->default_type;
        }
        else {
            type_name = FindAutomaticType(keymap->ctx, groupi);
            if (type_name != XKB_ATOM_NONE)
                *explicit_type = false;
        }
    }

    if (type_name == XKB_ATOM_NONE) {
        log_warn(keymap->ctx, XKB_WARNING_CANNOT_INFER_KEY_TYPE,
                 "Couldn't find an automatic type for key '%s' "
                 "group %d with %lu levels; Using the default type\n",
                 KeyNameText(keymap->ctx, keyi->name), group + 1,
                 (unsigned long) darray_size(groupi->levels));
        goto use_default;
    }

    for (i = 0; i < keymap->num_types; i++)
        if (keymap->types[i].name == type_name)
            break;

    if (i >= keymap->num_types) {
        log_warn(keymap->ctx, XKB_WARNING_UNDEFINED_KEY_TYPE,
                 "The type \"%s\" for key '%s' group %d "
                 "was not previously defined; Using the default type\n",
                 xkb_atom_text(keymap->ctx, type_name),
                 KeyNameText(keymap->ctx, keyi->name), group + 1);
        goto use_default;
    }

    return &keymap->types[i];

use_default:
    /*
     * Index 0 is guaranteed to contain something, usually
     * ONE_LEVEL or at least some default one-level type.
     */
    return &keymap->types[0];
}

static bool
CopySymbolsDefToKeymap(struct xkb_keymap *keymap, SymbolsInfo *info,
                       KeyInfo *keyi)
{
    struct xkb_key *key;
    GroupInfo *groupi;
    const GroupInfo *group0;
    xkb_layout_index_t i;

    /*
     * The name is guaranteed to be real and not an alias (see
     * AddKeySymbols), so 'false' is safe here.
     */
    key = XkbKeyByName(keymap, keyi->name, false);
    if (!key) {
        log_vrb(info->ctx, 5, XKB_WARNING_UNDEFINED_KEYCODE,
                "Key %s not found in keycodes; Symbols ignored\n",
                KeyInfoText(info, keyi));
        return false;
    }

    /* Find the range of groups we need. */
    key->num_groups = 0;
    darray_enumerate(i, groupi, keyi->groups) {
        /* Skip groups that have no levels and no explicit type.
         * Such group would be filled with `NoSymbol` anyway. */
        const bool has_explicit_type = (keyi->defined & KEY_FIELD_DEFAULT_TYPE)
                                    || (groupi->defined & GROUP_FIELD_TYPE);
        if (darray_size(groupi->levels) > 0 || has_explicit_type)
            key->num_groups = i + 1;
        if (has_explicit_type)
            key->explicit |= EXPLICIT_TYPES;
    }

    if (key->num_groups <= 0) {
        /* A key with no group may still have other fields defined */
        if (keyi->defined)
            goto key_fields;
        else
            return false;
    }

    darray_resize(keyi->groups, key->num_groups);

    /*
     * If there are empty groups between non-empty ones, fill them with data
     * from the first group.
     * We can make a wrong assumption here. But leaving gaps is worse.
     */
    group0 = &darray_item(keyi->groups, 0);
    darray_foreach_from(groupi, keyi->groups, 1) {
        if (groupi->defined)
            continue;

        CopyGroupInfo(groupi, group0);
    }

    key->groups = calloc(key->num_groups, sizeof(*key->groups));

    /* Find and assign the groups' types in the keymap. */
    darray_enumerate(i, groupi, keyi->groups) {
        const struct xkb_key_type *type;
        bool explicit_type;

        type = FindTypeForGroup(keymap, keyi, i, &explicit_type);

        /* Always have as many levels as the type specifies. */
        if (type->num_levels < darray_size(groupi->levels)) {
            struct xkb_level *leveli;

            log_vrb(info->ctx, 1, XKB_WARNING_EXTRA_SYMBOLS_IGNORED,
                    "Type \"%s\" has %d levels, but %s has %u levels; "
                    "Ignoring extra symbols\n",
                    xkb_atom_text(keymap->ctx, type->name), type->num_levels,
                    KeyInfoText(info, keyi),
                    darray_size(groupi->levels));

            darray_foreach_from(leveli, groupi->levels, type->num_levels)
                clear_level(leveli);
        }
        darray_resize0(groupi->levels, type->num_levels);

        key->groups[i].explicit_type = explicit_type;
        key->groups[i].type = type;
    }

    /* Copy levels. */
    darray_enumerate(i, groupi, keyi->groups) {
        /*
         * Compute the capitalization transformation of the keysyms.
         * This is necessary because `xkb_state_key_get_syms()` returns an
         * immutable array and does not use a buffer, so we must store them.
         * We apply only simple capitalization rules, so the keysyms count is
         * unchanged.
         */
        struct xkb_level *leveli;
        darray_foreach(leveli, groupi->levels) {
            switch (leveli->num_syms) {
            case 0:
                leveli->upper = XKB_KEY_NoSymbol;
                break;
            case 1:
                leveli->upper = xkb_keysym_to_upper(leveli->s.sym);
                break;
            default:
                /* Multiple keysyms: check if there is any cased keysym */
                leveli->has_upper = false;
                for (xkb_keysym_count_t k = 0; k < leveli->num_syms; k++) {
                    const xkb_keysym_t upper =
                        xkb_keysym_to_upper(leveli->s.syms[k]);
                    if (upper != leveli->s.syms[k]) {
                        leveli->has_upper = true;
                        break;
                    }
                }
                if (leveli->has_upper) {
                    /*
                     * Some cased keysyms: store the transformation result in
                     * the same array, right after the original keysyms.
                     */
                    leveli->s.syms = realloc(leveli->s.syms,
                                             (size_t) 2 * leveli->num_syms *
                                             sizeof(*leveli->s.syms));
                    if (!leveli->s.syms)
                        return false; /* FIXME: better handling? */
                    for (xkb_keysym_count_t k = 0; k < leveli->num_syms; k++) {
                        leveli->s.syms[leveli->num_syms + k] =
                            xkb_keysym_to_upper(leveli->s.syms[k]);
                    }
                }
            }
        }

        /* Copy the level */
        darray_steal(groupi->levels, &key->groups[i].levels, NULL);
        if (key->groups[i].type->num_levels > 1 ||
            key->groups[i].levels[0].num_syms > 0)
            key->explicit |= EXPLICIT_SYMBOLS;
        if (groupi->defined & GROUP_FIELD_ACTS) { // FIXME
            key->groups[i].explicit_actions = true;
            key->explicit |= EXPLICIT_INTERP;
        }
        if (key->groups[i].explicit_type)
            key->explicit |= EXPLICIT_TYPES;
    }

    key->out_of_range_group_number = keyi->out_of_range_group_number;
    key->out_of_range_group_action = keyi->out_of_range_group_action;

key_fields:
    if (keyi->defined & KEY_FIELD_VMODMAP) {
        key->vmodmap = keyi->vmodmap;
        key->explicit |= EXPLICIT_VMODMAP;
    }

    if (keyi->repeat != KEY_REPEAT_UNDEFINED) {
        key->repeats = (keyi->repeat == KEY_REPEAT_YES);
        key->explicit |= EXPLICIT_REPEAT;
    }

    return true;
}

static bool
CopyModMapDefToKeymap(struct xkb_keymap *keymap, SymbolsInfo *info,
                      ModMapEntry *entry)
{
    struct xkb_key *key;

    if (!entry->haveSymbol) {
        key = XkbKeyByName(keymap, entry->u.keyName, true);
        if (!key) {
            log_vrb(info->ctx, 5, XKB_WARNING_UNDEFINED_KEYCODE,
                    "Key %s not found in keycodes; "
                    "Modifier map entry for %s not updated\n",
                    KeyNameText(info->ctx, entry->u.keyName),
                    ModIndexText(info->ctx, &info->mods, entry->modifier));
            return false;
        }
    }
    else {
        key = FindKeyForSymbol(keymap, entry->u.keySym);
        if (!key) {
            log_vrb(info->ctx, 5, XKB_WARNING_UNRESOLVED_KEYMAP_SYMBOL,
                    "Key \"%s\" not found in symbol map; "
                    "Modifier map entry for %s not updated\n",
                    KeysymText(info->ctx, entry->u.keySym),
                    ModIndexText(info->ctx, &info->mods, entry->modifier));
            return false;
        }
    }

    /* Skip modMap None */
    if (entry->modifier != XKB_MOD_NONE) {
        /* Convert modifier index to modifier mask */
        key->modmap |= (UINT32_C(1) << entry->modifier);
    }

    return true;
}

static bool
CopySymbolsToKeymap(struct xkb_keymap *keymap, SymbolsInfo *info)
{
    KeyInfo *keyi;
    ModMapEntry *mm;

    keymap->symbols_section_name = strdup_safe(info->name);
    XkbEscapeMapName(keymap->symbols_section_name);

    keymap->mods = info->mods;

    darray_steal(info->group_names,
                 &keymap->group_names, &keymap->num_group_names);

    darray_foreach(keyi, info->keys)
        if (!CopySymbolsDefToKeymap(keymap, info, keyi))
            info->errorCount++;

    if (xkb_context_get_log_verbosity(keymap->ctx) > 3) {
        struct xkb_key *key;

        xkb_keys_foreach(key, keymap) {
            if (key->name == XKB_ATOM_NONE)
                continue;

            if (key->num_groups < 1)
                log_info(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                         "No symbols defined for %s\n",
                         KeyNameText(info->ctx, key->name));
        }
    }

    darray_foreach(mm, info->modmaps)
        if (!CopyModMapDefToKeymap(keymap, info, mm))
            info->errorCount++;

    /* XXX: If we don't ignore errorCount, things break. */
    return true;
}

bool
CompileSymbols(XkbFile *file, struct xkb_keymap *keymap)
{
    SymbolsInfo info;

    InitSymbolsInfo(&info, keymap, 0, &keymap->mods);

    if (file !=NULL)
        HandleSymbolsFile(&info, file);

    if (info.errorCount != 0)
        goto err_info;

    if (!CopySymbolsToKeymap(keymap, &info))
        goto err_info;

    ClearSymbolsInfo(&info);
    return true;

err_info:
    ClearSymbolsInfo(&info);
    return false;
}
