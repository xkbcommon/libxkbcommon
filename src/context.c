/*
 * Copyright © 2012 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>

#include "xkb-priv.h"
#include "atom.h"

struct xkb_context {
    int refcnt;

    ATTR_PRINTF(3, 0) void (*log_fn)(struct xkb_context *ctx, int priority,
                                     const char *fmt, va_list args);
    int log_priority;
    int log_verbosity;
    void *user_data;

    darray(char *) includes;

    /* xkbcomp needs to assign sequential IDs to XkbFile's it creates. */
    unsigned file_id;

    struct atom_table *atom_table;
};

/**
 * Append one directory to the context's include path.
 */
XKB_EXPORT int
xkb_context_include_path_append(struct xkb_context *ctx, const char *path)
{
    struct stat stat_buf;
    int err;
    char *tmp;

    err = stat(path, &stat_buf);
    if (err != 0)
        return 0;
    if (!S_ISDIR(stat_buf.st_mode))
        return 0;

#if defined(HAVE_EACCESS)
    if (eaccess(path, R_OK | X_OK) != 0)
        return 0;
#elif defined(HAVE_EUIDACCESS)
    if (euidaccess(path, R_OK | X_OK) != 0)
        return 0;
#endif

    tmp = strdup(path);
    if (!tmp)
        return 0;

    darray_append(ctx->includes, tmp);
    return 1;
}

/**
 * Append the default include directories to the context.
 */
XKB_EXPORT int
xkb_context_include_path_append_default(struct xkb_context *ctx)
{
    const char *home;
    char *user_path;
    int err;
    int ret = 0;

    ret |= xkb_context_include_path_append(ctx, DFLT_XKB_CONFIG_ROOT);

    home = getenv("HOME");
    if (!home)
        return ret;
    err = asprintf(&user_path, "%s/.xkb", home);
    if (err <= 0)
        return ret;
    ret |= xkb_context_include_path_append(ctx, user_path);
    free(user_path);

    return ret;
}

/**
 * Remove all entries in the context's include path.
 */
XKB_EXPORT void
xkb_context_include_path_clear(struct xkb_context *ctx)
{
    char **path;

    darray_foreach(path, ctx->includes)
    free(*path);

    darray_free(ctx->includes);
}

/**
 * xkb_context_include_path_clear() + xkb_context_include_path_append_default()
 */
XKB_EXPORT int
xkb_context_include_path_reset_defaults(struct xkb_context *ctx)
{
    xkb_context_include_path_clear(ctx);
    return xkb_context_include_path_append_default(ctx);
}

/**
 * Returns the number of entries in the context's include path.
 */
XKB_EXPORT unsigned int
xkb_context_num_include_paths(struct xkb_context *ctx)
{
    return darray_size(ctx->includes);
}

/**
 * Returns the given entry in the context's include path, or NULL if an
 * invalid index is passed.
 */
XKB_EXPORT const char *
xkb_context_include_path_get(struct xkb_context *ctx, unsigned int idx)
{
    if (idx >= xkb_context_num_include_paths(ctx))
        return NULL;

    return darray_item(ctx->includes, idx);
}

unsigned
xkb_context_take_file_id(struct xkb_context *ctx)
{
    return ctx->file_id++;
}

/**
 * Take a new reference on the context.
 */
XKB_EXPORT struct xkb_context *
xkb_context_ref(struct xkb_context *ctx)
{
    ctx->refcnt++;
    return ctx;
}

/**
 * Drop an existing reference on the context, and free it if the refcnt is
 * now 0.
 */
XKB_EXPORT void
xkb_context_unref(struct xkb_context *ctx)
{
    if (--ctx->refcnt > 0)
        return;

    xkb_context_include_path_clear(ctx);
    atom_table_free(ctx->atom_table);
    free(ctx);
}

static const char *
priority_to_prefix(int priority)
{
    switch (priority) {
    case LOG_DEBUG:
        return "Debug:";
    case LOG_INFO:
        return "Info:";
    case LOG_WARNING:
        return "Warning:";
    case LOG_ERR:
        return "Error:";
    case LOG_CRIT:
    case LOG_ALERT:
    case LOG_EMERG:
        return "Internal error:";
    default:
        return NULL;
    }
}

