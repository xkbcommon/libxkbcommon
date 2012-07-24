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

#ifndef EXPR_H
#define EXPR_H 1

#include "xkbcomp-priv.h"

typedef union _ExprResult {
    int ival;
    unsigned uval;
} ExprResult;

typedef struct _LookupEntry {
    const char *name;
    unsigned result;
} LookupEntry;

extern const char *
exprOpText(enum expr_op_type op);

bool
ExprResolveLhs(struct xkb_context *ctx, ExprDef *expr, const char **elem_rtrn,
               const char **field_rtrn, ExprDef **index_rtrn);

extern bool
LookupModMask(struct xkb_context *ctx, const void *priv, xkb_atom_t field,
              enum expr_value_type type, ExprResult *val_rtrn);

extern bool
LookupVModMask(struct xkb_context *ctx, const void *priv, xkb_atom_t field,
               enum expr_value_type type, ExprResult *val_rtrn);

extern bool
LookupModIndex(struct xkb_context *ctx, const void *priv, xkb_atom_t field,
               enum expr_value_type type, ExprResult *val_rtrn);

extern int
ExprResolveModMask(struct xkb_context *ctx, ExprDef *expr,
                   ExprResult *val_rtrn);

extern int
ExprResolveVModMask(struct xkb_keymap *keymap, ExprDef *expr,
                    ExprResult *val_rtrn);

bool
ExprResolveBoolean(struct xkb_context *ctx, ExprDef *expr, bool *set_rtrn);

bool
ExprResolveKeyCode(struct xkb_context *ctx, ExprDef *expr, xkb_keycode_t *kc);

bool
ExprResolveInteger(struct xkb_context *ctx, ExprDef *expr, int *val_rtrn);

bool
ExprResolveLevel(struct xkb_context *ctx, ExprDef *expr,
                 unsigned int *level_rtrn);

bool
ExprResolveGroup(struct xkb_context *ctx, ExprDef *expr,
                 xkb_group_index_t *group_rtrn);

bool
ExprResolveButton(struct xkb_context *ctx, ExprDef *expr, int *btn_rtrn);

bool
ExprResolveString(struct xkb_context *ctx, ExprDef *expr,
                  const char **val_rtrn);

bool
ExprResolveKeyName(struct xkb_context *ctx, ExprDef *expr,
                   char name[XkbKeyNameLength]);

extern int
ExprResolveEnum(struct xkb_context *ctx, ExprDef *expr, ExprResult *val_rtrn,
                const LookupEntry *values);

extern int
ExprResolveMask(struct xkb_context *ctx, ExprDef *expr, ExprResult *val_rtrn,
                const LookupEntry *values);

bool
ExprResolveKeySym(struct xkb_context *ctx, ExprDef *expr,
                  xkb_keysym_t *sym_rtrn);

#endif /* EXPR_H */
