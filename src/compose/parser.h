/*
 * Copyright © 2013 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "config.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "xkbcommon/xkbcommon.h"
#include "xkbcommon/xkbcommon-compose.h"

#include "src/utils.h"

XKB_EXPORT_PRIVATE char *
parse_string_literal(struct xkb_context *ctx, const char *string);

bool
parse_string(struct xkb_compose_table *table,
             const char *string, size_t len,
             const char *file_name);

bool
parse_file(struct xkb_compose_table *table,
           FILE *file, const char *file_name);
