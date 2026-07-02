/*
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"
#include "test-config.h"

#include "context.h"

static const char *
log_level_to_string(enum xkb_log_level level)
{
    switch (level) {
    case XKB_LOG_LEVEL_CRITICAL:
        return "critical";
    case XKB_LOG_LEVEL_ERROR:
        return "error";
    case XKB_LOG_LEVEL_WARNING:
        return "warning";
    case XKB_LOG_LEVEL_INFO:
        return "info";
    case XKB_LOG_LEVEL_DEBUG:
        return "debug";
    }

    return "unknown";
}

ATTR_PRINTF(3, 0) static void
log_fn(struct xkb_context *ctx, enum xkb_log_level level,
       const char *fmt, va_list args)
{
    char *s;
    int size;
    darray_char *ls = xkb_context_get_user_data(ctx);
    assert(ls);

    size = vasprintf(&s, fmt, args);
    assert(size != -1);

    darray_append_string(*ls, log_level_to_string(level));
    darray_append_lit(*ls, ": ");
    darray_append_string(*ls, s);
    free(s);
}
