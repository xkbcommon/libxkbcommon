/*
 * Copyright © 2014 Ran Benita <ran234@gmail.com>
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

#include <time.h>

#include "xkbcommon/xkbcommon.h"

#include "../test/test.h"
#include "bench.h"

#define BENCHMARK_ITERATIONS 1000

int
main(void)
{
    struct xkb_context *context;
    struct bench bench;
    const char* layout = "de";
    char *elapsed;

    context = test_get_context(CONTEXT_NO_FLAG);
    assert(context);

    xkb_context_set_log_level(context, XKB_LOG_LEVEL_CRITICAL);
    xkb_context_set_log_verbosity(context, 0);
    struct xkb_keymap *keymap;
    struct xkb_rule_names rmlvo = {
        .rules = NULL,
        .model = NULL,
        .layout = layout,
        .variant = NULL,
        .options = NULL
    };
    char *keymap_str;

    keymap = xkb_keymap_new_from_names(context, &rmlvo, 0);
    keymap_str = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_FORMAT_TEXT_V1);
    xkb_keymap_unref(keymap);

    bench_start(&bench);
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        keymap = xkb_keymap_new_from_string(
            context, keymap_str,
            XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
        assert(keymap);
        xkb_keymap_unref(keymap);
    }
    bench_stop(&bench);

    free(keymap_str);

    elapsed = bench_elapsed_str(&bench);
    fprintf(stderr, "compiled %d keymaps (layout: %s) in %ss\n",
            BENCHMARK_ITERATIONS, layout, elapsed);
    free(elapsed);

    xkb_context_unref(context);
    return 0;
}
