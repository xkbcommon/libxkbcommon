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
    for (i = 0; i < darray_size(keymap->vmods); i++)
        info->defined |= (1 << i);
}

bool
HandleVModDef(VModDef *stmt, struct xkb_keymap *keymap, VModInfo *info)
{
    xkb_mod_index_t i;
    const struct xkb_vmod *vmod;
    struct xkb_vmod new;

    if (stmt->value)
        log_err(keymap->ctx,
                "Support for setting a value in a virtual_modifiers statement has been removed; "
                "Value ignored\n");

    darray_enumerate(i, vmod, keymap->vmods) {
        if (vmod->name == stmt->name) {
            info->available |= 1 << i;
            return true;
        }
    }

    if (darray_size(keymap->vmods) >= XKB_MAX_VIRTUAL_MODS) {
        log_err(keymap->ctx,
                "Too many virtual modifiers defined (maximum %d)\n",
                XKB_MAX_VIRTUAL_MODS);
        return false;
    }

    new.name = stmt->name;
    new.mapping = 0;
    darray_append(keymap->vmods, new);
    info->available |= (1 << (darray_size(keymap->vmods) - 1));
    return true;
}

static bool
LookupVModIndex(const struct xkb_keymap *keymap, xkb_atom_t field,
                enum expr_value_type type, xkb_mod_index_t *val_rtrn)
{
    const struct xkb_vmod *vmod;
    xkb_mod_index_t i;

    if (type != EXPR_TYPE_INT)
        return false;

    darray_enumerate(i, vmod, keymap->vmods) {
        if (vmod->name == field) {
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
    const struct xkb_vmod *vmod;
    xkb_mod_index_t i;
    xkb_atom_t name = def->value.str;

    if (def->op != EXPR_IDENT) {
        log_err(keymap->ctx,
                "Cannot resolve virtual modifier: "
                "found %s where a virtual modifier name was expected\n",
                expr_op_type_to_string(def->op));
        return false;
    }

    darray_enumerate(i, vmod, keymap->vmods) {
        if ((info->available & (1 << i)) && vmod->name == name) {
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
