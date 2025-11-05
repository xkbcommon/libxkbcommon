/*
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 * SPDX-License-Identifier: HPND
 */
#pragma once

#include "config.h"

#include "ast.h"
#include "utils.h"

/* Reasonable threshold, with plenty of margin for keymaps in the wild */
#define INCLUDE_MAX_DEPTH 15

#define MERGE_OVERRIDE_PREFIX '+'
#define MERGE_AUGMENT_PREFIX  '|'
#define MERGE_REPLACE_PREFIX  '^'
#define MERGE_DEFAULT_PREFIX  MERGE_OVERRIDE_PREFIX
#define is_merge_mode_prefix(ch) \
    ((ch) == MERGE_OVERRIDE_PREFIX || (ch) == MERGE_AUGMENT_PREFIX || \
     (ch) == MERGE_REPLACE_PREFIX)
static const char MERGE_MODE_PREFIXES[] =
    { MERGE_OVERRIDE_PREFIX, MERGE_AUGMENT_PREFIX, MERGE_REPLACE_PREFIX, 0 };


bool
ParseIncludeMap(char **str_inout, char **file_rtrn, char **map_rtrn,
                char *nextop_rtrn, char **extra_data);

ssize_t
expand_path(struct xkb_context *ctx, const char* parent_file_name,
            const char *name, size_t name_len, enum xkb_file_type type,
            char *buf, size_t buf_size);

FILE *
FindFileInXkbPath(struct xkb_context *ctx, const char* parent_file_name,
                  const char *name, size_t name_len, enum xkb_file_type type,
                  char *buf, size_t buf_size, unsigned int *offset,
                  bool required);

bool
ExceedsIncludeMaxDepth(struct xkb_context *ctx, unsigned int include_depth);

XkbFile *
ProcessIncludeFile(struct xkb_context *ctx, const IncludeStmt *stmt,
                   enum xkb_file_type file_type, char *path, size_t path_size);
