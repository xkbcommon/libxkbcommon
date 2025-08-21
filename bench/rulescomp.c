/*
 * Copyright © 2009 Dan Nicholson
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <time.h>

#include "../test/test.h"
#include "bench.h"
#include "xkbcommon/xkbcommon.h"

#define BENCHMARK_ITERATIONS 1000

int
main(int argc, char *argv[])
{
    struct xkb_context *ctx;
    struct xkb_keymap *keymap;
    struct bench bench;
    char *elapsed;
    int i;

    ctx = test_get_context(0);
    assert(ctx);

    xkb_enable_quiet_logging(ctx);

    bench_start(&bench);
    for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
        keymap = test_compile_rules(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, "evdev",
                                    "pc104", "us", "", "");
        assert(keymap);
        xkb_keymap_unref(keymap);
    }
    bench_stop(&bench);

    elapsed = bench_elapsed_str(&bench);
    fprintf(stderr, "compiled %d keymaps in %ss\n",
            BENCHMARK_ITERATIONS, elapsed);
    free(elapsed);

    xkb_context_unref(ctx);
    return 0;
}
