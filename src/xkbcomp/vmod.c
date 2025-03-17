/*
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 * SPDX-License-Identifier: HPND
 */

#include "config.h"

#include "xkbcomp-priv.h"
#include "text.h"
#include "expr.h"
#include "vmod.h"

bool
HandleVModDef(struct xkb_context *ctx, struct xkb_mod_set *mods,
              VModDef *stmt, enum merge_mode merge)
{
    xkb_mod_mask_t mapping = 0;
    if (stmt->value) {
        /*
         * This is a statement such as 'virtualModifiers NumLock = Mod1';
         * it sets the vmod-to-real-mod[s] mapping directly instead of going
         * through modifier_map or some such.
         */
        if (!ExprResolveModMask(ctx, stmt->value, MOD_REAL, mods, &mapping)) {
            log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                    "Declaration of %s ignored\n",
                    xkb_atom_text(ctx, stmt->name));
            return false;
        }
    }

    merge = (merge == MERGE_DEFAULT ? stmt->merge : merge);
    xkb_mod_index_t i;
    struct xkb_mod *mod;
    xkb_mods_enumerate(i, mod, mods) {
        if (mod->name == stmt->name) {
            if (mod->type != MOD_VIRT) {
                log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                        "Can't add a virtual modifier named \"%s\"; "
                        "there is already a non-virtual modifier with this name! Ignored\n",
                        xkb_atom_text(ctx, mod->name));
                return false;
            }

            if (mod->mapping == mapping || !stmt->value) {
                /*
                 * Same definition or no new explicit mapping: do nothing.
                 * Note that we must test the statement value and not the mapping
                 * in order to allow resetting it: e.g. `VMod=none`.
                 */
                return true;
            }

            const xkb_mod_mask_t vmod = UINT32_C(1) << i;
            if (mods->explicit_vmods & vmod) {
                /* Handle conflicting mappings */
                assert(mod->mapping != 0);
                const bool clobber = (merge != MERGE_AUGMENT);
                const xkb_mod_mask_t use = (clobber ? mapping : mod->mapping);
                const xkb_mod_mask_t ignore = (clobber ? mod->mapping : mapping);

                log_warn(ctx, XKB_LOG_MESSAGE_NO_ID,
                         "Virtual modifier %s defined multiple times; "
                         "Using %s, ignoring %s\n",
                         xkb_atom_text(ctx, stmt->name),
                         ModMaskText(ctx, MOD_REAL, mods, use),
                         ModMaskText(ctx, MOD_REAL, mods, ignore));

                mapping = use;
            }

            mod->mapping = mapping;
            if (mapping) {
                mods->explicit_vmods |= vmod;
            } else {
                mods->explicit_vmods &= ~vmod;
            }

            return true;
        }
    }

    if (mods->num_mods >= XKB_MAX_MODS) {
        log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                "Too many modifiers defined (maximum %d)\n",
                XKB_MAX_MODS);
        return false;
    }

    mods->mods[mods->num_mods].name = stmt->name;
    mods->mods[mods->num_mods].type = MOD_VIRT;
    mods->mods[mods->num_mods].mapping = mapping;
    if (mapping)
        mods->explicit_vmods |= UINT32_C(1) << mods->num_mods;
    mods->num_mods++;
    return true;
}
