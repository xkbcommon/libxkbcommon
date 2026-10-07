/*
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include "xkbcommon/xkbcommon.h"
#include "test/test.h"
#include "tools/tools-common.h"
#include "bench.h"
#include "utils.h"
#include "util-random.h"

#define DEFAULT_ITERATIONS 6000000

enum { KEY_COUNT = 256 };

static void
report_iterations(unsigned int iterations,
                  const struct bench *bench,
                  const struct estimate *est)
{
    struct bench_time total_elapsed;
    bench_elapsed(bench, &total_elapsed);
    fprintf(stdout,
            "mean: %lld ns; processed %u input events in %ld.%06lds\n",
            est->elapsed, iterations,
            total_elapsed.seconds, total_elapsed.nanoseconds / 1000);
}

static unsigned long
bench_legacy_api_loop(bool * restrict keys, struct xkb_state *state)
{
    const xkb_keycode_t keycode = (random() % (KEY_COUNT - 1 - 9)) + 9;
    const enum xkb_key_direction direction = (keys[keycode])
                                           ? XKB_KEY_UP : XKB_KEY_DOWN;
    const enum xkb_state_component changed =
        xkb_state_update_key(state, keycode, direction);
    unsigned long acc = (unsigned long)changed;

    keys[keycode] = !keys[keycode];

    if (keys[keycode]) {
        const xkb_keysym_t keysym = xkb_state_key_get_one_sym(state, keycode);
        acc += (unsigned long)keysym;
    }
    return acc;
}

static void
bench_legacy_api(unsigned int max_iterations, struct xkb_keymap *keymap)
{
    struct xkb_state *state = xkb_state_new(keymap);
    if (!state)
        exit(EXIT_FAILURE);

    struct bench bench;
    struct bench_time elapsed;
    struct estimate est;
    bool keys[KEY_COUNT] = { 0 };
    volatile unsigned long acc = 0;

    bench_start2(&bench);
    for (size_t i = 0; i < max_iterations; i++) {
        acc += bench_legacy_api_loop(keys, state);
    }
    bench_stop2(&bench);
    bench_elapsed(&bench, &elapsed);
    est.elapsed = bench_time_elapsed_nanoseconds(&elapsed) / max_iterations;
    est.stdev = 0;
    report_iterations(max_iterations, &bench, &est);

    (void)acc;

    xkb_state_unref(state);
}

static unsigned long
bench_modern_api_loop(bool * restrict keys,
                      struct xkb_machine *sm,
                      struct xkb_events *events,
                      struct xkb_state *state)
{
    unsigned long acc = 0;
    const xkb_keycode_t keycode = (random() % (KEY_COUNT - 1 - 9)) + 9;
    const enum xkb_key_direction direction = (keys[keycode])
                                           ? XKB_KEY_UP : XKB_KEY_DOWN;
    const enum xkb_status ret =
        xkb_machine_process_key(sm, keycode, direction, events);
    acc += (unsigned long)ret;

    const struct xkb_event *event;
    while ((event = xkb_events_next(events))) {
        enum xkb_state_component changed = 0;
        (void)xkb_state_update_event(state, event, &changed);
        acc += (unsigned long)changed;
    }

    keys[keycode] = !keys[keycode];

    if (keys[keycode]) {
        const xkb_keysym_t keysym = xkb_state_key_get_one_sym(state, keycode);
        acc += (unsigned long)keysym;
    }
    return acc;
}

static void
bench_modern_api(unsigned int max_iterations,
                 struct xkb_context *ctx, struct xkb_keymap *keymap)
{
    struct xkb_machine_builder *builder =
        xkb_machine_builder_new(keymap, NULL, NULL);
    if (!builder)
        exit(EXIT_FAILURE);
    struct xkb_machine *sm = xkb_machine_new(builder, NULL);
    if (!sm)
        exit(EXIT_FAILURE);
    xkb_machine_builder_unref(builder);
    struct xkb_events *events = xkb_events_new(ctx, NULL, NULL);
    if (!events)
        exit(EXIT_FAILURE);
    struct xkb_state *state = xkb_state_new(keymap);
    if (!state)
        exit(EXIT_FAILURE);

    struct bench bench;
    struct bench_time elapsed;
    struct estimate est;
    bool keys[KEY_COUNT] = { 0 };
    volatile unsigned long acc = 0;

    bench_start2(&bench);
    for (size_t i = 0; i < max_iterations; i++) {
        acc += bench_modern_api_loop(keys, sm, events, state);
    }
    bench_stop2(&bench);
    bench_elapsed(&bench, &elapsed);
    est.elapsed = bench_time_elapsed_nanoseconds(&elapsed) / max_iterations;
    est.stdev = 0;
    report_iterations(max_iterations, &bench, &est);

    (void)acc;

    xkb_state_unref(state);
    xkb_events_destroy(events);
    xkb_machine_unref(sm);
}

int
main(void)
{
    struct xkb_context *ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!ctx)
        exit(EXIT_FAILURE);

    const enum xkb_keymap_format format = XKB_KEYMAP_FORMAT_TEXT_V1;
    const enum xkb_keymap_compile_flags flags = XKB_KEYMAP_COMPILE_NO_FLAGS;

    struct xkb_keymap *keymap;
    if (is_pipe_or_regular_file(STDIN_FILENO)) {
        fprintf(stdout, "Bench using keymap from stdin\n");
        FILE *file = tools_read_stdin();
        if (!file)
            exit(EXIT_FAILURE);
        keymap = xkb_keymap_new_from_file(ctx, file, format, flags);
    } else {
        fprintf(stdout, "Bench using keymap from fixed RMLVO\n");
        static const struct xkb_rule_names rmlvo = {
            .rules = "evdev",
            .model = "pc104",
            .layout = "us,ru,il,de",
            .variant = ",,,neo",
            .options = "grp:menu_toggle"
        };
        keymap = xkb_keymap_new_from_names2(ctx, &rmlvo, format, flags);
    }
    if (!keymap)
        exit(EXIT_FAILURE);

    xkb_enable_quiet_logging(ctx);

    srandom((unsigned) time(NULL));

    /*
     * Legacy server state machine API
     */
    fprintf(stdout, "--- Legacy server API ---\n");
    bench_legacy_api(DEFAULT_ITERATIONS, keymap);

    /*
     * Full server state machine API
     */
    fprintf(stdout, "--- Modern server API ---\n");
    bench_modern_api(DEFAULT_ITERATIONS, ctx, keymap);

    xkb_keymap_unref(keymap);
    xkb_context_unref(ctx);

    return EXIT_SUCCESS;
}
