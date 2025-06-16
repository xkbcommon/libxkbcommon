/*
 * Copyright © 2025 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <string.h>

#include "xkbcommon/xkbcommon.h"
#include "rmlvo.h"
#include "darray.h"
#include "utils.h"

struct xkb_rmlvo_builder*
xkb_rmlvo_builder_new(struct xkb_context *context,
                      const char *rules, const char *model,
                      enum xkb_rmlvo_builder_flags flags)
{
    struct xkb_rmlvo_builder * const builder = calloc(1, sizeof(*builder));
    if (!builder)
        return NULL;

    builder->refcnt = 1;
    builder->ctx = xkb_context_ref(context);

    char *new = strdup_safe(rules);
    if (!new && rules)
        goto error;
    builder->rules = new;

    new = strdup_safe(model);
    if (!new && model)
        goto error;
    builder->model = new;

    darray_init(builder->layouts);
    darray_init(builder->options);

    return builder;

error:
    xkb_rmlvo_builder_unref(builder);
    return NULL;
}

bool
xkb_rmlvo_builder_append_layout(struct xkb_rmlvo_builder *rmlvo,
                                const char *layout,
                                const char *variant)
{
    struct xkb_rmlvo_builder_layout new = {
        .layout = strdup_safe(layout),
        .variant = strdup_safe(variant)
    };

    if (!new.layout || (!new.variant && variant)) {
        free(new.layout);
        free(new.variant);
        return false;
    }

    darray_append(rmlvo->layouts, new);
    return true;
}

bool
xkb_rmlvo_builder_append_option(struct xkb_rmlvo_builder *rmlvo,
                                const char *option)
{
    if (!option)
        return false;

    /* Check for previous entry */
    char **prev;
    darray_foreach(prev, rmlvo->options) {
        if (strcmp(*prev, option) == 0)
            return true;
    }

    /* Append new entry */
    char *new = strdup_safe(option);
    if (!new)
        return false;
    darray_append(rmlvo->options, new);
    return true;
}

void
xkb_rmlvo_builder_unref(struct xkb_rmlvo_builder *rmlvo)
{
    if (!rmlvo || --rmlvo->refcnt > 0)
        return;

    free(rmlvo->rules);
    free(rmlvo->model);

    struct xkb_rmlvo_builder_layout *layout;
    darray_foreach(layout, rmlvo->layouts) {
        free(layout->layout);
        free(layout->variant);
    }
    darray_free(rmlvo->layouts);

    char **option;
    darray_foreach(option, rmlvo->options)
        free(*option);
    darray_free(rmlvo->options);

    xkb_context_unref(rmlvo->ctx);
    free(rmlvo);
}

bool
xkb_rmlvo_builder_to_rules_names(const struct xkb_rmlvo_builder *builder,
                                 struct xkb_rule_names *rmlvo,
                                 char *buf, size_t buf_size)
{
    rmlvo->rules = builder->rules;
    rmlvo->model = builder->model;

    char *start = buf;
    rmlvo->layout = start;
    darray_size_t k;
    struct xkb_rmlvo_builder_layout *layout;
    darray_enumerate(k, layout, builder->layouts) {
        int count = snprintf(start, buf_size, "%s%s",
                             (k > 0 ? "," : ""), layout->layout);
        if (count < 0 || (size_t) count >= buf_size)
            return false;
        buf_size -= count;
        start += count;
    }
    if (buf_size <= 1)
        return false;
    *start = '\0';
    start++;
    buf_size--;

    rmlvo->variant = start;
    darray_enumerate(k, layout, builder->layouts) {
        int count = snprintf(start, buf_size, "%s%s",
                             (k > 0 ? "," : ""),
                             (layout->variant ? layout->variant : ""));
        if (count < 0 || (size_t) count >= buf_size)
            return false;
        buf_size -= count;
        start += count;
    }
    if (buf_size <= 1)
        return false;
    *start = '\0';
    start++;
    buf_size--;

    rmlvo->options = start;
    char **option;
    darray_enumerate(k, option, builder->options) {
        int count = snprintf(start, buf_size, "%s%s",
                             (k > 0 ? "," : ""), *option);
        if (count < 0 || (size_t) count >= buf_size)
            return false;
        buf_size -= count;
        start += count;
    }
    if (buf_size == 0)
        return false;
    *start = '\0';
    return true;
}
