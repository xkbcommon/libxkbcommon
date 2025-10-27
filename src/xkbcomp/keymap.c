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

#include <string.h>
#include <stdbool.h>

#include "xkbcommon/xkbcommon.h"
#include "darray.h"
#include "keymap.h"
#include "text.h"
#include "utils.h"
#include "xkbcomp-priv.h"

static bool
has_unbound_vmods(struct xkb_keymap *keymap, xkb_mod_mask_t mask)
{
    xkb_mod_index_t k;
    struct xkb_mod *mod;
    xkb_vmods_enumerate(k, mod, &keymap->mods)
        if ((mask & (UINT32_C(1) << k)) && mod->mapping == 0)
            return true;
    return false;
}

static inline void
ComputeEffectiveMask(struct xkb_keymap *keymap, struct xkb_mods *mods)
{
    /*
     * Since we accept numeric values for the vmod mask in the keymap, we may
     * have extra bits set that encode no real/virtual modifier. Keep them
     * unchanged for consistency.
     */
    const xkb_mod_mask_t unknown_mods = ~(xkb_mod_mask_t)
        ((UINT64_C(1) << keymap->mods.num_mods) - UINT64_C(1));
    mods->mask = mod_mask_get_effective(keymap, mods->mods)
               | (mods->mods & unknown_mods);
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

const struct xkb_sym_interpret default_interpret = {
    /* Keysym unused for when applying interpretation, but used as a default
     * entry when dumping the keymap */
    .sym = XKB_KEY_VoidSymbol,
    .repeat = true,
    .match = MATCH_ANY_OR_NONE,
    .mods = 0,
    .virtual_mod = XKB_MOD_INVALID,
    .num_actions = 0,
    .a = { .action = { .type = ACTION_TYPE_NONE } },
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
        for (darray_size_t i = 0; i < keymap->num_sym_interprets; i++) {
            struct xkb_sym_interpret * const interp = &keymap->sym_interprets[i];
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
                                 "#%d \"%s\" at level %"PRIu32"/group %"PRIu32" "
                                 "on key %s.\n",
                                 s + 1, KeysymText(keymap->ctx, syms[s]),
                                 level + 1, group + 1,
                                 KeyNameText(keymap->ctx, key->name));
                        goto not_found;
                    }
                }
            }
            if (found) {
                darray_append(*interprets, interp);
                interp->required = true;
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
    xkb_level_index_t level;
    // FIXME: do not use darray, add actions directly in FindInterpForKey
    xkb_sym_interprets interprets = darray_new();
    darray(union xkb_action) actions = darray_new();

    for (xkb_layout_index_t group = 0; group < key->num_groups; group++) {
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

            darray_enumerate(k, interp_iter, interprets) {
                interp = *interp_iter;
                /* Infer default key behaviours from the base level. */
                if (group == 0 && level == 0)
                    if (!(key->explicit & EXPLICIT_REPEAT) && interp->repeat)
                        key->repeats = true;

                if ((group == 0 && level == 0) || !interp->level_one_only)
                    if (interp->virtual_mod != XKB_MOD_INVALID)
                        vmodmap |= (UINT32_C(1) << interp->virtual_mod);

                switch (interp->num_actions) {
                case 0:
                    break;
                case 1:
                    darray_append(actions, interp->a.action);
                    break;
                default:
                    darray_append_items(actions, interp->a.actions,
                                        interp->num_actions);
                }
            }

            /* Copy the actions */
            if (unlikely(darray_size(actions)) > MAX_ACTIONS_PER_LEVEL) {
                log_warn(keymap->ctx, XKB_LOG_MESSAGE_NO_ID,
                         "Could not append interpret actions to key %s: "
                         "maximum is %u, got: %u. Dropping excessive actions\n",
                         KeyNameText(keymap->ctx, key->name),
                         MAX_ACTIONS_PER_LEVEL, darray_size(actions));
                key->groups[group].levels[level].num_actions =
                    MAX_ACTIONS_PER_LEVEL;
            } else {
                key->groups[group].levels[level].num_actions =
                    (xkb_action_count_t) darray_size(actions);
            }
            switch (darray_size(actions)) {
                case 0:
                    key->groups[group].levels[level].a.action =
                        (union xkb_action) { .type = ACTION_TYPE_NONE };
                    break;
                case 1:
                    key->groups[group].levels[level].a.action =
                        darray_item(actions, 0);
                    break;
                default:
                    key->groups[group].levels[level].a.actions =
                        memdup(darray_items(actions),
                               key->groups[group].levels[level].num_actions,
                               sizeof(*darray_items(actions)));
                    if (!key->groups[group].levels[level].a.actions) {
                        log_err(keymap->ctx, XKB_ERROR_ALLOCATION_ERROR,
                                "Could not allocate interpret actions\n");
                        darray_free(actions);
                        darray_free(interprets);
                        return false;
                    }
            }

            /* Do not free here */
            darray_resize(actions, 0);
        }
    }
    darray_free(actions);
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
            for (xkb_action_count_t i = 0; i < level->num_actions; i++) {
                union xkb_action *action1 = &level->a.actions[i];
                bool mod_action = is_mod_action(action1);
                bool group_action = is_group_action(action1);
                if (!(mod_action || group_action))
                    continue;
                for (xkb_action_count_t j = i + 1; j < level->num_actions; j++) {
                    union xkb_action *action2 = &level->a.actions[j];
                    if ((mod_action && is_mod_action(action2)) ||
                        (group_action && is_group_action(action2)))
                    {
                        log_err(keymap->ctx, XKB_LOG_MESSAGE_NO_ID,
                                "Cannot use multiple %s actions "
                                "in the same level. Action #%u "
                                "for key %s in group %"PRIu32"/level %"PRIu32" "
                                "ignored.\n",
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

static void
add_key_aliases(struct xkb_keymap *keymap, darray_size_t min, darray_size_t max,
                struct xkb_key_alias *aliases)
{
    darray_size_t num_key_aliases = 0;
    for (darray_size_t alias = min; alias <= max; alias++) {
        const KeycodeMatch entry = keymap->key_names[alias];
        if (entry.is_alias && entry.found) {
            aliases[num_key_aliases++] = (struct xkb_key_alias) {
                .alias = alias,
                .real = entry.alias.real
            };
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
    /**
     * Create the key alias list
     *
     * Since we already allocated the key name LUT, we try to overwrite it:
     * - An alias entry is exactly 2× the size of a LUT entry (on most compilers).
     * - We ensure that there is a real key for each alias.
     * - So, if each alias maps to a different keycode, we usually have:
     *   current alloc(keymap.key_names) ≥ needed alloc(keymap.key_aliases).
     * - Usual keymaps have enough space in the LUT to enable overwriting real
     *   key entries without overlap with the alias entries.
     * - In the unlikely event that this is not the case, we need to realloc.
     */
#ifdef _WIN32
    /* Bit fields are not packed on Windows as efficiently as other platforms */
    static_assert(sizeof(struct xkb_key_alias) == sizeof(KeycodeMatch), "");
#else
    static_assert(sizeof(struct xkb_key_alias) == 2 * sizeof(KeycodeMatch), "");
#endif
    darray_size_t num_key_aliases = 0;
    darray_size_t min_alias = DARRAY_SIZE_MAX;
    darray_size_t max_alias = 0;
    for (xkb_atom_t alias = 0; alias < keymap->num_key_names; alias++) {
        const KeycodeMatch entry = keymap->key_names[alias];
        if (entry.is_alias && entry.found) {
            num_key_aliases++;
            if (min_alias == DARRAY_SIZE_MAX)
                min_alias = alias;
            max_alias = alias;
        }
    }
    if (num_key_aliases) {
        /*
         * No fancy algorithm used here: either we can trivially write the whole
         * range of `xkb_key_aliases` without overlapping with the `KeycodeMatch`
         * alias entries or we allocate a new array.
         * In practice no new allocation is is needed and it delivers performs
         * better than using a buffer to handle overlaps.
         */
        const darray_size_t required_space = sizeof(struct xkb_key_alias)
                                           / sizeof(KeycodeMatch)
                                           * num_key_aliases;
        if (min_alias >= required_space) {
            /* Overwrite before the *first* alias entry */
            add_key_aliases(keymap, min_alias, max_alias, keymap->key_aliases);
            /* Shrink */
            struct xkb_key_alias * const r =
                realloc(keymap->key_aliases,
                        num_key_aliases * sizeof(*keymap->key_aliases));
            if (r == NULL)
                return false;
            keymap->key_aliases = r;
        } else if (keymap->num_key_names - max_alias - 1 > required_space) {
            /* Overwrite after the *last* alias entry, then move to the start */
            struct xkb_key_alias * const aliases = (struct xkb_key_alias *) (
                keymap->key_names - required_space -
                !is_aligned(keymap->key_names - required_space,
                            sizeof(struct xkb_key_alias))
            );
            add_key_aliases(keymap, min_alias, max_alias, aliases);
            memcpy(keymap->key_aliases, aliases,
                   num_key_aliases * sizeof(*aliases));
            /* Shrink */
            struct xkb_key_alias * const r =
                realloc(keymap->key_aliases,
                        num_key_aliases * sizeof(*keymap->key_aliases));
            if (r == NULL)
                return false;
            keymap->key_aliases = r;
        } else {
            /*
             * No space trivially available: instead of processing the LUT with
             * a buffer to handle overlaps, we simply allocate a dedicated array
             */
            struct xkb_key_alias * const aliases = calloc(num_key_aliases,
                                                          sizeof(*aliases));
            if (aliases == NULL)
                return false;
            add_key_aliases(keymap, min_alias, max_alias, aliases);
            free(keymap->key_names);
            keymap->key_aliases = aliases;
        }
    }
    keymap->num_key_aliases = num_key_aliases;

    /* Find all the interprets for the key and bind them to actions,
     * which will also update the vmodmap. */
    struct xkb_key *key;
    xkb_keys_foreach(key, keymap) {
        if (!ApplyInterpsToKey(keymap, key))
            return false;
        CheckMultipleActionsCategories(keymap, key);
    }

    /* Update keymap->mods, the virtual -> real mod mapping. */
    xkb_mod_index_t idx;
    struct xkb_mod *mod;
    xkb_keys_foreach(key, keymap)
        xkb_vmods_enumerate(idx, mod, &keymap->mods)
            if (key->vmodmap & (UINT32_C(1) << idx))
                mod->mapping |= key->modmap;

    if (keymap->format >= XKB_KEYMAP_FORMAT_TEXT_V2) {
        /*
         * XKB extension: canonical virtual modifiers (incompatible with X11)
         *
         * Virtual modifiers that are not mapped, map to their canonical
         * encoding, i.e. themselves.
         *
         * This excludes:
         * - Modifiers mapped *explicitly*: e.g. `virtual_modifiers M = 0x1;`.
         * - Modifiers mapped *implicitly* via vmodmap/modifier_map, i.e. the
         *   usual X11 way used in xkeyboard-config.
         */
        xkb_vmods_enumerate(idx, mod, &keymap->mods) {
            const xkb_mod_mask_t mask = UINT32_C(1) << idx;
            if (mod->mapping == 0 && !(keymap->mods.explicit_vmods & mask)) {
                mod->mapping = mask;
                /*
                 * Make the mapping explicit, so that it is interoperable
                 * without relying on the implementation-specific virtual
                 * modifiers indices.
                 */
                keymap->mods.explicit_vmods |= mask;
            }
        }
    }

    /* Update the canonical modifiers state mask */
    assert(keymap->canonical_state_mask == MOD_REAL_MASK_ALL);
    xkb_mod_mask_t extra_canonical_mods = 0;
    xkb_vmods_enumerate(idx, mod, &keymap->mods) {
        extra_canonical_mods |= mod->mapping;
    }
    keymap->canonical_state_mask |= extra_canonical_mods;

    /* Now update the level masks for all the types to reflect the vmods. */
    for (darray_size_t i = 0; i < keymap->num_types; i++) {
        ComputeEffectiveMask(keymap, &keymap->types[i].mods);

        for (darray_size_t j = 0; j < keymap->types[i].num_entries; j++) {
            if (has_unbound_vmods(keymap,
                                  keymap->types[i].entries[j].mods.mods)) {
                /*
                 * Map entries which specify unbound virtual modifiers are not
                 * considered (see the XKB protocol, section “Determining the
                 * KeySym Associated with a Key Event”).
                 *
                 * Deactivate entry by zeroing its mod mask and skipping further
                 * processing.
                 *
                 * See also: `entry_is_active`.
                 */
                keymap->types[i].entries[j].mods.mask = 0;
                continue;
            }
            ComputeEffectiveMask(keymap, &keymap->types[i].entries[j].mods);
            ComputeEffectiveMask(keymap, &keymap->types[i].entries[j].preserve);
        }
    }

    /* Update action modifiers. */
    xkb_keys_foreach(key, keymap) {
        for (xkb_layout_index_t i = 0; i < key->num_groups; i++) {
            for (xkb_level_index_t j = 0; j < XkbKeyNumLevels(key, i); j++) {
                if (key->groups[i].levels[j].num_actions == 1) {
                    UpdateActionMods(keymap, &key->groups[i].levels[j].a.action,
                                     key->modmap);
                } else {
                    for (xkb_action_count_t k = 0;
                         k < key->groups[i].levels[j].num_actions; k++) {
                        UpdateActionMods(keymap,
                                         &key->groups[i].levels[j].a.actions[k],
                                         key->modmap);
                    }
                }
            }
        }
    }

    /* Update vmod -> led maps. */
    struct xkb_led *led;
    xkb_leds_foreach(led, keymap)
        ComputeEffectiveMask(keymap, &led->mods);

    /* Find maximum number of groups out of all keys in the keymap. */
    xkb_keys_foreach(key, keymap)
        keymap->num_groups = MAX(keymap->num_groups, key->num_groups);

    return true;
}

typedef bool (*compile_file_fn)(XkbFile *file, struct xkb_keymap *keymap);

static const compile_file_fn compile_file_fns[LAST_KEYMAP_FILE_TYPE + 1] = {
    [FILE_TYPE_KEYCODES] = CompileKeycodes,
    [FILE_TYPE_TYPES] = CompileKeyTypes,
    [FILE_TYPE_COMPAT] = CompileCompatMap,
    [FILE_TYPE_SYMBOLS] = CompileSymbols,
};

bool
CompileKeymap(XkbFile *file, struct xkb_keymap *keymap)
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
                log_vrb(ctx, XKB_LOG_VERBOSITY_BRIEF,
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
                    xkb_file_type_to_string(type), safe_map_name(files[type]));
        }

        /* Missing components are initialized with defaults */
        const bool ok = compile_file_fns[type](files[type], keymap);
        if (!ok) {
            log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                    "Failed to compile %s\n",
                    xkb_file_type_to_string(type));
            return false;
        }
    }

    return UpdateDerivedKeymapFields(keymap);
}
