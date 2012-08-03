/*
 * Copyright 2009 Dan Nicholson
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
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "xkbcommon/xkbcommon.h"
#include "test.h"

#define DATA_PATH "keymaps/stringcomp.data"

int
main(int argc, char *argv[])
{
    struct xkb_context *ctx = test_get_context();
    struct xkb_keymap *keymap;
    char *original, *dump;

    assert(ctx);

    /* Load in a prebuilt keymap, make sure we can compile it from a string,
     * then compare it to make sure we get the same result when dumping it
     * to a string. */
    original = test_read_file(DATA_PATH);
    assert(original);

    keymap = test_compile_string(ctx, original);
    assert(keymap);

    dump = xkb_map_get_as_string(keymap);
    assert(dump);

    if (!streq(original, dump)) {
        fprintf(stderr,
                "round-trip test failed: dumped map differs from original\n");
        fprintf(stderr, "path to original file: %s\n",
                test_get_path(DATA_PATH));
        fprintf(stderr, "length: dumped %zu, original %zu\n",
                strlen(dump), strlen(original));
        fprintf(stderr, "dumped map:\n");
        fprintf(stderr, "%s\n", dump);
        fflush(stderr);
        assert(0);
    }

    free(original);
    free(dump);
    xkb_map_unref(keymap);

    /* Make sure we can't (falsely claim to) compile an empty string. */
    keymap = test_compile_string(ctx, "");
    assert(!keymap);

    xkb_context_unref(ctx);

    return 0;
}
