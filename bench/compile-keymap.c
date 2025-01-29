/*
 * Copyright © 2024 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <fcntl.h>
#include <time.h>
#include <getopt.h>

#include "xkbcommon/xkbcommon.h"
#include "utils.h"

#include "bench.h"

#define DEFAULT_ITERATIONS 3000
#define DEFAULT_STDEV 0.05

static void
usage(char **argv)
{
    printf("Usage: %s [OPTIONS]\n"
           "\n"
           "Benchmark compilation of the given RMLVO\n"
           "\n"
           "Options:\n"
           " --help\n"
           "    Print this help and exit\n"
           " --iter\n"
           "    Exact number of iterations to run\n"
           " --stdev\n"
           "    Minimal relative standard deviation (percentage) to reach.\n"
           "    (default: %f)\n"
           "Note: --iter and --stdev are mutually exclusive.\n"
           "\n"
           "XKB-specific options:\n"
           " --rules <rules>\n"
           "    The XKB ruleset (default: '%s')\n"
           " --model <model>\n"
           "    The XKB model (default: '%s')\n"
           " --layout <layout>\n"
           "    The XKB layout (default: '%s')\n"
           " --variant <variant>\n"
           "    The XKB layout variant (default: '%s')\n"
           " --options <options>\n"
           "    The XKB options (default: '%s')\n"
           "\n",
           argv[0], DEFAULT_STDEV * 100, DEFAULT_XKB_RULES,
           DEFAULT_XKB_MODEL, DEFAULT_XKB_LAYOUT,
           DEFAULT_XKB_VARIANT ? DEFAULT_XKB_VARIANT : "<none>",
           DEFAULT_XKB_OPTIONS ? DEFAULT_XKB_OPTIONS : "<none>");
}

int
main(int argc, char **argv)
{
    struct xkb_context *context;
    struct bench bench;
    struct bench_time elapsed;
    struct estimate est;
    bool explicit_iterations = false;
    int ret = 0;
    struct xkb_rule_names rmlvo = {
        .rules = DEFAULT_XKB_RULES,
        .model = DEFAULT_XKB_MODEL,
        /* layout and variant are tied together, so we either get user-supplied for
         * both or default for both, see below */
        .layout = NULL,
        .variant = NULL,
        .options = DEFAULT_XKB_OPTIONS,
    };
    unsigned int max_iterations = DEFAULT_ITERATIONS;
    double stdev = DEFAULT_STDEV;

    enum options {
        OPT_RULES,
        OPT_MODEL,
        OPT_LAYOUT,
        OPT_VARIANT,
        OPT_OPTION,
        OPT_ITERATIONS,
        OPT_STDEV,
    };

    static struct option opts[] = {
        {"help",             no_argument,            0, 'h'},
        {"rules",            required_argument,      0, OPT_RULES},
        {"model",            required_argument,      0, OPT_MODEL},
        {"layout",           required_argument,      0, OPT_LAYOUT},
        {"variant",          required_argument,      0, OPT_VARIANT},
        {"options",          required_argument,      0, OPT_OPTION},
        {"iter",             required_argument,      0, OPT_ITERATIONS},
        {"stdev",            required_argument,      0, OPT_STDEV},
        {0, 0, 0, 0},
    };

    while (1) {
        int c;
        int option_index = 0;
        c = getopt_long(argc, argv, "h", opts, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'h':
            usage(argv);
            exit(EXIT_SUCCESS);
        case OPT_RULES:
            rmlvo.rules = optarg;
            break;
        case OPT_MODEL:
            rmlvo.model = optarg;
            break;
        case OPT_LAYOUT:
            rmlvo.layout = optarg;
            break;
        case OPT_VARIANT:
            rmlvo.variant = optarg;
            break;
        case OPT_OPTION:
            rmlvo.options = optarg;
            break;
        case OPT_ITERATIONS:
            if (max_iterations == 0) {
                usage(argv);
                exit(EXIT_INVALID_USAGE);
            }
            {
                const int max_iterations_raw = atoi(optarg);
                if (max_iterations_raw <= 0)
                    max_iterations = DEFAULT_ITERATIONS;
                else
                    max_iterations = (unsigned int) max_iterations_raw;
            }
            explicit_iterations = true;
            break;
        case OPT_STDEV:
            if (explicit_iterations) {
                usage(argv);
                exit(EXIT_INVALID_USAGE);
            }
            stdev = atof(optarg) / 100;
            if (stdev <= 0)
                stdev = DEFAULT_STDEV;
            max_iterations = 0;
            break;
        default:
            usage(argv);
            exit(EXIT_INVALID_USAGE);
        }
    }

    /* Now fill in the layout */
    if (!rmlvo.layout || !*rmlvo.layout) {
        if (rmlvo.variant && *rmlvo.variant) {
            fprintf(stderr, "Error: a variant requires a layout\n");
            return EXIT_INVALID_USAGE;
        }
        rmlvo.layout = DEFAULT_XKB_LAYOUT;
        rmlvo.variant = DEFAULT_XKB_VARIANT;
    }

    context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!context)
        exit(1);

    struct xkb_keymap *keymap;

    keymap = xkb_keymap_new_from_names(context, &rmlvo, XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!keymap) {
        fprintf(stderr, "ERROR: Cannot compile keymap.\n");
        goto keymap_error;
        exit(EXIT_FAILURE);
    }
