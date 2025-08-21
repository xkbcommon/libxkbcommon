/*
 * Copyright © 2023 Pierre Le Marre
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <string.h>
#include <time.h>

#include "xkbcommon/xkbcommon-compose.h"

#include "../test/compose-iter.h"
#include "../test/test.h"
#include "bench.h"

#define BENCHMARK_ITERATIONS 1000

static void
compose_fn(struct xkb_compose_table_entry *entry, void *data)
{
    assert (entry);
}

/* Benchmark compose traversal using:
 * • the internal recursive function `xkb_compose_table_for_each` if `foreach` is
 *   is passed as argument to the program;
 * • else the iterator API (`xkb_compose_table_iterator_new`, …).
 */
int
main(int argc, char *argv[])
{
    struct xkb_context *ctx;
    char *path;
    FILE *file;
    struct xkb_compose_table *table;
    struct bench bench;
    char *elapsed;

    bool use_foreach_impl = (argc > 1 && strcmp(argv[1], "foreach") == 0);

    ctx = test_get_context(CONTEXT_NO_FLAG);
    assert(ctx);

    path = test_get_path("locale/en_US.UTF-8/Compose");
    file = fopen(path, "rb");
    if (file == NULL) {
        perror(path);
        free(path);
        xkb_context_unref(ctx);
        return -1;
    }
    free(path);

    xkb_enable_quiet_logging(ctx);

    table = xkb_compose_table_new_from_file(ctx, file, "",
                                            XKB_COMPOSE_FORMAT_TEXT_V1,
                                            XKB_COMPOSE_COMPILE_NO_FLAGS);
    fclose(file);
    assert(table);

    bench_start(&bench);
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        if (use_foreach_impl) {
            xkb_compose_table_for_each(table, compose_fn, NULL);
        } else {
            struct xkb_compose_table_iterator *iter;
            struct xkb_compose_table_entry *entry;
            iter = xkb_compose_table_iterator_new(table);
            while ((entry = xkb_compose_table_iterator_next(iter))) {
                assert (entry);
            }
            xkb_compose_table_iterator_free(iter);
        }
    }
    bench_stop(&bench);

    xkb_compose_table_unref(table);

    elapsed = bench_elapsed_str(&bench);
    fprintf(stderr, "traversed %d compose tables in %ss\n",
            BENCHMARK_ITERATIONS, elapsed);
    free(elapsed);

    xkb_context_unref(ctx);
    return 0;
}
