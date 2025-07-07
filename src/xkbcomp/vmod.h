/*
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 * SPDX-License-Identifier: HPND
 */
#pragma once

#include "config.h"

#include "xkbcommon/xkbcommon.h"

#include <stdbool.h>

#include "ast.h"
#include "keymap.h"

void
InitVMods(struct xkb_mod_set *info, const struct xkb_mod_set *mods, bool reset);

void
MergeModSets(struct xkb_context *ctx, struct xkb_mod_set *into,
             const struct xkb_mod_set *from, enum merge_mode merge);

bool
HandleVModDef(struct xkb_context *ctx, struct xkb_mod_set *mods, VModDef *stmt);
