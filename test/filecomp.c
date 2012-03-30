/*
Copyright 2009 Dan Nicholson

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the names of the authors or their
institutions shall not be used in advertising or otherwise to promote the
sale, use or other dealings in this Software without prior written
authorization from the authors.
*/

#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "xkbcommon/xkbcommon.h"

static int
test_file(const char *path)
{
    int fd;
    struct xkb_context *context;
    struct xkb_keymap *xkb;

    fd = open(path, O_RDONLY);
    assert(fd >= 0);

    context = xkb_context_new();
    assert(context);

    fprintf(stderr, "\nCompiling path: %s\n", path);

    xkb = xkb_map_new_from_fd(context, fd, XKB_KEYMAP_FORMAT_TEXT_V1);
    close(fd);

    if (!xkb) {
        fprintf(stderr, "Failed to compile keymap\n");
        xkb_context_unref(context);
        return 0;
    }

    xkb_map_unref(xkb);
    xkb_context_unref(context);
    return 1;
}

static int
test_file_name(const char *file_name)
{
    static char path[PATH_MAX];
    const char *srcdir = getenv("srcdir");

    snprintf(path, PATH_MAX - 1, "%s/test/data/%s", srcdir ? srcdir : ".", file_name);
    return test_file(path);
}

static int
test_string(const char *string)
{
    struct xkb_context *context;
    struct xkb_keymap *xkb;

    context = xkb_context_new();
    assert(context);

    fprintf(stderr, "\nCompiling string\n");

    xkb = xkb_map_new_from_string(context, string, XKB_KEYMAP_FORMAT_TEXT_V1);
    if (!xkb) {
        xkb_context_unref(context);
        return 0;
    }

    xkb_map_unref(xkb);
    xkb_context_unref(context);
    return 1;
}

int
main(void)
{

    assert(test_file_name("basic.xkb"));
    /* XXX check we actually get qwertz here ... */
    assert(test_file_name("default.xkb"));
    assert(test_file_name("comprehensive-plus-geom.xkb"));

    assert(!test_file_name("bad.xkb"));

    assert(!test_string(""));

    return 0;
}
