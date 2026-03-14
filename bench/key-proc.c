/*
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <stdlib.h>
#include <time.h>

#include "xkbcommon/xkbcommon.h"
#include "test/test.h"
#include "tools/tools-common.h"
#include "bench.h"
#include "utils.h"

#define BENCHMARK_ITERATIONS 3000000

NOINLINE static void
bench_state_api(struct xkb_state *state)
{
    bool keys[256] = { 0 };
    volatile unsigned long acc_changed = 0;
    volatile unsigned long acc_keysym  = 0;

    for (size_t i = 0; i < BENCHMARK_ITERATIONS; i++) {
        const xkb_keycode_t keycode = (rand() % (255 - 9)) + 9;
        const enum xkb_key_direction direction = (keys[keycode])
                                               ? XKB_KEY_UP : XKB_KEY_DOWN;
        const enum xkb_state_component changed =
            xkb_state_update_key(state, keycode, direction);
        acc_changed += (unsigned long)changed;

        if (keys[keycode]) {
            const xkb_keysym_t keysym =
                xkb_state_key_get_one_sym(state, keycode);
            acc_keysym += (unsigned long)keysym;
        }

        keys[keycode] = !keys[keycode];
    }
}

NOINLINE static void
bench_state_machine_api(struct xkb_state_machine *sm,
                        struct xkb_event_iterator *events,
                        struct xkb_state *state)
{
    bool keys[256] = { 0 };
    volatile unsigned long acc_ret = 0;
    volatile unsigned long acc_changed = 0;
    volatile unsigned long acc_keysym  = 0;
    const struct xkb_event *event;

    for (size_t i = 0; i < BENCHMARK_ITERATIONS; i++) {
        const xkb_keycode_t keycode = (rand() % (255 - 9)) + 9;
        const enum xkb_key_direction direction = (keys[keycode])
                                               ? XKB_KEY_UP : XKB_KEY_DOWN;
        const int ret =
            xkb_state_machine_update_key(sm, events, keycode, direction);
        acc_ret += (unsigned long)ret;

        enum xkb_state_component changed = 0;
        while ((event = xkb_event_iterator_next(events))) {
            changed |= xkb_state_update_from_event(state, event);
        }
        acc_changed += (unsigned long)changed;

        if (keys[keycode]) {
            const xkb_keysym_t keysym =
                xkb_state_key_get_one_sym(state, keycode);
            acc_keysym += (unsigned long)keysym;
        }

        keys[keycode] = !keys[keycode];
    }
}

int
main(void)
{
    struct xkb_context *ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    assert(ctx);
    const enum xkb_keymap_format format = XKB_KEYMAP_FORMAT_TEXT_V1;
    const enum xkb_keymap_compile_flags flags = XKB_KEYMAP_COMPILE_NO_FLAGS;

    struct xkb_keymap *keymap;
    if (is_pipe_or_regular_file(STDIN_FILENO)) {
        fprintf(stderr, "Bench using keymap from stdin\n");
        FILE *file = tools_read_stdin();
        assert(file);
        keymap = xkb_keymap_new_from_file(ctx, file, format, flags);
        assert(keymap);
    } else {
        fprintf(stderr, "Bench using keymap from fixed RMLVO\n");
        static const struct xkb_rule_names rmlvo = {
            .rules = "evdev",
            .model = "pc104",
            .layout = "us,ru,il,de",
            .variant = ",,,neo",
            .options = "grp:menu_toggle"
        };
        keymap = xkb_keymap_new_from_names2(ctx, &rmlvo, format, flags);
        assert(keymap);
    }

    xkb_enable_quiet_logging(ctx);

    srand((unsigned) time(NULL));

    struct bench bench;
    struct bench_time elapsed;
    long average;
    char *elapsed_str;

    /*
     * Simple state API
     */

    struct xkb_state *state = xkb_state_new(keymap);
    assert(state);

    bench_start2(&bench);
    bench_state_api(state);
    bench_stop2(&bench);

    xkb_state_unref(state);

    bench_elapsed(&bench, &elapsed);
    average = (bench_time_elapsed_nanoseconds(&elapsed)) / BENCHMARK_ITERATIONS;
    elapsed_str = bench_elapsed_str(&bench);
    fprintf(stdout, "Simple state machine API: average=%ldns; %d iterations in %ss\n",
            average, BENCHMARK_ITERATIONS, elapsed_str);
    free(elapsed_str);

    /*
     * Full state machine API
     */

    struct xkb_state_machine *sm = xkb_state_machine_new(keymap, NULL);
    assert(sm);
    struct xkb_event_iterator *events =
        xkb_event_iterator_new(ctx, XKB_EVENT_ITERATOR_NO_FLAGS);
    assert(events);
    state = xkb_state_new(keymap);
    assert(state);

    bench_start2(&bench);
    bench_state_machine_api(sm, events, state);
    bench_stop2(&bench);

    xkb_state_unref(state);
    xkb_event_iterator_destroy(events);
    xkb_state_machine_unref(sm);

    bench_elapsed(&bench, &elapsed);
    average = (bench_time_elapsed_nanoseconds(&elapsed)) / BENCHMARK_ITERATIONS;
    elapsed_str = bench_elapsed_str(&bench);
    fprintf(stdout, "Full   state machine API: average=%ldns, %d iterations in %ss\n",
            average, BENCHMARK_ITERATIONS, elapsed_str);
    free(elapsed_str);

    xkb_keymap_unref(keymap);
    xkb_context_unref(ctx);

    return EXIT_SUCCESS;
}
