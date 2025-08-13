/*
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 * SPDX-License-Identifier: HPND
 */
#pragma once

#include "config.h"

#include <stdbool.h>

struct parser_param;
struct scanner;

#include "scanner-utils.h"
#include "parser.h"

int
_xkbcommon_lex(YYSTYPE *yylval, struct scanner *scanner);

XkbFile *
parse(struct xkb_context *ctx, struct scanner *scanner, const char *map);

bool
parse_next(struct xkb_context *ctx, struct scanner *scanner, XkbFile **xkb_file);

int
keyword_to_token(const char *string, size_t len);
