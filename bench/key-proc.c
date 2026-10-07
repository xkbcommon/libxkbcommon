/*
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include "xkbcommon/xkbcommon.h"
#include "bench.h"
#include "src/typing.h"
#include "test/test.h"
#include "tools/tools-common.h"
#include "utils.h"
#include "util-random.h"

#define DEFAULT_ITERATIONS 6000000
#define DEFAULT_WARM_UP (DEFAULT_ITERATIONS / 100)
#define DEFAULT_STDEV 0.05

enum api {
    API_NONE = 0,
    API_LEGACY = (1u << 0),
    API_MODERN = (1u << 1),
    API_ALL = API_LEGACY | API_MODERN,
};

// NOLINTBEGIN(readability-enum-initial-value)
enum typing_mode {
    TYPING_MODE_SYNTHETIC,
    TYPING_MODE_REALISTIC,
    _NUM_TYPING_MODE,
    TYPING_MODE_UNKNOWN = _NUM_TYPING_MODE,
    TYPING_MODE_DEFAULT = TYPING_MODE_REALISTIC,
};
// NOLINTEND(readability-enum-initial-value)

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
           " --stdev[=STDEV]\n"
           "    Target relative standard deviation (percentage) to reach\n"
           "    (maximum acceptable; default: %f)\n"
           " --seed SEED\n"
           "    Seed for the pseudo-random generator\n"
           " --legacy\n"
           "    Bench legacy server API (xkb_state)\n"
           " --modern\n"
           "    Bench modern server API (xkb_machine)\n"
           " --realistic-typing\n"
           "    Bench realistic typing: generates key sequences using realistic\n"
           "    proportions for character, modifier, and misc keys, with\n"
           "    stateful modifier mechanics (set, latch, lock).\n"
           " --synthetic-typing\n"
           "    Bench synthetic typing: generates uniform random key sequences\n"
           "    without realistic category weighting or modifier state flows.\n"
           "Mutually exclusive groups:\n"
           " * --iter, --stdev\n"
           " * --realistic-typing, --synthetic-typing\n"
           "\n",
           argv[0], DEFAULT_WARM_UP, DEFAULT_ITERATIONS, DEFAULT_STDEV * 100);
}

static void
parse_args(int argc, char **argv, unsigned int *warm_up_iter,
           unsigned int *max_iterations, double *stdev,
           unsigned int *seed, enum api *api, enum typing_mode *typing_mode)
{
    enum options {
        OPT_WARM_UP,
        OPT_ITERATIONS,
        OPT_STDEV,
        OPT_SEED,
        OPT_LEGACY_API,
        OPT_MODERN_API,
        OPT_REALISTIC_TYPING,
        OPT_SYNTHETIC_TYPING,
    };

    static struct option opts[] = {
        {"help",             no_argument,            0, 'h'},
        {"warm-up",          required_argument,      0, OPT_WARM_UP},
        {"iter",             optional_argument,      0, OPT_ITERATIONS},
        {"stdev",            optional_argument,      0, OPT_STDEV},
        {"seed",             required_argument,      0, OPT_SEED},
        {"legacy",           no_argument,            0, OPT_LEGACY_API},
        {"modern",           no_argument,            0, OPT_MODERN_API},
        {"realistic-typing", no_argument,            0, OPT_REALISTIC_TYPING},
        {"synthetic-typing", no_argument,            0, OPT_SYNTHETIC_TYPING},
        {0, 0, 0, 0},
    };

    bool explicit_iterations = false;

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
            if (*max_iterations == 0)
                goto mutually_exclusive_iter_stdev;

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
            explicit_iterations = true;
            break;
        }
        case OPT_STDEV: {
            if (explicit_iterations)
                goto mutually_exclusive_iter_stdev;

            /* Accept `--stdev 100` in addition to `--stdev=100` */
            if (!optarg && optind < argc && argv[optind][0] != '-')
                optarg = argv[optind++];
            if (optarg) {
                errno = 0;
                char *endp = optarg;
                *stdev = strtod(optarg, &endp) / 100;
                if (errno || optarg == endp || *endp != '\0' || *stdev <= 0 ||
                    !isfinite(*stdev))
                {
                    fprintf(stderr, "ERROR: invalid 'stdev' parameter\n");
                    usage(stderr, argv);
                    exit(EXIT_INVALID_USAGE);
                }
            } else {
                *stdev = DEFAULT_STDEV;
            }
            *max_iterations = 0;
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
        case OPT_REALISTIC_TYPING:
            if (*typing_mode == TYPING_MODE_SYNTHETIC)
                goto mutually_exclusive_type_mode;
            *typing_mode = TYPING_MODE_REALISTIC;
            break;
        case OPT_SYNTHETIC_TYPING:
            if (*typing_mode == TYPING_MODE_REALISTIC)
                goto mutually_exclusive_type_mode;
            *typing_mode = TYPING_MODE_SYNTHETIC;
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

    if (*typing_mode == TYPING_MODE_UNKNOWN)
        *typing_mode = TYPING_MODE_DEFAULT;

    return;

