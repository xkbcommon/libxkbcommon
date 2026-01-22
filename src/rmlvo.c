/*
 * Copyright © 2025 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <assert.h>
#include <string.h>

#include "xkbcommon/xkbcommon.h"
#include "context.h"
#include "keymap.h"
#include "messages-codes.h"
#include "rmlvo.h"
#include "darray.h"
#include "utils.h"
#include "xkbcomp/rules.h"

struct xkb_rmlvo_builder*
xkb_rmlvo_builder_new(struct xkb_context *context,
                      const char *rules, const char *model,
                      enum xkb_rmlvo_builder_flags flags)
{
    static const enum xkb_rmlvo_builder_flags XKB_RMLVO_BUILDER_FLAGS =
        XKB_RMLVO_BUILDER_NO_FLAGS;

    if (flags & ~XKB_RMLVO_BUILDER_FLAGS) {
        log_err(context, XKB_LOG_MESSAGE_NO_ID,
                "Unsupported RMLVO flags: 0x%x\n",
                (flags & ~XKB_RMLVO_BUILDER_FLAGS));
        return NULL;
    }

    struct xkb_rmlvo_builder * const builder = calloc(1, sizeof(*builder));
    if (!builder)
        goto error;

    builder->refcnt = 1;
    builder->ctx = xkb_context_ref(context);

    builder->rules = strdup_safe(rules);
    if (!builder->rules && rules)
        goto error;

    builder->model = strdup_safe(model);
    if (!builder->model && model)
        goto error;

    darray_init(builder->layouts);
    darray_init(builder->options);

    return builder;

error:
    log_err(context, XKB_ERROR_ALLOCATION_ERROR,
            "Cannot allocate a RMLVO builder.\n");
    xkb_rmlvo_builder_unref(builder);
    return NULL;
}

bool
xkb_rmlvo_builder_append_layout(struct xkb_rmlvo_builder *rmlvo,
                                const char *layout, const char *variant,
                                const char* const* options, size_t options_len)
{
    const xkb_layout_index_t idx = (xkb_layout_index_t)
                                   darray_size(rmlvo->layouts);

    if (idx >= XKB_MAX_GROUPS) {
        log_err(rmlvo->ctx, XKB_ERROR_UNSUPPORTED_GROUP_INDEX,
                "Maximum layout count reached: %u; "
                "cannot add layout \"%s(%s)\" to the RMLVO builder.\n",
                XKB_MAX_GROUPS, layout, (variant) ? variant : "");
        return false;
    }

    /* Append layout entry */
    const struct xkb_rmlvo_builder_layout new = {
        .layout = strdup_safe(layout),
        .variant = strdup_safe(variant)
    };

    if (!new.layout || (!new.variant && variant)) {
        free(new.layout);
        free(new.variant);
        log_err(rmlvo->ctx, XKB_ERROR_ALLOCATION_ERROR,
                "Cannot allocate layout \"%s(%s)\" to the RMLVO builder.\n",
                layout, (variant) ? variant : "");
        return false;
    }

    darray_append(rmlvo->layouts, new);

    if (!options)
        options_len = 0;

    /* Append layout-specific options entries */
    for (size_t k = 0; k < options_len; k++) {
        const struct xkb_rmlvo_builder_option option = {
            .option = strdup_safe(options[k]),
            .layout = idx
        };

        if (!option.option) {
            log_err(rmlvo->ctx, XKB_ERROR_ALLOCATION_ERROR,
                    "Cannot allocate option \"%s\" of layout \"%s(%s)\" "
                    "to the RMLVO builder.\n",
                    options[k], layout, (variant) ? variant : "");
            return false;
        }

        darray_append(rmlvo->options, option);
    }

    return true;
}

bool
xkb_rmlvo_builder_append_option(struct xkb_rmlvo_builder *rmlvo,
                                const char *option)
{
    if (!option)
        return false;

    /* Check for previous entry */
    const struct xkb_rmlvo_builder_option *prev;
    darray_foreach(prev, rmlvo->options) {
        if (prev->layout == XKB_LAYOUT_INVALID &&
            strcmp(prev->option, option) == 0)
            return true;
    }

    /* Append new entry */
    const struct xkb_rmlvo_builder_option new = {
        .option = strdup_safe(option),
        .layout = XKB_LAYOUT_INVALID
    };
    if (!new.option) {
        log_err(rmlvo->ctx, XKB_ERROR_ALLOCATION_ERROR,
                "Cannot allocate option \"%s\" to the RMLVO builder.\n",
                option);
        return false;
    }
    darray_append(rmlvo->options, new);
    return true;
}

struct xkb_rmlvo_builder *
xkb_rmlvo_builder_ref(struct xkb_rmlvo_builder *rmlvo)
{
    assert(rmlvo->refcnt > 0);
    rmlvo->refcnt++;
    return rmlvo;
}

void
xkb_rmlvo_builder_unref(struct xkb_rmlvo_builder *rmlvo)
{
    assert(!rmlvo || rmlvo->refcnt > 0);
    if (!rmlvo || --rmlvo->refcnt > 0)
        return;

    free(rmlvo->rules);
    free(rmlvo->model);

    const struct xkb_rmlvo_builder_layout *layout;
    darray_foreach(layout, rmlvo->layouts) {
        free(layout->layout);
        free(layout->variant);
    }
    darray_free(rmlvo->layouts);

    const struct xkb_rmlvo_builder_option *option;
    darray_foreach(option, rmlvo->options)
        free(option->option);
    darray_free(rmlvo->options);

    xkb_context_unref(rmlvo->ctx);
    free(rmlvo);
}

/* NOTE: the converse function, `xkb_rules_names_to_rmlvo_builder`, is currently
 * only used in tests */
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
    const struct xkb_rmlvo_builder_layout *layout;
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
    const struct xkb_rmlvo_builder_option *option;
    darray_enumerate(k, option, builder->options) {
        int count = snprintf(start, buf_size, "%s%s",
                             (k > 0 ? "," : ""), option->option);
        if (count < 0 || (size_t) count >= buf_size)
            return false;
        buf_size -= count;
        start += count;
        if (option->layout != XKB_LAYOUT_INVALID) {
            count = snprintf(start, buf_size, "%c%"PRIu32,
                             OPTIONS_GROUP_SPECIFIER_PREFIX, option->layout);
            if (count < 0 || (size_t) count >= buf_size)
                return false;
            buf_size -= count;
            start += count;
        }
    }
    if (buf_size == 0)
        return false;
    *start = '\0';
    return true;
}
