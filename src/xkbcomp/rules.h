/*
 * Copyright © 2009 Dan Nicholson
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <stdbool.h>

#include "xkbcomp-priv.h"

XKB_EXPORT_PRIVATE bool
xkb_components_from_rules(struct xkb_context *ctx,
                          const struct xkb_rule_names *rmlvo,
                          struct xkb_component_names *out,
                          xkb_layout_index_t *explicit_layouts);

/* Maximum length of a layout index string:
 *
 * length = ceiling (bitsize(xkb_layout_index_t) * logBase 10 2)
 *        < ceiling (bitsize(xkb_layout_index_t) * 5 / 16)
 *        < 1 + floor (bitsize(xkb_layout_index_t) * 5 / 16)
 */
#define MAX_LAYOUT_INDEX_STR_LENGTH \
        (1 + ((sizeof(xkb_layout_index_t) * CHAR_BIT * 5) >> 4))