mutually_exclusive_iter_stdev:
    fprintf(stderr, "ERROR: --iter and --stdev are mutually exclusive\n");
    usage(stderr, argv);
    exit(EXIT_INVALID_USAGE);

mutually_exclusive_type_mode:
    fprintf(stderr,
            "ERROR: --synthetic-typing and --realistic-typing "
            "are mutually exclusive\n");
    usage(stderr, argv);
    exit(EXIT_INVALID_USAGE);
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

static void
report_stdev(unsigned int iterations,
             double stdev,
             const struct bench *bench,
             const struct bench_time *elapsed,
             const struct estimate *est)
{
    struct bench_time total_elapsed;
    bench_elapsed(bench, &total_elapsed);
    fprintf(stdout,
            "mean: %lld ns; stdev: %Lf%% (target: %f%%); "
            "last run: processed %u input events in %ld.%06lds; "
            "total time: %ld.%06lds\n",
            est->elapsed,
            (long double) est->stdev * 100.0 / (long double) est->elapsed,
            stdev * 100.0, iterations,
            elapsed->seconds, elapsed->nanoseconds / 1000,
            total_elapsed.seconds, total_elapsed.nanoseconds / 1000);
}

static unsigned long
bench_legacy_api_loop(struct xkb_typing_event key, struct xkb_state *state)
{
    const enum xkb_state_component changed =
        xkb_state_update_key(state, key.keycode, key.direction);
    unsigned long acc = (unsigned long)changed;

    if (key.direction == XKB_KEY_DOWN) {
        const xkb_keysym_t keysym =
            xkb_state_key_get_one_sym(state, key.keycode);
        acc += (unsigned long)keysym;
    }

    return acc;
}

static void
bench_legacy_api(bool warm_up, unsigned int max_iterations, double stdev,
                 const struct xkb_typing_input * restrict input,
                 struct xkb_keymap *keymap)
{
    struct xkb_state *state = xkb_state_new(keymap);
    if (!state)
        exit(EXIT_FAILURE);

    struct bench bench;
    struct bench_time elapsed;
    struct estimate est;
    volatile unsigned long acc = 0;
    size_t input_idx = 0;
    const size_t input_size = input->num_events;

    if (max_iterations) {
        bench_start2(&bench);
        for (size_t i = 0; i < max_iterations; i++) {
            const struct xkb_typing_event key = input->events[input_idx];
            acc += bench_legacy_api_loop(key, state);
            /* Wrap input */
            input_idx = (input_idx + 1 == input_size)
                      ? 0
                      : input_idx + 1;
        }
        bench_stop2(&bench);
        bench_elapsed(&bench, &elapsed);
        est.elapsed = bench_time_elapsed_nanoseconds(&elapsed) / max_iterations;
        est.stdev = 0; /* unused */
        if (!warm_up) {
            report_iterations(max_iterations, &bench, &est);
        }
    } else {
        bench_start2(&bench);
        BENCH(stdev, max_iterations, elapsed, est, input_idx = 0,
            const struct xkb_typing_event key = input->events[input_idx];
            acc += bench_legacy_api_loop(key, state);
            /* Wrap input */
            input_idx = (input_idx + 1 == input_size)
                      ? 0
                      : input_idx + 1;
        );
        bench_stop2(&bench);
        if (!warm_up) {
            report_stdev(max_iterations, stdev, &bench, &elapsed, &est);
        }
    }

    (void)acc;

    xkb_state_unref(state);
}

static unsigned long
bench_modern_api_loop(const struct xkb_typing_event key,
                      struct xkb_machine *sm,
                      struct xkb_events *events,
                      struct xkb_state *state)
{
    unsigned long acc = 0;
    const enum xkb_status ret =
        xkb_machine_process_key(sm, key.keycode, key.direction, events);
    acc += (unsigned long)ret;

    const struct xkb_event *event;
    while ((event = xkb_events_next(events))) {
        enum xkb_state_component changed = 0;
        (void)xkb_state_update_event(state, event, &changed);
        acc += (unsigned long)changed;
    }

    if (key.direction == XKB_KEY_DOWN) {
        const xkb_keysym_t keysym =
            xkb_state_key_get_one_sym(state, key.keycode);
        acc += (unsigned long)keysym;
    }

    return acc;
}

static void
bench_modern_api(bool warm_up, unsigned int max_iterations, double stdev,
                 const struct xkb_typing_input * restrict input,
                 struct xkb_context *ctx,
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
    volatile unsigned long acc = 0;
    size_t input_idx = 0;
    const size_t input_size = input->num_events;

    if (max_iterations) {
        bench_start2(&bench);
        for (size_t i = 0; i < max_iterations; i++) {
            const struct xkb_typing_event key = input->events[input_idx];
            acc += bench_modern_api_loop(key, sm, events, state);
            /* Wrap input */
            input_idx = (input_idx + 1 == input_size)
                      ? 0
                      : input_idx + 1;
        }
        bench_stop2(&bench);
        bench_elapsed(&bench, &elapsed);
        est.elapsed = bench_time_elapsed_nanoseconds(&elapsed) / max_iterations;
        est.stdev = 0; /* unused */
        if (!warm_up) {
            report_iterations(max_iterations, &bench, &est);
        }
    } else {
        bench_start2(&bench);
        BENCH(stdev, max_iterations, elapsed, est, input_idx = 0,
            const struct xkb_typing_event key = input->events[input_idx];
            acc += bench_modern_api_loop(key, sm, events, state);
            /* Wrap input */
            input_idx = (input_idx + 1 == input_size)
                      ? 0
                      : input_idx + 1;
        );
        bench_stop2(&bench);
        if (!warm_up) {
            report_stdev(max_iterations, stdev, &bench, &elapsed, &est);
        }
    }

    (void)acc;

    xkb_state_unref(state);
    xkb_events_destroy(events);
    xkb_machine_unref(sm);
}

