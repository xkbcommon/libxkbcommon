/*
 * Copyright © 2012 Intel Corporation
 * SPDX-License-Identifier: MIT
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 */
#pragma once

#include "config.h"

#include <stddef.h>

#include "xkbcommon/xkbcommon.h"
#include "atom.h"
#include "darray.h"
#include "messages-codes.h"
#include "rmlvo.h"
#include "utils.h"

struct xkb_context {
    int refcnt;

    ATTR_PRINTF(3, 0) void (*log_fn)(struct xkb_context *ctx,
                                     enum xkb_log_level level,
                                     const char *fmt, va_list args);
    enum xkb_log_level log_level;
    int log_verbosity;
    void *user_data;

    struct xkb_rule_names names_dflt;

    darray(char *) includes;
    darray(char *) failed_includes;

    struct atom_table *atom_table;

    /* Used and allocated by xkbcommon-x11, free()d with the context. */
    void *x11_atom_cache;

    /* Buffer for the *Text() functions. */
    char text_buffer[2048];
    size_t text_next;

    unsigned int use_environment_names : 1;
    unsigned int use_secure_getenv : 1;
};

char *
xkb_context_getenv(struct xkb_context *ctx, const char *name);

darray_size_t
xkb_context_num_failed_include_paths(struct xkb_context *ctx);

const char *
xkb_context_failed_include_path_get(struct xkb_context *ctx,
                                    darray_size_t idx);

const char *
xkb_context_include_path_get_extra_path(struct xkb_context *ctx);

const char *
xkb_context_include_path_get_system_path(struct xkb_context *ctx);

XKB_EXPORT_PRIVATE darray_size_t
xkb_atom_table_size(struct xkb_context *ctx);

/*
 * Returns XKB_ATOM_NONE if @string was not previously interned,
 * otherwise returns the atom.
 */
xkb_atom_t
xkb_atom_lookup(struct xkb_context *ctx, const char *string);

XKB_EXPORT_PRIVATE xkb_atom_t
xkb_atom_intern(struct xkb_context *ctx, const char *string, size_t len);

#define xkb_atom_intern_literal(ctx, literal) \
    xkb_atom_intern((ctx), (literal), sizeof(literal) - 1)

/**
 * If @string is dynamically allocated, NUL-terminated, free'd immediately
 * after being interned, and not used afterwards, use this function
 * instead of xkb_atom_intern to avoid some unnecessary allocations.
 * The caller should not use or free the passed in string afterwards.
 */
xkb_atom_t
xkb_atom_steal(struct xkb_context *ctx, char *string);

XKB_EXPORT_PRIVATE const char *
xkb_atom_text(struct xkb_context *ctx, xkb_atom_t atom);

char *
xkb_context_get_buffer(struct xkb_context *ctx, size_t size);

XKB_EXPORT_PRIVATE ATTR_PRINTF(4, 5) void
xkb_log(struct xkb_context *ctx, enum xkb_log_level level, int verbosity,
        const char *fmt, ...);

enum RMLVO
xkb_context_sanitize_rule_names(struct xkb_context *ctx,
                                struct xkb_rule_names *rmlvo);

/*
 * The format is not part of the argument list in order to avoid the
 * "ISO C99 requires rest arguments to be used" warning when only the
 * format is supplied without arguments. Not supplying it would still
 * result in an error, though.
 */
#define xkb_log_with_code(ctx, level, verbosity, msg_id, fmt, ...) \
    xkb_log(ctx, level, verbosity, PREPEND_MESSAGE_ID(msg_id, fmt), ##__VA_ARGS__)
#define log_dbg(ctx, id, ...) \
    xkb_log_with_code((ctx), XKB_LOG_LEVEL_DEBUG, XKB_LOG_VERBOSITY_MINIMAL, id, __VA_ARGS__)
#define log_info(ctx, id, ...) \
    xkb_log_with_code((ctx), XKB_LOG_LEVEL_INFO, XKB_LOG_VERBOSITY_MINIMAL, id, __VA_ARGS__)
#define log_warn(ctx, id, ...) \
    xkb_log_with_code((ctx), XKB_LOG_LEVEL_WARNING, XKB_LOG_VERBOSITY_MINIMAL, id, __VA_ARGS__)
#define log_vrb(ctx, vrb, id, ...) \
    xkb_log_with_code((ctx), XKB_LOG_LEVEL_WARNING, (vrb), id, __VA_ARGS__)
#define log_err(ctx, id, ...) \
    xkb_log_with_code((ctx), XKB_LOG_LEVEL_ERROR, XKB_LOG_VERBOSITY_MINIMAL, id, __VA_ARGS__)
#define log_wsgo(ctx, id, ...) \
    xkb_log_with_code((ctx), XKB_LOG_LEVEL_CRITICAL, XKB_LOG_VERBOSITY_MINIMAL, id, __VA_ARGS__)

/*
 * Variants which are prefixed by the name of the function they're
 * called from.
 * Here we must have the silly 1 variant.
 */
#define log_err_func(ctx, id, fmt, ...) \
    log_err(ctx, id, "%s: " fmt, __func__, __VA_ARGS__)
#define log_err_func1(ctx, id, fmt) \
    log_err(ctx, id, "%s: " fmt, __func__)
