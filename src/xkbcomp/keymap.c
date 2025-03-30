/*
 * Copyright © 2009 Dan Nicholson
 * Copyright © 2012 Intel Corporation
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 *
 * Author: Dan Nicholson <dbn.lists@gmail.com>
 * Author: Daniel Stone <daniel@fooishbar.org>
 * Author: Ran Benita <ran234@gmail.com>
 */

#include "config.h"

#include "darray.h"
#include "xkbcomp-priv.h"
#include "text.h"

static void
ComputeEffectiveMask(struct xkb_keymap *keymap, struct xkb_mods *mods)
{
    mods->mask = mod_mask_get_effective(keymap, mods->mods);
}

static void
UpdateActionMods(struct xkb_keymap *keymap, union xkb_action *act,
                 xkb_mod_mask_t modmap)
{
    switch (act->type) {
    case ACTION_TYPE_MOD_SET:
    case ACTION_TYPE_MOD_LATCH:
    case ACTION_TYPE_MOD_LOCK:
        if (act->mods.flags & ACTION_MODS_LOOKUP_MODMAP)
            act->mods.mods.mods = modmap;
        ComputeEffectiveMask(keymap, &act->mods.mods);
        break;
    default:
        break;
    }
}

static const struct xkb_sym_interpret default_interpret = {
    .sym = XKB_KEY_NoSymbol,
    .repeat = true,
    .match = MATCH_ANY_OR_NONE,
    .mods = 0,
    .virtual_mod = XKB_MOD_INVALID,
    .action = { .type = ACTION_TYPE_NONE },
};

typedef darray(const struct xkb_sym_interpret*) xkb_sym_interprets;

/**
 * Find an interpretation which applies to this particular level, either by
 * finding an exact match for the symbol and modifier combination, or a
 * generic XKB_KEY_NoSymbol match.
 */
static bool
FindInterpForKey(struct xkb_keymap *keymap, const struct xkb_key *key,
                 xkb_layout_index_t group, xkb_level_index_t level,
                 xkb_sym_interprets *interprets)
{
    const xkb_keysym_t *syms;
    int num_syms;

    num_syms = xkb_keymap_key_get_syms_by_level(keymap, key->keycode, group,
                                                level, &syms);
    if (num_syms <= 0)
        return false;

    /*
     * There may be multiple matchings interprets; we should always return
     * the most specific. Here we rely on compat.c to set up the
     * sym_interprets array from the most specific to the least specific,
     * such that when we find a match we return immediately.
     */
    for (int s = 0; s < num_syms; s++) {
        bool found = false;
        for (unsigned int i = 0; i < keymap->num_sym_interprets; i++) {
            const struct xkb_sym_interpret *interp = &keymap->sym_interprets[i];
            xkb_mod_mask_t mods;

            found = false;

            if (interp->sym != syms[s] && interp->sym != XKB_KEY_NoSymbol)
                continue;

            if (interp->level_one_only && level != 0)
                mods = 0;
            else
                mods = key->modmap;

            switch (interp->match) {
            case MATCH_NONE:
                found = !(interp->mods & mods);
                break;
            case MATCH_ANY_OR_NONE:
                found = (!mods || (interp->mods & mods));
                break;
            case MATCH_ANY:
                found = (interp->mods & mods);
                break;
            case MATCH_ALL:
                found = ((interp->mods & mods) == interp->mods);
                break;
            case MATCH_EXACTLY:
                found = (interp->mods == mods);
                break;
            }

            if (found && i > 0 && interp->sym == XKB_KEY_NoSymbol) {
                /*
                 * For an interpretation matching Any keysym, we may get the
                 * same interpretation for multiple keysyms. This may result in
                 * unwanted duplicate actions. So set this interpretation only
                 * if no previous keysym was matched with this interpret at this
                 * level, else set the default interpretation.
                 */
                struct xkb_sym_interpret const **previous_interp;
                darray_foreach(previous_interp, *interprets) {
                    if (*previous_interp == interp) {
                        /* Keep as a safeguard against refactoring
                         * NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores) */
                        found = false;
                        log_warn(keymap->ctx, XKB_LOG_MESSAGE_NO_ID,
                                 "Repeated interpretation ignored for keysym "
                                 "#%u \"%s\" at level %u/group %u on key %s.\n",
                                 s + 1, KeysymText(keymap->ctx, syms[s]),
                                 level + 1, group + 1,
                                 KeyNameText(keymap->ctx, key->name));
                        goto not_found;
                    }
                }
            }
            if (found) {
                darray_append(*interprets, interp);
                break;
            }
        }
        if (!found)
not_found:
            darray_append(*interprets, &default_interpret);
    }
    return true;
}