static long
prng(void *state)
{
    (void)state;
    return random();
}

int
main(int argc, char **argv)
{
    unsigned int warm_up_iter = DEFAULT_WARM_UP;
    unsigned int max_iterations = DEFAULT_ITERATIONS;
    double stdev = DEFAULT_STDEV;
    unsigned int seed = (unsigned int)time(NULL);
    enum api api = API_NONE;
    enum typing_mode typing_mode = TYPING_MODE_UNKNOWN;

    parse_args(argc, argv, &warm_up_iter, &max_iterations,
               &stdev, &seed, &api, &typing_mode);

    struct xkb_context *ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!ctx)
        exit(EXIT_FAILURE);

    /*
     * Compile keymap
     */

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

    /*
     * Prepare key event sample
     */

    fprintf(stdout, "Seed: %u\n", seed);
    srandom(seed);

    enum {
        /** Small sample to fit 50% of 512KiB L2 cache */
        DEFAULT_SAMPLE_SIZE = 0x40000 / sizeof(struct xkb_typing_event)
    };
    struct xkb_key_set set;
    enum xkb_status status = xkb_key_set_init(&set, keymap, NULL);
    if (status != XKB_SUCCESS)
        exit(EXIT_FAILURE);

    struct xkb_typing_input input;

    switch (typing_mode) {
    case TYPING_MODE_SYNTHETIC: {
        input.num_events = DEFAULT_SAMPLE_SIZE;
        input.events = calloc(DEFAULT_SAMPLE_SIZE, sizeof(*input.events));
        if (!input.events)
            exit(EXIT_FAILURE);
        enum { KEY_COUNT = 256 };
        bool keys[KEY_COUNT] = { 0 };
        const xkb_keycode_t min = MAX(8, xkb_keymap_min_keycode(keymap));
        const xkb_keycode_t max =
            MIN(KEY_COUNT - 1, xkb_keymap_max_keycode(keymap));
        size_t down = 0;
        xkb_keycode_t keycode = min;
        for (size_t e = 0; e < input.num_events; e++) {
            if (input.num_events - e <= down) {
                /* Only enough room left to release held keys */
                if (++keycode > max)
                    keycode = min;
                while (!keys[keycode]) keycode++;
            } else {
                keycode = (random() % (max - min + 1)) + min;
            }
            const bool is_down = keys[keycode];
            const enum xkb_key_direction direction = is_down
                ? XKB_KEY_UP
                : XKB_KEY_DOWN;
            input.events[e] = (struct xkb_typing_event) {
                .keycode = keycode,
                .direction = direction,
            };
            keys[keycode] = !is_down;
            down += is_down ? -1 : 1;
        }
        break;
    }
    case TYPING_MODE_REALISTIC: {
        struct xkb_typing_input_config input_config = {
            .prng = &prng,
            .prng_state = NULL,
            .input_length = DEFAULT_SAMPLE_SIZE,
        };
        status = xkb_typing_input_init(&input, &set, &input_config);
        xkb_key_set_destroy(&set);
        if (status != XKB_SUCCESS)
            exit(EXIT_FAILURE);
        break;
    }
    default: {
        static_assert(TYPING_MODE_REALISTIC == 1 &&
                      TYPING_MODE_REALISTIC == _NUM_TYPING_MODE - 1, "");
    }}

    /*
     * Run the benchmark
     */

    xkb_enable_quiet_logging(ctx);

    if (api & API_LEGACY) {
        fprintf(stdout, "--- Legacy server API ---\n");
        if (warm_up_iter) {
            fprintf(stdout, "Warm-up: %u iterations...\n", warm_up_iter);
            bench_legacy_api(true, warm_up_iter, 0, &input, keymap);
        }
        fprintf(stdout, "Benchmarking...\n");
        bench_legacy_api(false, max_iterations, stdev, &input, keymap);
    }
    if (api & API_MODERN) {
        fprintf(stdout, "--- Modern server API ---\n");
        if (warm_up_iter) {
            fprintf(stdout, "Warm-up: %u iterations...\n", warm_up_iter);
            bench_modern_api(true, warm_up_iter, 0, &input, ctx, keymap);
        }
        fprintf(stdout, "Benchmarking...\n");
        bench_modern_api(false, max_iterations, stdev, &input, ctx, keymap);
    }

    xkb_typing_input_destroy(&input);
    xkb_keymap_unref(keymap);
    xkb_context_unref(ctx);

    return EXIT_SUCCESS;
}
