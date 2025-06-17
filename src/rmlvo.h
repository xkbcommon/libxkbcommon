/*
 * Copyright © 2025 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "config.h"

#include <stdbool.h>

#include "xkbcommon/xkbcommon.h"
#include "darray.h"

enum RMLVO {
    RMLVO_RULES = (1 << 0),
    RMLVO_MODEL = (1 << 1),
    RMLVO_LAYOUT = (1 << 2),
    RMLVO_VARIANT = (1 << 3),
    RMLVO_OPTIONS = (1 << 4)
};

struct xkb_rmlvo_builder_layout {
    char *layout;
    char *variant;
};

typedef darray(struct xkb_rmlvo_builder_layout) xkb_rmlvo_builder_layouts;

struct xkb_rmlvo_builder_option {
    char *option;
    xkb_layout_index_t layout;
};

typedef darray(struct xkb_rmlvo_builder_option) xkb_rmlvo_builder_options;

struct xkb_rmlvo_builder {
    char *rules;
    char *model;
    xkb_rmlvo_builder_layouts layouts;
    xkb_rmlvo_builder_options options;

    int refcnt;
    struct xkb_context *ctx;
};

bool
xkb_rmlvo_builder_to_rules_names(const struct xkb_rmlvo_builder *builder,
                                 struct xkb_rule_names *rmlvo,
                                 char *buf, size_t buf_size);
