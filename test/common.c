/*
 * Copyright © 2009 Dan Nicholson <dbn.lists@gmail.com>
 * Copyright © 2012 Intel Corporation
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the names of the authors or their
 * institutions shall not be used in advertising or otherwise to promote the
 * sale, use or other dealings in this Software without prior written
 * authorization from the authors.
 *
 * Author: Dan Nicholson <dbn.lists@gmail.com>
 *         Daniel Stone <daniel@fooishbar.org>
 *         Ran Benita <ran234@gmail.com>
 */

#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "test.h"

const char *
test_get_path(const char *path_rel)
{
    static char path[PATH_MAX];
    const char *srcdir = getenv("srcdir");

    snprintf(path, PATH_MAX - 1,
             "%s/test/data/%s", srcdir ? srcdir : ".",
             path_rel ? path_rel : "");

    return path;
}

char *
test_read_file(const char *path_rel)
{
    struct stat info;
    char *ret, *tmp;
    int fd, count, remaining;

    fd = open(test_get_path(path_rel), O_RDONLY);
    if (fd < 0)
        return NULL;

    if (fstat(fd, &info) != 0) {
        close(fd);
        return NULL;
    }

    ret = malloc(info.st_size + 1);
    if (!ret) {
        close(fd);
        return NULL;
    }

    remaining = info.st_size;
    tmp = ret;
    while ((count = read(fd, tmp, remaining))) {
        remaining -= count;
        tmp += count;
    }
    ret[info.st_size] = '\0';
    close(fd);

    if (remaining != 0) {
        free(ret);
        return NULL;
    }

    return ret;
}

struct xkb_context *
test_get_context(void)
{
    struct xkb_context *ctx = xkb_context_new(XKB_CONTEXT_NO_DEFAULT_INCLUDES);

    if (!ctx)
        return NULL;

    xkb_context_include_path_append(ctx, test_get_path(""));

    return ctx;
}

struct xkb_keymap *
test_compile_file(struct xkb_context *context, const char *path_rel)
{
    struct xkb_keymap *keymap;
    FILE *file;
    const char *path = test_get_path(path_rel);

    file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, "Failed to open path: %s\n", path);
        return NULL;
    }
    assert(file != NULL);

    keymap = xkb_keymap_new_from_file(context, file,
                                      XKB_KEYMAP_FORMAT_TEXT_V1, 0);
    fclose(file);

    if (!keymap) {
        fprintf(stderr, "Failed to compile path: %s\n", path);
        return NULL;
    }

    fprintf(stderr, "Successfully compiled path: %s\n", path);

    return keymap;
}

struct xkb_keymap *
test_compile_string(struct xkb_context *context, const char *string)
{
    struct xkb_keymap *keymap;

    keymap = xkb_keymap_new_from_string(context, string,
                                        XKB_KEYMAP_FORMAT_TEXT_V1, 0);
    if (!keymap) {
        fprintf(stderr, "Failed to compile string\n");
        return NULL;
    }

    return keymap;
}

struct xkb_keymap *
test_compile_rules(struct xkb_context *context, const char *rules,
                   const char *model, const char *layout,
                   const char *variant, const char *options)
{
    struct xkb_keymap *keymap;
    struct xkb_rule_names rmlvo = {
        .rules = rules,
        .model = model,
        .layout = layout,
        .variant = variant,
        .options = options
    };

    keymap = xkb_keymap_new_from_names(context, &rmlvo, 0);
    if (!keymap) {
        fprintf(stderr,
                "Failed to compile RMLVO: '%s', '%s', '%s', '%s', '%s'\n",
                rules, model, layout, variant, options);
        return NULL;
    }

    return keymap;
}
