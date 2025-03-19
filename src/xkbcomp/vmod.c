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
InitVMods(struct xkb_mod_set_info *info, const struct xkb_mod_set *mods,
          bool reset)
{
    info->mods = *mods;
    info->set = 0;

    struct xkb_mod *mod;
    xkb_mod_index_t vmod = 0;
    xkb_mods_enumerate(vmod, mod, &info->mods) {
        if (reset)
            mod->mapping = 0;
        else if (mod->mapping)
            info->set |= UINT32_C(1) << vmod;
    }
    if (reset)
        info->mods.explicit_vmods = 0;
}

void
MergeModSets(struct xkb_context *ctx, struct xkb_mod_set_info *into,
             const struct xkb_mod_set_info *from, enum merge_mode merge) {
    const bool clobber = (merge != MERGE_AUGMENT);
    xkb_mod_index_t vmod;
    const struct xkb_mod *mod;
    assert(into->mods.num_mods <= from->mods.num_mods);
    xkb_mods_enumerate(vmod, mod, &from->mods) {
        const xkb_mod_mask_t mask = UINT32_C(1) << vmod;

        if ((mod->type != MOD_VIRT)) {
            /* No modifier in `from` or real modifier: nothing to do */
            assert((mod->type == 0 &&
                    mod->name == XKB_ATOM_NONE) ||
                   ((mod->type & MOD_REAL) &&
                    into->mods.mods[vmod].type == mod->type &&
                    mod->name != XKB_ATOM_NONE &&
                    into->mods.mods[vmod].name == mod->name));
            continue;
        } else if (into->mods.mods[vmod].type == 0) {
            /* No modifier in `into`: copy the whole modifier definition */
            assert(into->mods.mods[vmod].name == XKB_ATOM_NONE);
            assert(mod->type == MOD_VIRT);
            assert(mod->name != XKB_ATOM_NONE);
            assert(vmod >= into->mods.num_mods);
            into->mods.mods[vmod] = *mod;
            if (from->set & mask)
                into->set |= mask;
        } else {
            /* Modifier exists in both */
            assert(mod->type == MOD_VIRT);
            assert(mod->name != XKB_ATOM_NONE);
            assert(into->mods.mods[vmod].type == mod->type);
            assert(into->mods.mods[vmod].name == mod->name);

            if (!(from->set & mask)) {
                /* No redefinition */
                assert(mod->mapping == 0);
            } else if (!(into->set & mask)) {
                /* Implicit mapping in `into`: replace */
                assert(into->mods.mods[vmod].mapping == 0);
                into->mods.mods[vmod].mapping = mod->mapping;
                into->set |= mask;
            } else if (mod->mapping != into->mods.mods[vmod].mapping) {
                /* Handle conflicting mappings */
                /* Override or augment? */
                const bool clobber2 =
                    (clobber || (mod->mapping && into->mods.mods[vmod].mapping == 0));
                const xkb_mod_mask_t use = (clobber2)
                    ? mod->mapping
                    : into->mods.mods[vmod].mapping;
                const xkb_mod_mask_t ignore = (clobber2)
                    ? into->mods.mods[vmod].mapping
                    : mod->mapping;

                log_warn(ctx, XKB_LOG_MESSAGE_NO_ID,
                         "Virtual modifier %s defined multiple times; "
                         "Using %s, ignoring %s\n",
                         xkb_atom_text(ctx, mod->name),
                         ModMaskText(ctx, MOD_REAL, &from->mods, use),
                         ModMaskText(ctx, MOD_REAL, &from->mods, ignore));

                into->mods.mods[vmod].mapping = use;
                if (clobber2)
                    into->set |= mask;
            }
        }

        if (into->mods.mods[vmod].mapping) {
            /* Modifier is new or overriden */
            into->mods.explicit_vmods |= mask;
        } else {
            /* Modifier is unset or reset */
            into->mods.explicit_vmods &= ~mask;
        }
    }
    into->mods.num_mods = from->mods.num_mods;
}

bool
HandleVModDef(struct xkb_context *ctx, struct xkb_mod_set_info *mods,
              VModDef *stmt)
{
    xkb_mod_mask_t mapping = 0;
    if (stmt->value) {
        /*
         * This is a statement such as 'virtualModifiers NumLock = Mod1';
         * it sets the vmod-to-real-mod[s] mapping directly instead of going
         * through modifier_map or some such.
         */
        if (!ExprResolveModMask(ctx, stmt->value, MOD_REAL, &mods->mods, &mapping)) {
            log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                    "Declaration of %s ignored\n",
                    xkb_atom_text(ctx, stmt->name));
            return false;
        }
    }

    const bool clobber = (stmt->merge != MERGE_AUGMENT);
    xkb_mod_index_t vmod;
    struct xkb_mod *mod;
    xkb_mods_enumerate(vmod, mod, &mods->mods) {
        if (mod->name == stmt->name) {
            if (mod->type != MOD_VIRT) {
                log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                        "Can't add a virtual modifier named \"%s\"; "
                        "there is already a non-virtual modifier with this name! Ignored\n",
                        xkb_atom_text(ctx, mod->name));
                return false;
            }

            const xkb_mod_mask_t mask = UINT32_C(1) << vmod;

            if (mod->mapping == mapping || !stmt->value) {
                /*
                 * Same definition or no new explicit mapping: do nothing.
                 * Note that we must test the statement value and not the
                 * mapping in order to allow resetting it: e.g. `VMod=none`.
                 */
                if (stmt->value)
                    mods->set |= mask;
            } else {
                if (mods->set & mask) {
                    /* Handle conflicting mappings */
                    const bool clobber2 =
                        (clobber || (mapping && mod->mapping == 0));
                    const xkb_mod_mask_t use =
                        (clobber2 ? mapping : mod->mapping);
                    const xkb_mod_mask_t ignore =
                        (clobber2 ? mod->mapping : mapping);

                    log_warn(ctx, XKB_LOG_MESSAGE_NO_ID,
                             "Virtual modifier %s defined multiple times; "
                             "Using %s, ignoring %s\n",
                             xkb_atom_text(ctx, stmt->name),
                             ModMaskText(ctx, MOD_REAL, &mods->mods, use),
                             ModMaskText(ctx, MOD_REAL, &mods->mods, ignore));

                    mapping = use;
                    if (clobber2)
                        mods->set |= mask;
                } else {
                    mods->set |= mask;
                }

                mod->mapping = mapping;

                if (mapping) {
                    mods->mods.explicit_vmods |= mask;
                } else if (stmt->value) {
                    mods->mods.explicit_vmods &= ~mask;
                }
            }

            return true;
        }
    }

    if (mods->mods.num_mods >= XKB_MAX_MODS) {
        log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                "Cannot define virtual modifier %s: "
                "too many modifiers defined (maximum %d)\n",
                xkb_atom_text(ctx, stmt->name), XKB_MAX_MODS);
        return false;
    }

    mods->mods.mods[mods->mods.num_mods].name = stmt->name;
    mods->mods.mods[mods->mods.num_mods].type = MOD_VIRT;
    mods->mods.mods[mods->mods.num_mods].mapping = mapping;
    const xkb_mod_mask_t mask = UINT32_C(1) << mods->mods.num_mods;
    if (mapping)
        mods->mods.explicit_vmods |= mask;
    if (stmt->value)
        mods->set |= mask;
    mods->mods.num_mods++;
    return true;
}
