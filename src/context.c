/*
 * Copyright © 2012 Intel Corporation
 * Copyright © 2012 Ran Benita
 * SPDX-License-Identifier: MIT
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 */

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "xkbcommon/xkbcommon.h"
#include "context.h"
#include "darray.h"
#include "messages-codes.h"
#include "utils.h"


/**
 * Append one directory to the context’s include path.
 */
static int
context_include_path_append(struct xkb_context *ctx, const char *path)
{
    int err = ENOMEM;

    char *tmp = strdup(path);
    if (!tmp)
        goto err;

    struct stat stat_buf;
    err = stat(path, &stat_buf);
    if (err != 0) {
        err = errno;
        goto err;
    }
    if (!S_ISDIR(stat_buf.st_mode)) {
        err = ENOTDIR;
        goto err;
    }

    if (!check_eaccess(path, R_OK | X_OK)) {
        err = EACCES;
        goto err;
    }

    darray_append(ctx->includes, tmp);
    /* Use “info” log level to facilitate bug reporting. */
    log_info(ctx, XKB_LOG_MESSAGE_NO_ID, "Include path added: %s\n", tmp);

    return 1;

err:
    if (tmp)
        darray_append(ctx->failed_includes, tmp);
    /*
     * This error is not fatal because some valid paths may still be defined.
     * Use “info” log level to facilitate bug reporting.
     */
    log_info(ctx, XKB_LOG_MESSAGE_NO_ID,
             "Include path failed: \"%s\" (%s)\n", path, strerror(err));
    return 0;
}

/**
 * Append one directory to the context’s include path.
 */
int
xkb_context_include_path_append(struct xkb_context *ctx, const char *path)
{
    return (xkb_context_init_includes(ctx))
        ? context_include_path_append(ctx, path)
        : 0;
}

const char *
xkb_context_include_path_get_extra_path(struct xkb_context *ctx)
{
    const char * const extra = xkb_context_getenv(ctx, "XKB_CONFIG_EXTRA_PATH");
    /*
     * Only use default if path is undefined, but accept empty string, which may
     * be unintentional and should be reported.
     */
    return extra ? extra : DFLT_XKB_CONFIG_EXTRA_PATH;
}

const char *
xkb_context_include_path_get_system_path(struct xkb_context *ctx)
{
    const char * const root = xkb_context_getenv(ctx, "XKB_CONFIG_ROOT");
    /*
     * Only use default if path is undefined, but accept empty string, which may
     * be unintentional and should be reported.
     */
    return root ? root : DFLT_XKB_CONFIG_ROOT;
}

/**
 * Append the default include directories to the context.
 */
int
xkb_context_include_path_append_default(struct xkb_context *ctx)
{
    /*
     * We do not call `xkb_context_init_includes()` here, because either
     * we already initialized the includes paths or we are doing it now.
     */

    char *user_path;
    int ret = 0;

    const char * const home = xkb_context_getenv(ctx, "HOME");
    const char * const xdg = xkb_context_getenv(ctx, "XDG_CONFIG_HOME");
    /* Accept empty string, which may be unintentional and should be reported */
    if (xdg != NULL) {
        user_path = asprintf_safe("%s/xkb", xdg);
        if (user_path) {
            ret |= context_include_path_append(ctx, user_path);
            free(user_path);
        }
    } else if (home != NULL) {
        /* XDG_CONFIG_HOME fallback is $HOME/.config/ */
        user_path = asprintf_safe("%s/.config/xkb", home);
        if (user_path) {
            ret |= context_include_path_append(ctx, user_path);
            free(user_path);
        }
    }

    if (home != NULL) {
        user_path = asprintf_safe("%s/.xkb", home);
        if (user_path) {
            ret |= context_include_path_append(ctx, user_path);
            free(user_path);
        }
    }

    const char * const extra = xkb_context_include_path_get_extra_path(ctx);
    ret |= context_include_path_append(ctx, extra);

    /* Canonical XKB root */
    const char * const root = xkb_context_include_path_get_system_path(ctx);
    const bool has_root = context_include_path_append(ctx, root);
    ret |= has_root;

    /*
     * Fallback for misconfigured setups.
     * Some setups use the assumption that the canonical XKB root is always the
     * legacy X11 one, but this is no longer true since xkeyboard-config 2.45,
     * where the X11 path is now a mere symlink to a dedicated xkeyboard-config
     * data directory.
     * This fallback can still be skipped if deliberately using an empty string
     * for the canonical XKB root hereinabove.
     */
    if (!has_root && root[0] != '\0') {
        log_warn(ctx, XKB_LOG_MESSAGE_NO_ID,
                 "Root include path failed; fallback to \"%s\". "
                 "The setup is probably misconfigured. "
                 "Please ensure that \"%s\" is available in the environment.\n",
                 DFLT_XKB_LEGACY_ROOT, root);
        ret |= context_include_path_append(ctx, DFLT_XKB_LEGACY_ROOT);
    }

    return ret;
}