#ifndef KEYMAP_DUMP
    char *keymap_str = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_FORMAT_TEXT_V1);
    xkb_keymap_unref(keymap);
#endif

    /* Suspend stdout and stderr outputs */
    fflush(stdout);
    int stdout_old = dup(STDOUT_FILENO);
    int stdout_new = open("/dev/null", O_WRONLY);
    if (stdout_old == -1 || stdout_new == -1 ||
        dup2(stdout_new, STDOUT_FILENO) == -1) {
        perror("Stdout error");
        exit(EXIT_FAILURE);
    }
    close(stdout_new);
    fflush(stderr);
    int stderr_old = dup(STDERR_FILENO);
    int stderr_new = open("/dev/null", O_WRONLY);
    if (stderr_old == -1 || stderr_new == -1 ||
        dup2(stderr_new, STDERR_FILENO) == -1) {
        perror("Stderr error");
        exit(EXIT_FAILURE);
    }
    close(stderr_new);

    if (explicit_iterations) {
        stdev = 0;
        bench_start2(&bench);
        for (unsigned int i = 0; i < max_iterations; i++) {
#ifndef KEYMAP_DUMP
            keymap = xkb_keymap_new_from_string(
                context, keymap_str,
                XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
            assert(keymap);
            xkb_keymap_unref(keymap);
#else
            char *s = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_USE_ORIGINAL_FORMAT);
            assert(s);
            free(s);
#endif
        }
        bench_stop2(&bench);

        bench_elapsed(&bench, &elapsed);
        est.elapsed = (bench_time_elapsed_nanoseconds(&elapsed)) / max_iterations;
        est.stdev = 0;
    } else {
        bench_start2(&bench);
#ifndef KEYMAP_DUMP
        BENCH(stdev, max_iterations, elapsed, est,
            keymap = xkb_keymap_new_from_string(
                context, keymap_str,
                XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
            assert(keymap);
            xkb_keymap_unref(keymap);
        );
#else
        BENCH(stdev, max_iterations, elapsed, est,
            char *s = xkb_keymap_get_as_string(
                        keymap, XKB_KEYMAP_USE_ORIGINAL_FORMAT);
            assert(s);
            free(s);
        );
#endif
        bench_stop2(&bench);
    }

    /* Restore stdout and stderr outputs */
    fflush(stdout);
    dup2(stdout_old, STDOUT_FILENO);
    close(stdout_old);
    fflush(stderr);
    dup2(stderr_old, STDERR_FILENO);
    close(stderr_old);

#ifndef KEYMAP_DUMP
    free(keymap_str);
#else
    xkb_keymap_unref(keymap);
#endif

    struct bench_time total_elapsed;
    bench_elapsed(&bench, &total_elapsed);
    if (explicit_iterations) {
        fprintf(stderr,
                "mean: %lld µs; compiled %u keymaps in %ld.%06lds\n",
                est.elapsed / 1000, max_iterations,
                total_elapsed.seconds, total_elapsed.nanoseconds / 1000);
    } else {
        fprintf(stderr,
                "mean: %lld µs; stdev: %Lf%% (target: %f%%); "
                "last run: compiled %u keymaps in %ld.%06lds; "
                "total time: %ld.%06lds\n", est.elapsed / 1000,
                (long double) est.stdev * 100.0 / (long double) est.elapsed,
                stdev * 100,
                max_iterations, elapsed.seconds, elapsed.nanoseconds / 1000,
                total_elapsed.seconds, total_elapsed.nanoseconds / 1000);
    }

keymap_error:
    xkb_context_unref(context);
    return ret;
}
