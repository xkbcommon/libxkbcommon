/*
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <errno.h>
#include <getopt.h>
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
#define DEFAULT_WARM_UP (DEFAULT_ITERATIONS / 100)

enum api {
    API_NONE = 0,
    API_LEGACY = (1u << 0),
    API_MODERN = (1u << 1),
    API_ALL = API_LEGACY | API_MODERN,
};

static bool
parse_uint(const char *name, unsigned int min, unsigned int max,
           char *raw, unsigned long *val)
{
    errno = 0;
    char *endp = raw;
    *val = strtoul(raw, &endp, 10);
    if (errno || raw == endp || *endp != '\0' || *val < min || *val > max) {
        fprintf(stderr, "ERROR: invalid '%s' parameter. Valid range: %u..%u.\n",
                name, min, max);
        return false;
    }
    return true;
}

static void
usage(FILE *fp, char **argv)
{
    fprintf(fp, "Usage: %s [OPTIONS]\n"
           "\n"
           "Benchmark key processing\n"
           "\n"
           "Options:\n"
           " --help\n"
           "    Print this help and exit\n"
           " --warm-up WARM_UP\n"
           "    Number of iterations for warm-up\n"
           "    (default: %u)\n"
           " --iter[=ITER]\n"
           "    Exact number of iterations to run\n"
           "    (default: %u)\n"
           " --seed SEED\n"
           "    Seed for the pseudo-random generator\n"
           " --legacy\n"
           "    Bench legacy server API (xkb_state)\n"
           " --modern\n"
           "    Bench modern server API (xkb_machine)\n"
           "\n",
           argv[0], DEFAULT_WARM_UP, DEFAULT_ITERATIONS);
}

static void
parse_args(int argc, char **argv, unsigned int *warm_up_iter,
           unsigned int *max_iterations,
           unsigned int *seed, enum api *api)
{
    enum options {
        OPT_WARM_UP,
        OPT_ITERATIONS,
        OPT_SEED,
        OPT_LEGACY_API,
        OPT_MODERN_API,
    };

    static struct option opts[] = {
        {"help",             no_argument,            0, 'h'},
        {"warm-up",          required_argument,      0, OPT_WARM_UP},
        {"iter",             optional_argument,      0, OPT_ITERATIONS},
        {"seed",             required_argument,      0, OPT_SEED},
        {"legacy",           no_argument,            0, OPT_LEGACY_API},
        {"modern",           no_argument,            0, OPT_MODERN_API},
        {0, 0, 0, 0},
    };

    for (;;) {
        int c;
        int option_idx = 0;
        c = getopt_long(argc, argv, "h", opts, &option_idx);
        if (c == -1)
            break;

        switch (c) {
        case 'h':
            usage(stdout, argv);
            exit(EXIT_SUCCESS);
        case OPT_WARM_UP: {
            unsigned long raw;
            if (!parse_uint("warm-up", 0, UINT_MAX, optarg, &raw)) {
                usage(stderr, argv);
                exit(EXIT_INVALID_USAGE);
            } else {
                *warm_up_iter = (unsigned int)raw;
            }
            break;
        }
        case OPT_ITERATIONS: {
            /* Accept `--iter 100` in addition to `--iter=100` */
            if (!optarg && optind < argc && argv[optind][0] != '-')
                optarg = argv[optind++];
            if (optarg) {
                unsigned long raw;
                if (!parse_uint("iter", 1, UINT_MAX, optarg, &raw)) {
                    usage(stderr, argv);
                    exit(EXIT_INVALID_USAGE);
                } else {
                    *max_iterations = (unsigned int)raw;
                }
            } else {
                *max_iterations = DEFAULT_ITERATIONS;
            }
            break;
        }
        case OPT_SEED: {
            unsigned long raw;
            if (!parse_uint("seed", 0, UINT_MAX, optarg, &raw)) {
                usage(stderr, argv);
                exit(EXIT_INVALID_USAGE);
            } else {
                *seed = (unsigned int)raw;
            }
            break;
        }
        case OPT_LEGACY_API:
            *api |= API_LEGACY;
            break;
        case OPT_MODERN_API:
            *api |= API_MODERN;
            break;
        default:
            usage(stderr, argv);
            exit(EXIT_INVALID_USAGE);
        }
    }

    /* Never silently ignore stray arguments */
    if (optind < argc) {
        fprintf(stderr, "ERROR: unexpected argument '%s'\n", argv[optind]);
        usage(stderr, argv);
        exit(EXIT_INVALID_USAGE);
    }

    if (!*api) {
        *api = API_ALL;
    }
}

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