ATTR_PRINTF(3, 0) static void
default_log_fn(struct xkb_context *ctx, int priority,
               const char *fmt, va_list args)
{
    const char *prefix = priority_to_prefix(priority);

    if (prefix)
        fprintf(stderr, "%-15s", prefix);
    vfprintf(stderr, fmt, args);
}

static int
log_priority(const char *priority) {
    char *endptr;
    int prio;

    errno = 0;
    prio = strtol(priority, &endptr, 10);
    if (errno == 0 && (endptr[0] == '\0' || isspace(endptr[0])))
        return prio;
    if (strncasecmp(priority, "err", 3) == 0)
        return LOG_ERR;
    if (strncasecmp(priority, "warn", 4) == 0)
        return LOG_WARNING;
    if (strncasecmp(priority, "info", 4) == 0)
        return LOG_INFO;
    if (strncasecmp(priority, "debug", 5) == 0)
        return LOG_DEBUG;

    return LOG_ERR;
}

static int
log_verbosity(const char *verbosity) {
    char *endptr;
    int v;

    errno = 0;
    v = strtol(verbosity, &endptr, 10);
    if (errno == 0)
        return v;

    return 0;
}

/**
 * Create a new context.
 */
XKB_EXPORT struct xkb_context *
xkb_context_new(enum xkb_context_flags flags)
{
    const char *env;
    struct xkb_context *ctx = calloc(1, sizeof(*ctx));

    if (!ctx)
        return NULL;

    ctx->refcnt = 1;
    ctx->log_fn = default_log_fn;
    ctx->log_priority = LOG_ERR;
    ctx->log_verbosity = 0;

    /* Environment overwrites defaults. */
    env = getenv("XKB_LOG");
    if (env)
        xkb_set_log_priority(ctx, log_priority(env));

    env = getenv("XKB_VERBOSITY");
    if (env)
        xkb_set_log_verbosity(ctx, log_verbosity(env));

    if (!(flags & XKB_CONTEXT_NO_DEFAULT_INCLUDES) &&
        !xkb_context_include_path_append_default(ctx)) {
        log_err(ctx, "failed to add default include path %s\n",
                DFLT_XKB_CONFIG_ROOT);
        xkb_context_unref(ctx);
        return NULL;
    }

    ctx->atom_table = atom_table_new();
    if (!ctx->atom_table) {
        xkb_context_unref(ctx);
        return NULL;
    }

    return ctx;
}

xkb_atom_t
xkb_atom_intern(struct xkb_context *ctx, const char *string)
{
    return atom_intern(ctx->atom_table, string, false);
}

xkb_atom_t
xkb_atom_steal(struct xkb_context *ctx, char *string)
{
    return atom_intern(ctx->atom_table, string, true);
}

char *
xkb_atom_strdup(struct xkb_context *ctx, xkb_atom_t atom)
{
    return atom_strdup(ctx->atom_table, atom);
}

const char *
xkb_atom_text(struct xkb_context *ctx, xkb_atom_t atom)
{
    return atom_text(ctx->atom_table, atom);
}

void
xkb_log(struct xkb_context *ctx, int priority, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    ctx->log_fn(ctx, priority, fmt, args);
    va_end(args);
}

XKB_EXPORT void
xkb_set_log_fn(struct xkb_context *ctx,
               void (*log_fn)(struct xkb_context *ctx, int priority,
                              const char *fmt, va_list args))
{
    ctx->log_fn = log_fn;
}

XKB_EXPORT int
xkb_get_log_priority(struct xkb_context *ctx)
{
    return ctx->log_priority;
}

XKB_EXPORT void
xkb_set_log_priority(struct xkb_context *ctx, int priority)
{
    ctx->log_priority = priority;
}

XKB_EXPORT int
xkb_get_log_verbosity(struct xkb_context *ctx)
{
    return ctx->log_verbosity;
}

XKB_EXPORT void
xkb_set_log_verbosity(struct xkb_context *ctx, int verbosity)
{
    ctx->log_verbosity = verbosity;
}

XKB_EXPORT void *
xkb_get_user_data(struct xkb_context *ctx)
{
    if (ctx)
        return ctx->user_data;
    return NULL;
}

XKB_EXPORT void
xkb_set_user_data(struct xkb_context *ctx, void *user_data)
{
    ctx->user_data = user_data;
}
