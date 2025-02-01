/*
 * Copyright © 2023 Pierre Le Marre
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <time.h>
#include <stdbool.h>

#include "xkbcommon/xkbcommon.h"
#include "src/keysym.h"

#include "../test/test.h"
#include "bench.h"

#define BENCHMARK_ITERATIONS 300

typedef uint32_t (*CaseMappingFunc)(xkb_keysym_t ks);
typedef bool (*CaseTestFunc)(xkb_keysym_t ks);
struct TestedFunction {
    union {
        struct {
            CaseMappingFunc toLower;
            CaseMappingFunc toUpper;
        };
        struct {
            CaseTestFunc isLower;
            CaseTestFunc isUpper;
        };
    };
    const char *name;
};

static const struct TestedFunction functions[] = {
    { {.toLower = xkb_keysym_to_lower, .toUpper = xkb_keysym_to_upper},
      "to_lower & to_upper" },
    { {.isLower = xkb_keysym_is_lower, .isUpper = xkb_keysym_is_upper_or_title},
      "is_lower & is_upper" },
};

int
main(void)
{
    struct bench bench;

    for (size_t f = 0; f < ARRAY_SIZE(functions); f++) {
        for (int explicit = 1; explicit >= 0; explicit--) {
            fprintf(stderr, "Benchmarking %s...\n", functions[f].name);
            bench_start(&bench);
            for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
                struct xkb_keysym_iterator *iter = xkb_keysym_iterator_new(explicit);
                while (xkb_keysym_iterator_next(iter)) {
                    xkb_keysym_t ks = xkb_keysym_iterator_get_keysym(iter);
                    functions[f].toLower(ks);
                    functions[f].toUpper(ks);
                }
                iter = xkb_keysym_iterator_unref(iter);
            }
            bench_stop(&bench);

            char *elapsed = bench_elapsed_str(&bench);
            fprintf(stderr,
                    "Applied %d times \"%s\" to %s assigned keysyms in %ss\n",
                    BENCHMARK_ITERATIONS, functions[f].name, explicit ? "explicitly" : "all", elapsed);
            free(elapsed);
        }
    }


    return 0;
}
