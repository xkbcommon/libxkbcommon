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

#include "vmod.h"

void
InitVModInfo(VModInfo *info, struct xkb_keymap *keymap)
{
    ClearVModInfo(info, keymap);
}

void
ClearVModInfo(VModInfo *info, struct xkb_keymap *keymap)
{
    xkb_mod_index_t i;
    xkb_mod_mask_t bit;

    info->defined = info->available = 0;

    for (i = 0; i < XkbNumVirtualMods; i++)
        keymap->vmods[i] = XkbNoModifierMask;

    for (i = 0, bit = 1; i < XkbNumVirtualMods; i++, bit <<= 1)
        if (keymap->vmod_names[i])
            info->defined |= bit;
}

/***====================================================================***/

/**
 * Handle one entry in the virtualModifiers line (e.g. NumLock).
 * If the entry is e.g. NumLock=Mod1, stmt->value is not NULL, and the
 * XkbServerMap's vmod is set to the given modifier. Otherwise, the vmod is 0.
 *
 * @param stmt The statement specifying the name and (if any the value).
 * @param mergeMode Merge strategy (e.g. MERGE_OVERRIDE)
 */
bool
HandleVModDef(VModDef *stmt, struct xkb_keymap *keymap,
              enum merge_mode mergeMode, VModInfo *info)
{
    xkb_mod_index_t i;
    int nextFree;
    xkb_mod_mask_t bit;
    xkb_mod_mask_t mask;

    nextFree = -1;
    for (i = 0, bit = 1; i < XkbNumVirtualMods; i++, bit <<= 1) {
        const char *str1;
        const char *str2 = "";

        if (!(info->defined & bit)) {
            if (nextFree < 0)
                nextFree = i;
            continue;
        }

        /* already defined */
        if (!keymap->vmod_names[i])
            continue;

        if (!streq(keymap->vmod_names[i],
                   xkb_atom_text(keymap->ctx, stmt->name)))
            continue;

        info->available |= bit;

        if (!stmt->value)
            return true;

        if (!ExprResolveModMask(keymap->ctx, stmt->value, &mask)) {
            log_err(keymap->ctx, "Declaration of %s ignored\n",
                    xkb_atom_text(keymap->ctx, stmt->name));
            return false;
        }

        if (mask == keymap->vmods[i])
            return true;

        str1 = ModMaskText(keymap->vmods[i], true);
        if (mergeMode == MERGE_OVERRIDE) {
            str2 = str1;
            str1 = ModMaskText(mask, true);
        }

        log_warn(keymap->ctx,
                 "Virtual modifier %s defined multiple times; "
                 "Using %s, ignoring %s\n",
                 xkb_atom_text(keymap->ctx, stmt->name), str1, str2);

        if (mergeMode == MERGE_OVERRIDE)
            keymap->vmods[i] = mask;

        return true;
    }

    if (nextFree < 0) {
        log_err(keymap->ctx,
                "Too many virtual modifiers defined (maximum %d)\n",
                XkbNumVirtualMods);
        return false;
    }

    info->defined |= (1 << nextFree);
    info->available |= (1 << nextFree);

    keymap->vmod_names[nextFree] = xkb_atom_text(keymap->ctx, stmt->name);

    if (!stmt->value)
        return true;

    if (!ExprResolveModMask(keymap->ctx, stmt->value, &mask)) {
        log_err(keymap->ctx, "Declaration of %s ignored\n",
                xkb_atom_text(keymap->ctx, stmt->name));
        return false;
    }

    keymap->vmods[nextFree] = mask;
    return true;
}

/**
 * Returns the index of the given modifier in the keymap->vmod_names array.
 *
 * @param keymap Pointer to the xkb data structure.
 * @param field The Atom of the modifier's name (e.g. Atom for LAlt)
 * @param type Must be EXPR_TYPE_INT, otherwise return false.
 * @param val_rtrn Set to the index of the modifier that matches.
 *
 * @return true on success, false otherwise. If false is returned, val_rtrn is
 * undefined.
 */
static bool
LookupVModIndex(const struct xkb_keymap *keymap, xkb_atom_t field,
                enum expr_value_type type, xkb_mod_index_t *val_rtrn)
{
    xkb_mod_index_t i;
    const char *name = xkb_atom_text(keymap->ctx, field);

    if (type != EXPR_TYPE_INT)
        return false;

    /* For each named modifier, get the name and compare it to the one passed
     * in. If we get a match, return the index of the modifier.
     * The order of modifiers is the same as in the virtual_modifiers line in
     * the xkb_types section.
     */
    for (i = 0; i < XkbNumVirtualMods; i++) {
        if (keymap->vmod_names[i] && streq(keymap->vmod_names[i], name)) {
            *val_rtrn = i;
            return true;
        }
    }

    return false;
}

/**
 * Get the mask for the given (virtual or core) modifier and set
 * val_rtrn.uval to the mask value.
 *
 * @param priv Pointer to xkb data structure.
 * @param val_rtrn Member uval is set to the mask returned.
 *
 * @return true on success, false otherwise. If false is returned, val_rtrn is
 * undefined.
 */
bool
LookupVModMask(struct xkb_context *ctx, const void *priv, xkb_atom_t field,
               enum expr_value_type type, xkb_mod_mask_t *val_rtrn)
{
    xkb_mod_index_t ndx;

    if (LookupModMask(ctx, NULL, field, type, val_rtrn)) {
        return true;
    }
    else if (LookupVModIndex(priv, field, type, &ndx)) {
        *val_rtrn = (1 << (XkbNumModifiers + ndx));
        return true;
    }

    return false;
}

bool
ResolveVirtualModifier(ExprDef *def, struct xkb_keymap *keymap,
                       xkb_mod_index_t *ndx_rtrn, VModInfo *info)
{
    int val;

    if (def->op == EXPR_IDENT) {
        xkb_mod_index_t i;
        xkb_mod_mask_t bit;
        const char *name = xkb_atom_text(keymap->ctx, def->value.str);

        for (i = 0, bit = 1; i < XkbNumVirtualMods; i++, bit <<= 1) {
            if ((info->available & bit) && keymap->vmod_names[i] &&
                streq(keymap->vmod_names[i], name)) {
                *ndx_rtrn = i;
                return true;
            }
        }
    }

    if (!ExprResolveInteger(keymap->ctx, def, &val))
        return false;

    if (val < 0 || val >= XkbNumVirtualMods) {
        log_err(keymap->ctx,
                "Illegal virtual modifier %d (must be 0..%d inclusive)\n",
                val, XkbNumVirtualMods - 1);
        return false;
    }

    *ndx_rtrn = (xkb_mod_index_t) val;
    return true;
}
