/*
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 * SPDX-License-Identifier: HPND
 */

#ifndef XKBCOMP_VMOD_H
#define XKBCOMP_VMOD_H

#include "ast.h"
#include "keymap.h"

bool
HandleVModDef(struct xkb_context *ctx, struct xkb_mod_set *mods,
              VModDef *stmt, enum merge_mode merge);

#endif
