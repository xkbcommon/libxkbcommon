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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "xkbcommon/xkbcommon.h"

int main(int argc, char *argv[])
{
    struct xkb_context *ctx = xkb_context_new(0);
    struct xkb_keymap *keymap;
    struct xkb_rule_names names = {
        .rules = "evdev",
        .model = "pc105",
        .layout = "us,ru,ca,de",
        .variant = ",,multix,neo",
        .options = NULL,
    };
    struct stat stat_buf;
    char *as_string, *expected;
    const char *srcdir = getenv("srcdir");
    char *path;
    int fd;

    assert(srcdir);
    assert(asprintf(&path, "%s/test/dump.data", srcdir) != -1);
    fd = open(path, O_RDONLY);
    assert(fd >= 0);
    assert(stat(path, &stat_buf) == 0);
    assert(stat_buf.st_size > 0);
    free(path);

    expected = mmap(NULL, stat_buf.st_size, PROT_READ, MAP_SHARED, fd, 0);
    assert(expected != MAP_FAILED);

    assert(ctx);
    keymap = xkb_map_new_from_names(ctx, &names, 0);
    assert(keymap);

    as_string = xkb_map_get_as_string(keymap);
    assert(as_string);

    if (strcmp(as_string, expected) != 0) {
        printf("dumped map differs from expected!\n\n");
        printf("length: got %lu, expected %lu\n",
               (unsigned long) strlen(as_string),
               (unsigned long) strlen(expected));
        printf("result:\n%s\n", as_string);
        assert(0);
    }

    free(as_string);
    xkb_map_unref(keymap);
    xkb_context_unref(ctx);

    return 0;
}
