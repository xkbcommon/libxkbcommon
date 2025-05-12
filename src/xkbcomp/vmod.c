/*
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 * SPDX-License-Identifier: HPND
 */

#include "atom.h"
#include "config.h"

#include <stdint.h>

#include "xkbcommon/xkbcommon.h"
#include "keymap.h"
#include "text.h"
#include "expr.h"
#include "xkbcomp/ast.h"
#include "vmod.h"

void
InitVMods(struct xkb_mod_set *info, const struct xkb_mod_set *mods, bool reset)
{
    *info = *mods;

    if (!reset)
        return;

    struct xkb_mod *mod;
    xkb_mod_index_t vmod = 0;
    xkb_mods_enumerate(vmod, mod, info) {
        mod->mapping = 0;
    }
    info->explicit_vmods = 0;
}

void
MergeModSets(struct xkb_context *ctx, struct xkb_mod_set *into,
             const struct xkb_mod_set *from, enum merge_mode merge) {
    const bool clobber = (merge != MERGE_AUGMENT);
    xkb_mod_index_t vmod;
    const struct xkb_mod *mod;
    assert(into->num_mods <= from->num_mods);
    xkb_mods_enumerate(vmod, mod, from) {
        const xkb_mod_mask_t mask = UINT32_C(1) << vmod;

        if ((mod->type != MOD_VIRT)) {
            /* No modifier in `from` or real modifier: nothing to do */
            assert((mod->type == 0 &&
                    mod->name == XKB_ATOM_NONE) ||
                   ((mod->type & MOD_REAL) &&
                    into->mods[vmod].type == mod->type &&
                    mod->name != XKB_ATOM_NONE &&
                    into->mods[vmod].name == mod->name));
            continue;
        } else if (into->mods[vmod].type == 0) {
            /* No modifier in `into`: copy the whole modifier definition */
            assert(into->mods[vmod].name == XKB_ATOM_NONE);
            assert(mod->type == MOD_VIRT);
            assert(mod->name != XKB_ATOM_NONE);
            assert(vmod >= into->num_mods);
            into->mods[vmod] = *mod;
            if (from->explicit_vmods & mask)
                into->explicit_vmods |= mask;
        } else {
            /* Modifier exists in both */
            assert(mod->type == MOD_VIRT);
            assert(mod->name != XKB_ATOM_NONE);
            assert(into->mods[vmod].type == mod->type);
            assert(into->mods[vmod].name == mod->name);

            if (!(from->explicit_vmods & mask)) {
                /* Implicit mapping in `from`: do nothing */
                assert(mod->mapping == 0);
            } else if (!(into->explicit_vmods & mask)) {
                /* Implicit mapping in `into`: replace */
                assert(into->mods[vmod].mapping == 0);
                into->mods[vmod].mapping = mod->mapping;
                into->explicit_vmods |= mask;
            } else if (mod->mapping != into->mods[vmod].mapping) {
                /* Handle conflicting mappings */
                const xkb_mod_mask_t use = (clobber)
                    ? mod->mapping
                    : into->mods[vmod].mapping;
                const xkb_mod_mask_t ignore = (clobber)
                    ? into->mods[vmod].mapping
                    : mod->mapping;

                log_warn(ctx, XKB_LOG_MESSAGE_NO_ID,
                         "Virtual modifier %s mapping defined multiple times; "
                         "Using %s, ignoring %s\n",
                         xkb_atom_text(ctx, mod->name),
                         ModMaskText(ctx, MOD_REAL, from, use),
                         ModMaskText(ctx, MOD_REAL, from, ignore));

                into->mods[vmod].mapping = use;
            }
        }
    }
    into->num_mods = from->num_mods;
}

bool
HandleVModDef(struct xkb_context *ctx, struct xkb_mod_set *mods, VModDef *stmt)
{
    xkb_mod_mask_t mapping = 0;
    if (stmt->value) {
        /*
         * This is a statement such as 'virtualModifiers NumLock = Mod1';
         * it initialize the vmod-to-real-mod[s] mapping before going
         * through modifier_map.
         */
        if (!ExprResolveModMask(ctx, stmt->value, MOD_REAL, mods, &mapping)) {
            log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                    "Declaration of %s ignored\n",
                    xkb_atom_text(ctx, stmt->name));
            return false;
        }
    }

    xkb_mod_index_t vmod;
    struct xkb_mod *mod;
    xkb_mods_enumerate(vmod, mod, mods) {
        if (mod->name == stmt->name) {
            if (mod->type != MOD_VIRT) {
                log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                        "Can't add a virtual modifier named \"%s\"; there is "
                        "already a non-virtual modifier with this name! "
                        "Ignored\n",
                        xkb_atom_text(ctx, mod->name));
                return false;
            }

            const xkb_mod_mask_t mask = UINT32_C(1) << vmod;

            if (!stmt->value) {
                /* No new explicit mapping: do nothing */
                return true;
            } else if (!(mods->explicit_vmods & mask)) {
                /* No previous explicit mapping: add mapping */
                mod->mapping = mapping;
            } else if (mod->mapping != mapping) {
                /* Handle conflicting mappings */
                const bool clobber = (stmt->merge != MERGE_AUGMENT);
                const xkb_mod_mask_t use =
                    (clobber ? mapping : mod->mapping);
                const xkb_mod_mask_t ignore =
                    (clobber ? mod->mapping : mapping);

                log_warn(ctx, XKB_LOG_MESSAGE_NO_ID,
                         "Virtual modifier %s mapping defined multiple times; "
                         "Using %s, ignoring %s\n",
                         xkb_atom_text(ctx, stmt->name),
                         ModMaskText(ctx, MOD_REAL, mods, use),
                         ModMaskText(ctx, MOD_REAL, mods, ignore));

                mod->mapping = use;
            }

            mods->explicit_vmods |= mask;

            return true;
        }
    }

    if (mods->num_mods >= XKB_MAX_MODS) {
        log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                "Cannot define virtual modifier %s: "
                "too many modifiers defined (maximum %u)\n",
                xkb_atom_text(ctx, stmt->name), XKB_MAX_MODS);
        return false;
    }

    mods->mods[mods->num_mods].name = stmt->name;
    mods->mods[mods->num_mods].type = MOD_VIRT;
    mods->mods[mods->num_mods].mapping = mapping;
    if (stmt->value) {
        const xkb_mod_mask_t mask = UINT32_C(1) << mods->num_mods;
        mods->explicit_vmods |= mask;
    }
    mods->num_mods++;
    return true;
}