static bool
ApplyInterpsToKey(struct xkb_keymap *keymap, struct xkb_key *key)
{
    xkb_mod_mask_t vmodmap = 0;
    xkb_layout_index_t group;
    xkb_level_index_t level;
    // FIXME: do not use darray, add actions directly in FindInterpForKey
    xkb_sym_interprets interprets = darray_new();

    for (group = 0; group < key->num_groups; group++) {
        /* Skip any interpretation for this group if it has explicit actions */
        if (key->groups[group].explicit_actions)
            continue;

        for (level = 0; level < XkbKeyNumLevels(key, group); level++) {
            assert(key->groups[group].levels[level].num_actions == 0);

            const struct xkb_sym_interpret **interp_iter;
            const struct xkb_sym_interpret *interp;
            size_t k;
            darray_resize(interprets, 0);

            const bool found = FindInterpForKey(keymap, key, group, level, &interprets);
            if (!found)
                continue;

            key->groups[group].levels[level].num_actions =
                key->groups[group].levels[level].num_syms;

            /* Allocate actions */
            if (key->groups[group].levels[level].num_actions > 1) {
                key->groups[group].levels[level].a.actions =
                    calloc(key->groups[group].levels[level].num_actions,
                           sizeof(*key->groups[group].levels[level].a.actions));
            }

            darray_enumerate(k, interp_iter, interprets) {
                interp = *interp_iter;
                /* Infer default key behaviours from the base level. */
                if (group == 0 && level == 0)
                    if (!(key->explicit & EXPLICIT_REPEAT) && interp->repeat)
                        key->repeats = true;

                if ((group == 0 && level == 0) || !interp->level_one_only)
                    if (interp->virtual_mod != XKB_MOD_INVALID)
                        vmodmap |= (UINT32_C(1) << interp->virtual_mod);

                if (interp->action.type != ACTION_TYPE_NONE) {
                    if (key->groups[group].levels[level].num_actions == 1) {
                        key->groups[group].levels[level].a.action = interp->action;
                    } else {
                        key->groups[group].levels[level].a.actions[k] = interp->action;
                    }
                }
            }
        }
    }

    darray_free(interprets);

    if (!(key->explicit & EXPLICIT_VMODMAP))
        key->vmodmap = vmodmap;

    return true;
}

static inline bool
is_mod_action(union xkb_action *action)
{
    return action->type == ACTION_TYPE_MOD_SET ||
           action->type == ACTION_TYPE_MOD_LATCH ||
           action->type == ACTION_TYPE_MOD_LOCK;
}

static inline bool
is_group_action(union xkb_action *action)
{
    return action->type == ACTION_TYPE_GROUP_SET ||
           action->type == ACTION_TYPE_GROUP_LATCH ||
           action->type == ACTION_TYPE_GROUP_LOCK;
}

/* Check for mixing actions of the same category.
 * We do not support that yet, because it needs a careful refactor of the state
 * handling. See: `xkb_filter_apply_all`. */
static void
CheckMultipleActionsCategories(struct xkb_keymap *keymap, struct xkb_key *key)
{
    for (xkb_layout_index_t g = 0; g < key->num_groups; g++) {
        for (xkb_level_index_t l = 0; l < XkbKeyNumLevels(key, g); l++) {
            struct xkb_level *level = &key->groups[g].levels[l];
            if (level->num_actions <= 1)
                continue;
            for (unsigned i = 0; i < level->num_actions; i++) {
                union xkb_action *action1 = &level->a.actions[i];
                bool mod_action = is_mod_action(action1);
                bool group_action = is_group_action(action1);
                if (!(mod_action || group_action))
                    continue;
                for (unsigned j = i + 1; j < level->num_actions; j++) {
                    union xkb_action *action2 = &level->a.actions[j];
                    if ((mod_action && is_mod_action(action2)) ||
                        (group_action && is_group_action(action2)))
                    {
                        log_err(keymap->ctx, XKB_LOG_MESSAGE_NO_ID,
                                "Cannot use multiple %s actions "
                                "in the same level. Action #%u "
                                "for key %s in group %u/level %u ignored.\n",
                                (mod_action ? "modifiers" : "group"),
                                j + 1, KeyNameText(keymap->ctx, key->name),
                                g + 1, l + 1);
                        action2->type = ACTION_TYPE_NONE;
                    }
                }
            }
        }
    }
}

/**
 * This collects a bunch of disparate functions which was done in the server
 * at various points that really should've been done within xkbcomp.  Turns out
 * your actions and types are a lot more useful when any of your modifiers
 * other than Shift actually do something ...
 */