/**
 * Remove all entries in the context's include path.
 */
void
xkb_context_include_path_clear(struct xkb_context *ctx)
{
    char **path;

    darray_foreach(path, ctx->includes)
        free(*path);
    darray_free(ctx->includes);

    darray_foreach(path, ctx->failed_includes)
        free(*path);
    darray_free(ctx->failed_includes);

    /* It does not make sense to keep the pending defaults */
    ctx->pending_default_includes = false;
}

/**
 * xkb_context_include_path_clear() + xkb_context_include_path_append_default()
 */
int
xkb_context_include_path_reset_defaults(struct xkb_context *ctx)
{
    xkb_context_include_path_clear(ctx);
    return xkb_context_include_path_append_default(ctx);
}

/**
 * Returns the number of entries in the context's include path.
 */
unsigned int
xkb_context_num_include_paths(struct xkb_context *ctx)
{
    return (xkb_context_init_includes(ctx))
        ? darray_size(ctx->includes)
        : 0;
}

/**
 * Returns the given entry in the context's include path, or NULL if an
 * invalid index is passed.
 */
const char *
xkb_context_include_path_get(struct xkb_context *ctx, unsigned int idx)
{
    if (idx >= xkb_context_num_include_paths(ctx))
        return NULL;

    return darray_item(ctx->includes, idx);
}

/**
 * Take a new reference on the context.
 */
struct xkb_context *
xkb_context_ref(struct xkb_context *ctx)
{
    assert(ctx->refcnt > 0);
    ctx->refcnt++;
    return ctx;
}

/**
 * Drop an existing reference on the context, and free it if the refcnt is
 * now 0.
 */
void
xkb_context_unref(struct xkb_context *ctx)
{
    assert(!ctx || ctx->refcnt > 0);
    if (!ctx || --ctx->refcnt > 0)
        return;

    free(ctx->x11_atom_cache);
    xkb_context_include_path_clear(ctx);
    atom_table_free(ctx->atom_table);
    free(ctx);
}

static const char *
log_level_to_prefix(enum xkb_log_level level)
{
    switch (level) {
    case XKB_LOG_LEVEL_DEBUG:
        return "xkbcommon: DEBUG: ";
    case XKB_LOG_LEVEL_INFO:
        return "xkbcommon: INFO: ";
    case XKB_LOG_LEVEL_WARNING:
        return "xkbcommon: WARNING: ";
    case XKB_LOG_LEVEL_ERROR:
        return "xkbcommon: ERROR: ";
    case XKB_LOG_LEVEL_CRITICAL:
        return "xkbcommon: CRITICAL: ";
    default:
        return NULL;
    }
}

ATTR_PRINTF(3, 0) static void
default_log_fn(struct xkb_context *ctx, enum xkb_log_level level,
               const char *fmt, va_list args)
{
    const char *prefix = log_level_to_prefix(level);

    if (prefix)
        fprintf(stderr, "%s", prefix);
    vfprintf(stderr, fmt, args);
}