enum { KEY_COUNT = 256 };

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
bench_legacy_api(bool warm_up, unsigned int max_iterations,
                 unsigned int seed,
                 struct xkb_keymap *keymap)
{
    struct xkb_state *state = xkb_state_new(keymap);
    if (!state)
        exit(EXIT_FAILURE);

    struct bench bench;
    struct bench_time elapsed;
    struct estimate est;
    bool keys[KEY_COUNT] = { 0 };
    volatile unsigned long acc = 0;
    srandom(seed);

    bench_start2(&bench);
    for (size_t i = 0; i < max_iterations; i++) {
        acc += bench_legacy_api_loop(keys, state);
    }
    bench_stop2(&bench);
    bench_elapsed(&bench, &elapsed);
    est.elapsed = bench_time_elapsed_nanoseconds(&elapsed) / max_iterations;
    est.stdev = 0;
    if (!warm_up) {
        report_iterations(max_iterations, &bench, &est);
    }

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
bench_modern_api(bool warm_up, unsigned int max_iterations,
                 unsigned int seed, struct xkb_context *ctx,
                 struct xkb_keymap *keymap)
                 unsigned int seed, struct xkb_context *ctx,
                 struct xkb_keymap *keymap)
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
    srandom(seed);

    bench_start2(&bench);
    for (size_t i = 0; i < max_iterations; i++) {
        acc += bench_modern_api_loop(keys, sm, events, state);
    }
    bench_stop2(&bench);
    bench_elapsed(&bench, &elapsed);
    est.elapsed = bench_time_elapsed_nanoseconds(&elapsed) / max_iterations;
    est.stdev = 0;
    if (!warm_up) {
        report_iterations(max_iterations, &bench, &est);
    }

    (void)acc;

    xkb_state_unref(state);
    xkb_events_destroy(events);
    xkb_machine_unref(sm);
}

int
main(int argc, char **argv)
{
    unsigned int warm_up_iter = DEFAULT_WARM_UP;
    unsigned int max_iterations = DEFAULT_ITERATIONS;
    unsigned int seed = (unsigned int)time(NULL);
    enum api api = API_NONE;

    parse_args(argc, argv, &warm_up_iter, &max_iterations, &seed, &api);

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

    fprintf(stdout, "Seed: %u\n", seed);

    if (api & API_LEGACY) {
        fprintf(stdout, "--- Legacy server API ---\n");
        if (warm_up_iter) {
            fprintf(stdout, "Warm-up: %u iterations...\n", warm_up_iter);
            bench_legacy_api(true, warm_up_iter, seed, keymap);
        }
        fprintf(stdout, "Benchmarking...\n");
        bench_legacy_api(false, max_iterations, seed, keymap);
    }
    if (api & API_MODERN) {
        fprintf(stdout, "--- Modern server API ---\n");
        if (warm_up_iter) {
            fprintf(stdout, "Warm-up: %u iterations...\n", warm_up_iter);
            bench_modern_api(true, warm_up_iter, seed, ctx, keymap);
        }
        fprintf(stdout, "Benchmarking...\n");
        bench_modern_api(false, max_iterations, seed, ctx, keymap);
    }

    xkb_keymap_unref(keymap);
    xkb_context_unref(ctx);

    return EXIT_SUCCESS;
}
