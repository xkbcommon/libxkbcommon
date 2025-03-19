/*
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 * SPDX-License-Identifier: HPND
 */
#pragma once

#include "xkbcommon/xkbcommon.h"

#include <stdbool.h>

#include "ast.h"
#include "keymap.h"

struct xkb_mod_set_info {
    struct xkb_mod_set mods;
    /* Modifiers set. Required to handle case where modifier mapping is reset */
    xkb_mod_mask_t set;
};

void
InitVMods(struct xkb_mod_set_info *info, const struct xkb_mod_set *mods,
          bool reset);

void
MergeModSets(struct xkb_context *ctx, struct xkb_mod_set_info *into,
             const struct xkb_mod_set_info *from, enum merge_mode merge);

bool
HandleVModDef(struct xkb_context *ctx, struct xkb_mod_set_info *mods,
              VModDef *stmt);