static enum xkb_log_level
log_level(const char *level) {
    char *endptr;
    enum xkb_log_level lvl;

    errno = 0;
    lvl = strtol(level, &endptr, 10);
    if (errno == 0 && (endptr[0] == '\0' || is_space(endptr[0])))
        return lvl;
    if (istreq_prefix("crit", level))
        return XKB_LOG_LEVEL_CRITICAL;
    if (istreq_prefix("err", level))
        return XKB_LOG_LEVEL_ERROR;
    if (istreq_prefix("warn", level))
        return XKB_LOG_LEVEL_WARNING;
    if (istreq_prefix("info", level))
        return XKB_LOG_LEVEL_INFO;
    if (istreq_prefix("debug", level) || istreq_prefix("dbg", level))
        return XKB_LOG_LEVEL_DEBUG;

    return XKB_LOG_LEVEL_ERROR;
}

static int
log_verbosity(const char *verbosity) {
    errno = 0;
    const long int v = strtol(verbosity, NULL, 10);
    if (errno == 0) {
        /* Truncate */
        return (int) v;
    }

    return XKB_LOG_VERBOSITY_DEFAULT;
}

/**
 * Create a new context.
 */
struct xkb_context *
xkb_context_new(enum xkb_context_flags flags)
{
    const char *env;
    struct xkb_context *ctx = calloc(1, sizeof(*ctx));

    if (!ctx)
        return NULL;

    ctx->refcnt = 1;
    ctx->log_fn = default_log_fn;
    ctx->log_level = XKB_LOG_LEVEL_ERROR;
    ctx->log_verbosity = XKB_LOG_VERBOSITY_DEFAULT;
    ctx->use_environment_names = !(flags & XKB_CONTEXT_NO_ENVIRONMENT_NAMES);
    ctx->use_secure_getenv = !(flags & XKB_CONTEXT_NO_SECURE_GETENV);

    /*
     * Default includes paths are delayed and added only if necessary.
     *
     * It is more efficient for most clients, which only get the keymap from the
     * server: it avoids unnecessary allocations and file system queries.
     *
     * It also avoid the corner case where a containerized app lacks access to
     * the XKB directories.
     *
     * There might be an issue in case the environment variables relevant to the
     * include paths change between the context initialization and the call to
     * `xkb_context_include_path_append_default()`. However this seems very
     * unlikely and would have already triggered issues when the `%`-expansion
     * is used.
     */
    ctx->pending_default_includes = !(flags & XKB_CONTEXT_NO_DEFAULT_INCLUDES);
    darray_init(ctx->includes);
    darray_init(ctx->failed_includes);

    /* Environment overwrites defaults. */
    env = xkb_context_getenv(ctx, "XKB_LOG_LEVEL");
    if (env)
        xkb_context_set_log_level(ctx, log_level(env));

    env = xkb_context_getenv(ctx, "XKB_LOG_VERBOSITY");
    if (env)
        xkb_context_set_log_verbosity(ctx, log_verbosity(env));

    ctx->atom_table = atom_table_new();
    if (!ctx->atom_table) {
        xkb_context_unref(ctx);
        return NULL;
    }

    ctx->x11_atom_cache = NULL;

    return ctx;
}

void
xkb_context_set_log_fn(struct xkb_context *ctx,
                       void (*log_fn)(struct xkb_context *ctx,
                                      enum xkb_log_level level,
                                      const char *fmt, va_list args))
{
    ctx->log_fn = (log_fn ? log_fn : default_log_fn);
}

enum xkb_log_level
xkb_context_get_log_level(struct xkb_context *ctx)
{
    return ctx->log_level;
}

void
xkb_context_set_log_level(struct xkb_context *ctx, enum xkb_log_level level)
{
    ctx->log_level = level;
}

int
xkb_context_get_log_verbosity(struct xkb_context *ctx)
{
    return ctx->log_verbosity;
}

void
xkb_context_set_log_verbosity(struct xkb_context *ctx, int verbosity)
{
    ctx->log_verbosity = verbosity;
}

void *
xkb_context_get_user_data(struct xkb_context *ctx)
{
    if (ctx)
        return ctx->user_data;
    return NULL;
}

void
xkb_context_set_user_data(struct xkb_context *ctx, void *user_data)
{
    ctx->user_data = user_data;
}
