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

/*
 * Test a sequence of keysyms, resulting from a sequence of key presses,
 * against the keysyms they're supposed to generate.
 *
 * - Each test runs with a clean state.
 * - Each line in the test is made up of:
 *   + A keycode, given as a KEY_* from linux/input.h.
 *   + A direction - DOWN for press, UP for release, BOTH for
 *     immediate press + release, REPEAT to just get the syms.
 *   + A sequence of keysyms that should result from this keypress.
 *
 * The vararg format is:
 * <KEY_*>  <DOWN | UP | BOTH>  <XKB_KEY_* (zero or more)>  <NEXT | FINISH>
 *
 * See below for examples.
 */
int
test_key_seq(struct xkb_keymap *keymap, ...)
{
    struct xkb_state *state;

    va_list ap;
    xkb_keycode_t kc;
    int op;
    xkb_keysym_t keysym;

    const xkb_keysym_t *syms;
    unsigned int nsyms, i;
    char ksbuf[64];

    fprintf(stderr, "----\n");

    state = xkb_state_new(keymap);
    assert(state);

    va_start(ap, keymap);

    for (;;) {
        kc = va_arg(ap, int) + EVDEV_OFFSET;
        op = va_arg(ap, int);

        nsyms = xkb_state_key_get_syms(state, kc, &syms);
        fprintf(stderr, "got %d syms for key 0x%x: [", nsyms, kc);

        if (op == DOWN || op == BOTH)
            xkb_state_update_key(state, kc, XKB_KEY_DOWN);
        if (op == UP || op == BOTH)
            xkb_state_update_key(state, kc, XKB_KEY_UP);

        for (i = 0; i < nsyms; i++) {
            keysym = va_arg(ap, int);
            xkb_keysym_get_name(syms[i], ksbuf, sizeof(ksbuf));
            fprintf(stderr, "%s%s", (i != 0) ? ", " : "", ksbuf);

            if (keysym == FINISH || keysym == NEXT) {
                xkb_keysym_get_name(syms[i], ksbuf, sizeof(ksbuf));
                fprintf(stderr, "Did not expect keysym: %s.\n", ksbuf);
                goto fail;
            }

            if (keysym != syms[i]) {
                xkb_keysym_get_name(keysym, ksbuf, sizeof(ksbuf));
                fprintf(stderr, "Expected keysym: %s. ", ksbuf);;
                xkb_keysym_get_name(syms[i], ksbuf, sizeof(ksbuf));
                fprintf(stderr, "Got keysym: %s.\n", ksbuf);;
                goto fail;
            }
        }

        fprintf(stderr, "]\n");

        keysym = va_arg(ap, int);
        if (keysym == NEXT)
            continue;
        if (keysym == FINISH)
            break;

        xkb_keysym_get_name(keysym, ksbuf, sizeof(ksbuf));
        fprintf(stderr, "Expected keysym: %s. Didn't get it.\n", ksbuf);
        goto fail;
    }

    va_end(ap);
    xkb_state_unref(state);
    return 1;

fail:
    va_end(ap);
    xkb_state_unref(state);
    return 0;
}

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
test_get_context(enum test_context_flags test_flags)
{
    enum xkb_context_flags ctx_flags;
    struct xkb_context *ctx;

    ctx_flags = XKB_CONTEXT_NO_DEFAULT_INCLUDES;
    if (test_flags & CONTEXT_ALLOW_ENVIRONMENT_NAMES) {
        unsetenv("XKB_DEFAULT_RULES");
        unsetenv("XKB_DEFAULT_MODEL");
        unsetenv("XKB_DEFAULT_LAYOUT");
        unsetenv("XKB_DEFAULT_VARIANT");
        unsetenv("XKB_DEFAULT_OPTIONS");
    }
    else {
        ctx_flags |= XKB_CONTEXT_NO_ENVIRONMENT_NAMES;
    }

    ctx = xkb_context_new(ctx_flags);
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
