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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>

#include "xkbcommon/xkbcommon.h"
#include "XKBcommonint.h"
#include "utils.h"

struct xkb_context {
    int refcnt;
    char **include_paths;
    int num_include_paths;
    int size_include_paths;
};

/**
 * Append one directory to the context's include path.
 */
int
xkb_context_include_path_append(struct xkb_context *context, const char *path)
{
    struct stat stat_buf;
    int err;

    if (context->size_include_paths <= context->num_include_paths) {
        int new_size;
        char **new_paths;
        new_size = context->size_include_paths + 2;
        new_paths = uTypedRecalloc(context->include_paths,
                                   context->size_include_paths,
                                   new_size,
                                   typeof(new_paths));
        if (!new_paths)
            return 0;
        context->include_paths = new_paths;
        context->size_include_paths = new_size;
    }

    err = stat(path, &stat_buf);
    if (err != 0)
        return 0;
    if (!S_ISDIR(stat_buf.st_mode))
        return 0;
    if (eaccess(path, R_OK | X_OK) != 0)
        return 0;

    context->include_paths[context->num_include_paths] = strdup(path);
    if (!context->include_paths[context->num_include_paths])
        return 0;
    context->num_include_paths++;

    return 1;
}

/**
 * Append the default include directories to the context.
 */
int
xkb_context_include_path_append_default(struct xkb_context *context)
{
    const char *home = getenv("HOME");
    char *user_path;
    int err;

    (void) xkb_context_include_path_append(context, DFLT_XKB_CONFIG_ROOT);

    home = getenv("HOME");
    if (!home)
        return 1;
    err = asprintf(&user_path, "%s/.xkb", home);
    if (err <= 0)
        return 1;
    (void) xkb_context_include_path_append(context, user_path);
    free(user_path);

    return 1;
}

/**
 * Remove all entries in the context's include path.
 */
void
xkb_context_include_path_clear(struct xkb_context *context)
{
    int i;

    for (i = 0; i < context->num_include_paths; i++) {
        free(context->include_paths[i]);
        context->include_paths[i] = NULL;
    }
    free(context->include_paths);
    context->include_paths = NULL;
    context->num_include_paths = 0;
}

/**
 * xkb_context_include_path_clear() + xkb_context_include_path_append_default()
 */
int
xkb_context_include_path_reset_defaults(struct xkb_context *context)
{
    xkb_context_include_path_clear(context);
    return xkb_context_include_path_append_default(context);
}

/**
 * Returns the number of entries in the context's include path.
 */
unsigned int
xkb_context_num_include_paths(struct xkb_context *context)
{
    return context->num_include_paths;
}

/**
 * Returns the given entry in the context's include path, or NULL if an
 * invalid index is passed.
 */
const char *
xkb_context_include_path_get(struct xkb_context *context, unsigned int idx)
{
    if (idx >= xkb_context_num_include_paths(context))
        return NULL;

    return context->include_paths[idx];
}

/**
 * Take a new reference on the context.
 */
void
xkb_context_ref(struct xkb_context *context)
{
    context->refcnt++;
}

/**
 * Drop an existing reference on the context, and free it if the refcnt is
 * now 0.
 */
void
xkb_context_unref(struct xkb_context *context)
{
    if (--context->refcnt > 0)
        return;

    xkb_context_include_path_clear(context);
    free(context);
}

/**
 * Create a new context.
 */
struct xkb_context *
xkb_context_new(void)
{
    struct xkb_context *context = calloc(1, sizeof(*context));

    if (!context)
        return NULL;

    context->refcnt = 1;

    if (!xkb_context_include_path_append_default(context)) {
        xkb_context_unref(context);
        return NULL;
    }

    return context;
}
