/*
 * Copyright © 2014 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "xkbcommon/xkbcommon.h"

char *
resolve_locale(struct xkb_context *ctx, const char *locale);

const char *
get_xlocaledir_path(struct xkb_context *ctx);

char *
get_xcomposefile_path(struct xkb_context *ctx);

char *
get_xdg_xcompose_file_path(struct xkb_context *ctx);

char *
get_home_xcompose_file_path(struct xkb_context *ctx);

char *
get_locale_compose_file_path(struct xkb_context *ctx, const char *locale);
