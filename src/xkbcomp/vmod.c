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
    xkb_mod_index_t i;

    memset(info, 0, sizeof(*info));
    for (i = 0; i < XKB_NUM_VIRTUAL_MODS; i++)
        if (keymap->vmod_names[i])
            info->defined |= (1 << i);
}

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
    for (i = 0, bit = 1; i < XKB_NUM_VIRTUAL_MODS; i++, bit <<= 1) {
        if (!(info->defined & bit)) {
            if (nextFree < 0)
                nextFree = i;
            continue;
        }

        /* Already defined. */
        if (!keymap->vmod_names[i])
            continue;

        if (keymap->vmod_names[i] != stmt->name)
            continue;

        info->available |= bit;
        return true;
    }

    if (nextFree < 0) {
        log_err(keymap->ctx,
                "Too many virtual modifiers defined (maximum %d)\n",
                XKB_NUM_VIRTUAL_MODS);
        return false;
    }

    info->defined |= (1 << nextFree);
    info->available |= (1 << nextFree);

    keymap->vmod_names[nextFree] = stmt->name;
    return true;
}

static bool
LookupVModIndex(const struct xkb_keymap *keymap, xkb_atom_t field,
                enum expr_value_type type, xkb_mod_index_t *val_rtrn)
{
    xkb_mod_index_t i;

    if (type != EXPR_TYPE_INT)
        return false;

    for (i = 0; i < XKB_NUM_VIRTUAL_MODS; i++) {
        if (keymap->vmod_names[i] == field) {
            *val_rtrn = i;
            return true;
        }
    }

    return false;
}

bool
LookupVModMask(struct xkb_context *ctx, const void *priv, xkb_atom_t field,
               enum expr_value_type type, xkb_mod_mask_t *val_rtrn)
{
    xkb_mod_index_t ndx;

    if (LookupModMask(ctx, NULL, field, type, val_rtrn)) {
        return true;
    }
    else if (LookupVModIndex(priv, field, type, &ndx)) {
        *val_rtrn = (1 << (XKB_NUM_CORE_MODS + ndx));
        return true;
    }

    return false;
}

bool
ResolveVirtualModifier(ExprDef *def, struct xkb_keymap *keymap,
                       xkb_mod_index_t *ndx_rtrn, VModInfo *info)
{
    xkb_mod_index_t i;
    xkb_atom_t name = def->value.str;

    if (def->op != EXPR_IDENT) {
        log_err(keymap->ctx,
                "Cannot resolve virtual modifier: "
                "found %s where a virtual modifier name was expected\n",
                expr_op_type_to_string(def->op));
        return false;
    }

    for (i = 0; i < XKB_NUM_VIRTUAL_MODS; i++) {
        if ((info->available & (1 << i)) && keymap->vmod_names[i] == name) {
            *ndx_rtrn = i;
            return true;
        }
    }

    log_err(keymap->ctx,
            "Cannot resolve virtual modifier: "
            "\"%s\" was not previously declared\n",
            xkb_atom_text(keymap->ctx, name));
    return false;
}
