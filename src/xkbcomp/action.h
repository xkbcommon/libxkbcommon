/*
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 * SPDX-License-Identifier: HPND
 */
#pragma once

#include "ast.h"
#include "keymap.h"

/*
 * This struct contains the default values which every new action
 * (e.g. in an interpret statement) starts off with. It can be
 * modified within the files (see calls to SetDefaultActionField).
 */
typedef struct {
    union xkb_action actions[_ACTION_TYPE_NUM_ENTRIES];
} ActionsInfo;

void
InitActionsInfo(ActionsInfo *info);

bool
HandleActionDef(struct xkb_context *ctx, ActionsInfo *info,
                const struct xkb_mod_set *mods, ExprDef *def,
                union xkb_action *action);

bool
SetDefaultActionField(struct xkb_context *ctx, ActionsInfo *info,
                      struct xkb_mod_set *mods, const char *elem,
                      const char *field, ExprDef *array_ndx,
                      ExprDef *value, enum merge_mode merge);
