/*
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <getopt.h>
#include <stdlib.h>

#include "xkbcommon/xkbcommon.h"
#include "../test/test.h"
#include "xkbcomp/rules.h"
#include "bench.h"

const unsigned int DEFAULT_ITERATIONS = 20000;
const double       DEFAULT_STDEV = 0.05;

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
main(int argc, char *argv[])
{
    struct bench bench;
    struct bench_time elapsed;
    struct estimate est;
    bool explicit_iterations = false;
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

    struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!context)
        exit(EXIT_FAILURE);

    xkb_context_set_log_level(context, XKB_LOG_LEVEL_CRITICAL);
    xkb_context_set_log_verbosity(context, 0);

    if (explicit_iterations) {
        stdev = 0;
        bench_start2(&bench);
        for (unsigned int i = 0; i < max_iterations; i++) {
            struct xkb_component_names kccgst;

            assert(xkb_components_from_rules_names(context, &rmlvo, &kccgst, NULL));
            free(kccgst.keycodes);
            free(kccgst.types);
            free(kccgst.compatibility);
            free(kccgst.symbols);
            free(kccgst.geometry);
        }
        bench_stop2(&bench);

        bench_elapsed(&bench, &elapsed);
        est.elapsed = (bench_time_elapsed_nanoseconds(&elapsed)) / max_iterations;
        est.stdev = 0;
    } else {
        bench_start2(&bench);
        BENCH(stdev, max_iterations, elapsed, est,
            struct xkb_component_names kccgst;

            assert(xkb_components_from_rules_names(context, &rmlvo, &kccgst, NULL));
            free(kccgst.keycodes);
            free(kccgst.types);
            free(kccgst.compatibility);
            free(kccgst.symbols);
            free(kccgst.geometry);
        );
        bench_stop2(&bench);
    }

    struct bench_time total_elapsed;
    bench_elapsed(&bench, &total_elapsed);
    if (explicit_iterations) {
        fprintf(stderr,
                "mean: %lld µs; compiled %u rules in %ld.%06lds\n",
                est.elapsed / 1000, max_iterations,
                total_elapsed.seconds, total_elapsed.nanoseconds / 1000);
    } else {
        fprintf(stderr,
                "mean: %lld µs; stdev: %Lf%% (target: %f%%); "
                "last run: compiled %u rules in %ld.%06lds; "
                "total time: %ld.%06lds\n", est.elapsed / 1000,
                (long double) est.stdev * 100.0 / (long double) est.elapsed,
                stdev * 100,
                max_iterations, elapsed.seconds, elapsed.nanoseconds / 1000,
                total_elapsed.seconds, total_elapsed.nanoseconds / 1000);
    }

    xkb_context_unref(context);
    return EXIT_SUCCESS;
}
