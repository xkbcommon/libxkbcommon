/*
 * Copyright © 2009 Dan Nicholson
 * SPDX-License-Identifier: MIT
 */

#ifndef XKBCOMP_RULES_H
#define XKBCOMP_RULES_H

#include <stdbool.h>

#include "xkbcomp-priv.h"

bool
xkb_components_from_rules(struct xkb_context *ctx,
                          const struct xkb_rule_names *rmlvo,
                          struct xkb_component_names *out,
                          xkb_layout_index_t *explicit_layouts);

/* Maximum length of a layout index string:
 * [NOTE] Currently XKB_MAX_GROUPS is 4, but the following code is
 * future-proof for all possible indexes.
 *
 * length = ceiling (bitsize(xkb_layout_index_t) * logBase 10 2)
 *        < ceiling (bitsize(xkb_layout_index_t) * 5 / 16)
 *        < 1 + floor (bitsize(xkb_layout_index_t) * 5 / 16)
 */
#define MAX_LAYOUT_INDEX_STR_LENGTH \
        (1 + ((sizeof(xkb_layout_index_t) * CHAR_BIT * 5) >> 4))

#endif
