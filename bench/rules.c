/*
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <time.h>

#include "../test/test.h"
#include "xkbcomp/xkbcomp-priv.h"
#include "xkbcomp/rules.h"
#include "bench.h"

#define BENCHMARK_ITERATIONS 20000

int
main(int argc, char *argv[])
{
    struct xkb_context *ctx;
    int i;
    struct xkb_rule_names rmlvo = {
        "evdev", "pc105", "us,il", ",", "ctrl:nocaps,grp:menu_toggle",
    };
    struct bench bench;
    char *elapsed;

    ctx = test_get_context(0);
    assert(ctx);

    xkb_context_set_log_level(ctx, XKB_LOG_LEVEL_CRITICAL);
    xkb_context_set_log_verbosity(ctx, 0);

    bench_start(&bench);
    for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
        struct xkb_component_names kccgst;

        assert(xkb_components_from_rules(ctx, &rmlvo, &kccgst, NULL));
        free(kccgst.keycodes);
        free(kccgst.types);
        free(kccgst.compat);
        free(kccgst.symbols);
    }
    bench_stop(&bench);

    elapsed = bench_elapsed_str(&bench);
    fprintf(stderr, "processed %d rule files in %ss\n",
            BENCHMARK_ITERATIONS, elapsed);
    free(elapsed);

    xkb_context_unref(ctx);
    return 0;
}
