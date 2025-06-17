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
HandleActionDef(struct xkb_context *ctx, enum xkb_keymap_format format,
                ActionsInfo *info, const struct xkb_mod_set *mods, ExprDef *def,
                union xkb_action *action);

bool
SetDefaultActionField(struct xkb_context *ctx, enum xkb_keymap_format format,
                      ActionsInfo *info, struct xkb_mod_set *mods,
                      const char *elem, const char *field, ExprDef *array_ndx,
                      ExprDef *value, enum merge_mode merge);

static inline bool
isModsUnLockOnPressSupported(enum xkb_keymap_format format)
{
    /* Lax bound */
    return format >= XKB_KEYMAP_FORMAT_TEXT_V2;
}

static inline bool
isGroupLockOnReleaseSupported(enum xkb_keymap_format format)
{
    /* Lax bound */
    return format >= XKB_KEYMAP_FORMAT_TEXT_V2;
}

static inline bool
isModsLatchOnPressSupported(enum xkb_keymap_format format)
{
    /* Lax bound */
    return format >= XKB_KEYMAP_FORMAT_TEXT_V2;
}
