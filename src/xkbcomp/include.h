/*
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 * SPDX-License-Identifier: HPND
 */

#ifndef XKBCOMP_INCLUDE_H
#define XKBCOMP_INCLUDE_H

#include "ast.h"

/* Reasonable threshold, with plenty of margin for keymaps in the wild */
#define INCLUDE_MAX_DEPTH 15

#define MERGE_OVERRIDE_PREFIX '+'
#define MERGE_AUGMENT_PREFIX  '|'
#define MERGE_DEFAULT_PREFIX  MERGE_OVERRIDE_PREFIX
#define is_merge_mode_prefix(ch) \
    ((ch) == MERGE_OVERRIDE_PREFIX || (ch) == MERGE_AUGMENT_PREFIX)

bool
ParseIncludeMap(char **str_inout, char **file_rtrn, char **map_rtrn,
                char *nextop_rtrn, char **extra_data);

FILE *
FindFileInXkbPath(struct xkb_context *ctx, const char *name,
                  enum xkb_file_type type, char **pathRtrn,
                  unsigned int *offset);

bool
ExceedsIncludeMaxDepth(struct xkb_context *ctx, unsigned int include_depth);

XkbFile *
ProcessIncludeFile(struct xkb_context *ctx, IncludeStmt *stmt,
                   enum xkb_file_type file_type);

#endif
