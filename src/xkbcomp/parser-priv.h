/*
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 * SPDX-License-Identifier: HPND
 */
#pragma once

struct parser_param;
struct scanner;

#include "parser.h"

int
_xkbcommon_lex(YYSTYPE *yylval, struct scanner *scanner);

XkbFile *
parse(struct xkb_context *ctx, struct scanner *scanner, const char *map);

int
keyword_to_token(const char *string, size_t len);
