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
#include <stdio.h>
#include <unistd.h>

#include "xkb-priv.h"
#include "atom.h"

struct xkb_context {
    int refcnt;

    char **include_paths;
    int num_include_paths;
    int size_include_paths;

    /* xkbcomp needs to assign sequential IDs to XkbFile's it creates. */
    int file_id;

    struct atom_table *atom_table;
};

/**
 * Append one directory to the context's include path.
 */
_X_EXPORT int
xkb_context_include_path_append(struct xkb_context *ctx, const char *path)
{
    struct stat stat_buf;
    int err;

    if (ctx->size_include_paths <= ctx->num_include_paths) {
        int new_size;
        char **new_paths;
        new_size = ctx->size_include_paths + 2;
        new_paths = uTypedRecalloc(ctx->include_paths,
                                   ctx->size_include_paths,
                                   new_size,
                                   char *);
        if (!new_paths)
            return 0;
        ctx->include_paths = new_paths;
        ctx->size_include_paths = new_size;
    }

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

    ctx->include_paths[ctx->num_include_paths] = strdup(path);
    if (!ctx->include_paths[ctx->num_include_paths])
        return 0;
    ctx->num_include_paths++;

    return 1;
}

/**
 * Append the default include directories to the context.
 */
_X_EXPORT int
xkb_context_include_path_append_default(struct xkb_context *ctx)
{
    const char *home;
    char *user_path;
    int err;

    (void) xkb_context_include_path_append(ctx, DFLT_XKB_CONFIG_ROOT);

    home = getenv("HOME");
    if (!home)
        return 1;
    err = asprintf(&user_path, "%s/.xkb", home);
    if (err <= 0)
        return 1;
    (void) xkb_context_include_path_append(ctx, user_path);
    free(user_path);

    return 1;
}

/**
 * Remove all entries in the context's include path.
 */
_X_EXPORT void
xkb_context_include_path_clear(struct xkb_context *ctx)
{
    int i;

    for (i = 0; i < ctx->num_include_paths; i++) {
        free(ctx->include_paths[i]);
        ctx->include_paths[i] = NULL;
    }
    free(ctx->include_paths);
    ctx->include_paths = NULL;
    ctx->num_include_paths = 0;
}

/**
 * xkb_context_include_path_clear() + xkb_context_include_path_append_default()
 */
_X_EXPORT int
xkb_context_include_path_reset_defaults(struct xkb_context *ctx)
{
    xkb_context_include_path_clear(ctx);
    return xkb_context_include_path_append_default(ctx);
}

/**
 * Returns the number of entries in the context's include path.
 */
_X_EXPORT unsigned int
xkb_context_num_include_paths(struct xkb_context *ctx)
{
    return ctx->num_include_paths;
}

/**
 * Returns the given entry in the context's include path, or NULL if an
 * invalid index is passed.
 */
_X_EXPORT const char *
xkb_context_include_path_get(struct xkb_context *ctx, unsigned int idx)
{
    if (idx >= xkb_context_num_include_paths(ctx))
        return NULL;

    return ctx->include_paths[idx];
}

int
xkb_context_take_file_id(struct xkb_context *ctx)
{
    return ctx->file_id++;
}

/**
 * Take a new reference on the context.
 */
_X_EXPORT struct xkb_context *
xkb_context_ref(struct xkb_context *ctx)
{
    ctx->refcnt++;
    return ctx;
}

/**
 * Drop an existing reference on the context, and free it if the refcnt is
 * now 0.
 */
_X_EXPORT void
xkb_context_unref(struct xkb_context *ctx)
{
    if (--ctx->refcnt > 0)
        return;

    xkb_context_include_path_clear(ctx);
    atom_table_free(ctx->atom_table);
    free(ctx);
}

/**
 * Create a new context.
 */
_X_EXPORT struct xkb_context *
xkb_context_new(enum xkb_context_flags flags)
{
    struct xkb_context *ctx = calloc(1, sizeof(*ctx));

    if (!ctx)
        return NULL;

    ctx->refcnt = 1;

    if (!(flags & XKB_CONTEXT_NO_DEFAULT_INCLUDES) &&
        !xkb_context_include_path_append_default(ctx)) {
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
    return atom_intern(ctx->atom_table, string);
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
