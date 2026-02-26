/*
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 * SPDX-License-Identifier: HPND
 */
#pragma once

#include "config.h"

#include "xkbcommon/xkbcommon.h"

#include "ast.h"
#include "keymap.h"
#include "text.h"
#include "xkbcomp-priv.h"

bool
ExprResolveLhs(struct xkb_context *ctx, const ExprDef *expr,
               const char **elem_rtrn, const char **field_rtrn,
               ExprDef **index_rtrn);

bool
ExprResolveModMask(struct xkb_context *ctx, const ExprDef *expr,
                   enum mod_type mod_type, const struct xkb_mod_set *mods,
                   xkb_mod_mask_t *mask_rtrn);

bool
ExprResolveMod(struct xkb_context *ctx, const ExprDef *def,
               enum mod_type mod_type, const struct xkb_mod_set *mods,
               xkb_mod_index_t *ndx_rtrn);

bool
ExprResolveBoolean(struct xkb_context *ctx, const ExprDef *expr,
                   bool *set_rtrn);

bool
ExprResolveInteger(struct xkb_context *ctx, const ExprDef *expr,
                   int64_t *val_rtrn);

bool
ExprResolveLevel(struct xkb_context *ctx, const ExprDef *expr,
                 xkb_level_index_t *level_rtrn);

enum xkb_parser_error
ExprResolveGroup(const struct xkb_keymap_info *keymap_info,
                 const ExprDef *expr, bool absolute,
                 xkb_layout_index_t *group_rtrn, bool *pending_rtrn);

bool
ExprResolveGroupMask(const struct xkb_keymap_info *keymap_info,
                     const ExprDef *expr, xkb_layout_mask_t *group_rtrn,
                     bool *pending_rtrn);

bool
ExprResolveButton(struct xkb_context *ctx, const ExprDef *expr,
                  int64_t *btn_rtrn);

bool
ExprResolveString(struct xkb_context *ctx, const ExprDef *expr,
                  xkb_atom_t *val_rtrn);

bool
ExprResolveEnum(struct xkb_context *ctx, const ExprDef *expr,
                uint32_t *val_rtrn, const LookupEntry *values);

bool
ExprResolveMask(struct xkb_context *ctx, const ExprDef *expr,
                uint32_t *mask_rtrn, const LookupEntry *values);