static bool
UpdateDerivedKeymapFields(struct xkb_keymap *keymap)
{
    struct xkb_key *key;
    struct xkb_mod *mod;
    struct xkb_led *led;
    unsigned int i, j, k;

    /* Find all the interprets for the key and bind them to actions,
     * which will also update the vmodmap. */
    xkb_keys_foreach(key, keymap) {
        if (!ApplyInterpsToKey(keymap, key))
            return false;
        CheckMultipleActionsCategories(keymap, key);
    }

    /* Update keymap->mods, the virtual -> real mod mapping. */
    xkb_keys_foreach(key, keymap)
        xkb_mods_enumerate(i, mod, &keymap->mods)
            if (key->vmodmap & (UINT32_C(1) << i))
                mod->mapping |= key->modmap;

    /* Now update the level masks for all the types to reflect the vmods. */
    for (i = 0; i < keymap->num_types; i++) {
        ComputeEffectiveMask(keymap, &keymap->types[i].mods);

        for (j = 0; j < keymap->types[i].num_entries; j++) {
            ComputeEffectiveMask(keymap, &keymap->types[i].entries[j].mods);
            ComputeEffectiveMask(keymap, &keymap->types[i].entries[j].preserve);
        }
    }

    /* Update action modifiers. */
    xkb_keys_foreach(key, keymap)
        for (i = 0; i < key->num_groups; i++)
            for (j = 0; j < XkbKeyNumLevels(key, i); j++) {
                if (key->groups[i].levels[j].num_actions == 1) {
                    UpdateActionMods(keymap, &key->groups[i].levels[j].a.action,
                                     key->modmap);
                } else {
                    for (k = 0; k < key->groups[i].levels[j].num_actions; k++) {
                        UpdateActionMods(keymap,
                                         &key->groups[i].levels[j].a.actions[k],
                                         key->modmap);
                    }
                }
            }

    /* Update vmod -> led maps. */
    xkb_leds_foreach(led, keymap)
        ComputeEffectiveMask(keymap, &led->mods);

    /* Find maximum number of groups out of all keys in the keymap. */
    xkb_keys_foreach(key, keymap)
        keymap->num_groups = MAX(keymap->num_groups, key->num_groups);

    return true;
}

typedef bool (*compile_file_fn)(XkbFile *file,
                                struct xkb_keymap *keymap,
                                enum merge_mode merge);

static const compile_file_fn compile_file_fns[LAST_KEYMAP_FILE_TYPE + 1] = {
    [FILE_TYPE_KEYCODES] = CompileKeycodes,
    [FILE_TYPE_TYPES] = CompileKeyTypes,
    [FILE_TYPE_COMPAT] = CompileCompatMap,
    [FILE_TYPE_SYMBOLS] = CompileSymbols,
};

bool
CompileKeymap(XkbFile *file, struct xkb_keymap *keymap, enum merge_mode merge)
{
    XkbFile *files[LAST_KEYMAP_FILE_TYPE + 1] = { NULL };
    enum xkb_file_type type;
    struct xkb_context *ctx = keymap->ctx;

    /* Collect section files and check for duplicates. */
    for (file = (XkbFile *) file->defs; file;
         file = (XkbFile *) file->common.next) {
        if (file->file_type < FIRST_KEYMAP_FILE_TYPE ||
            file->file_type > LAST_KEYMAP_FILE_TYPE) {
            if (file->file_type == FILE_TYPE_GEOMETRY) {
                log_vrb(ctx, 1,
                        XKB_WARNING_UNSUPPORTED_GEOMETRY_SECTION,
                        "Geometry sections are not supported; ignoring\n");
            } else {
                log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                        "Cannot define %s in a keymap file\n",
                        xkb_file_type_to_string(file->file_type));
            }
            continue;
        }

        if (files[file->file_type]) {
            log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                    "More than one %s section in keymap file; "
                    "All sections after the first ignored\n",
                    xkb_file_type_to_string(file->file_type));
            continue;
        }

        files[file->file_type] = file;
    }

    /*
     * Compile sections
     *
     * NOTE: Any component is optional.
     */
    for (type = FIRST_KEYMAP_FILE_TYPE;
         type <= LAST_KEYMAP_FILE_TYPE;
         type++) {
        if (files[type] == NULL) {
            log_dbg(ctx, XKB_LOG_MESSAGE_NO_ID,
                    "Component %s not provided in keymap\n",
                    xkb_file_type_to_string(type));
        } else {
            log_dbg(ctx, XKB_LOG_MESSAGE_NO_ID,
                    "Compiling %s \"%s\"\n",
                    xkb_file_type_to_string(type), files[type]->name);
        }

        /* Missing components are initialized with defaults */
        const bool ok = compile_file_fns[type](files[type], keymap, merge);
        if (!ok) {
            log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                    "Failed to compile %s\n",
                    xkb_file_type_to_string(type));
            return false;
        }
    }

    return UpdateDerivedKeymapFields(keymap);
}
