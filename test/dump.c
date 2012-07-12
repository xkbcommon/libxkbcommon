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
#include "test.h"

int main(int argc, char *argv[])
{
    struct xkb_context *ctx = test_get_context();
    struct xkb_keymap *keymap;
    char *as_string, *expected;

    assert(ctx);

    keymap = test_compile_rules(ctx, "evdev", "pc105", "us,ru,ca,de",
                                ",,multix,neo", NULL);
    assert(keymap);

    as_string = xkb_map_get_as_string(keymap);
    xkb_map_unref(keymap);
    assert(as_string);

    expected = test_read_file("keymaps/dump.data");
    assert(expected);

    if (strcmp(as_string, expected) != 0) {
        fprintf(stderr, "dumped map differs from expected!\n\n");
        fprintf(stderr, "length: got %zu, expected %zu\n",
                strlen(as_string), strlen(expected));
        fprintf(stderr, "result:\n");
        fprintf(stderr, "%s\n", as_string);
        fflush(stderr);
        assert(0);
    }

    free(as_string);
    free(expected);

    xkb_context_unref(ctx);

    return 0;
}
