/*
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <stdlib.h>
#include <time.h>

#include "../test/test.h"
#include "bench.h"
#include "xkbcommon/xkbcommon.h"

#define BENCHMARK_ITERATIONS 20000000

static void
bench_key_proc(struct xkb_state *state)
{
    int8_t keys[256] = { 0 };
    xkb_keycode_t keycode;
    xkb_keysym_t keysym;
    int i;

    for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
        keycode = (rand() % (255 - 9)) + 9;
        if (keys[keycode]) {
            xkb_state_update_key(state, keycode, XKB_KEY_UP);
            keys[keycode] = 0;
            keysym = xkb_state_key_get_one_sym(state, keycode);
            (void) keysym;
        } else {
            xkb_state_update_key(state, keycode, XKB_KEY_DOWN);
            keys[keycode] = 1;
        }
    }
}

int
main(void)
{
    struct xkb_context *ctx;
    struct xkb_keymap *keymap;
    struct xkb_state *state;
    struct bench bench;
    char *elapsed;

    ctx = test_get_context(0);
    assert(ctx);

    keymap = test_compile_rules(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, "evdev",
                                "pc104", "us,ru,il,de",
                                ",,,neo", "grp:menu_toggle");
    assert(keymap);

    state = xkb_state_new(keymap);
    assert(state);

    xkb_context_set_log_level(ctx, XKB_LOG_LEVEL_CRITICAL);
    xkb_context_set_log_verbosity(ctx, 0);

    srand((unsigned) time(NULL));

    bench_start(&bench);
    bench_key_proc(state);
    bench_stop(&bench);

    elapsed = bench_elapsed_str(&bench);
    fprintf(stderr, "ran %d iterations in %ss\n",
            BENCHMARK_ITERATIONS, elapsed);
    free(elapsed);

    xkb_state_unref(state);
    xkb_keymap_unref(keymap);
    xkb_context_unref(ctx);

    return 0;
}
