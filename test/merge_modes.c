/*
 * Copyright © 2024 Pierre Le Marre <dev@wismill.eu>
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
 */

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "test.h"
#include "merge_modes.h"

/* Our keymap compiler is the xkbcommon buffer compiler */
static struct xkb_keymap *
compile_buffer(struct xkb_context *context, const char *buf, size_t len,
                     void *private)
{
    return test_compile_buffer(context, buf, len);
}

static void
test_symbols(struct xkb_context *ctx, enum update_files update_output_files)
{
    make_symbols_tests("merge_modes", "", "",
                       compile_buffer, NULL, update_output_files);
}

int
main(int argc, char *argv[])
{
    test_init();

    /* Check if we run the tests or just update their outputs */
    enum update_files update_output_files = NO_UPDATE;
    if (argc > 1) {
        if (streq(argv[1], "update")) {
            /* Update files with *expected* results */
            update_output_files = UPDATE_USING_TEST_INPUT;
        } else if (streq(argv[1], "update-obtained")) {
            /* Update files with *obtained* results */
            update_output_files = UPDATE_USING_TEST_OUTPUT;
        } else {
            fprintf(stderr, "ERROR: unsupported argument: \"%s\n\".", argv[1]);
            exit(EXIT_FAILURE);
        }
    }

    struct xkb_context *ctx = test_get_context(CONTEXT_NO_FLAG);
    assert(ctx);

    test_symbols(ctx, update_output_files);

    xkb_context_unref(ctx);

    return EXIT_SUCCESS;
}
