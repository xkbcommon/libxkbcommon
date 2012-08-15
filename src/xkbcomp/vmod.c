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

#include "xkbcomp-priv.h"
#include "text.h"
#include "expr.h"
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
 *
 * @param stmt The statement specifying the name
 * @param mergeMode Merge strategy (e.g. MERGE_OVERRIDE)
 */
bool
HandleVModDef(VModDef *stmt, struct xkb_keymap *keymap,
              enum merge_mode mergeMode, VModInfo *info)
{
    xkb_mod_index_t i;
    int nextFree;
    xkb_mod_mask_t bit;

    if (stmt->value)
        log_err(keymap->ctx,
                "Support for setting a value in a virtual_modifiers statement has been removed; "
                "Value ignored\n");

    nextFree = -1;
    for (i = 0, bit = 1; i < XkbNumVirtualMods; i++, bit <<= 1) {
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
    xkb_mod_index_t i;
    const char *name;

    if (def->op != EXPR_IDENT) {
        log_err(keymap->ctx,
                "Cannot resolve virtual modifier: "
                "found %s where a virtual modifier name was expected\n",
                expr_op_type_to_string(def->op));
        return false;
    }

    name = xkb_atom_text(keymap->ctx, def->value.str);

    for (i = 0; i < XkbNumVirtualMods; i++) {
        if ((info->available & (1 << i)) &&
            streq_not_null(keymap->vmod_names[i], name)) {
            *ndx_rtrn = i;
            return true;
        }
    }

    log_err(keymap->ctx,
            "Cannot resolve virtual modifier: "
            "\"%s\" was not previously declared\n", name);
    return false;
}
