/*
 * Copyright © 2014 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <time.h>

#include "xkbcommon/xkbcommon-compose.h"

#include "../test/test.h"
#include "bench.h"

#define BENCHMARK_ITERATIONS 1000

int
main(void)
{
    struct xkb_context *ctx;
    char *path;
    FILE *file;
    struct xkb_compose_table *table;
    struct bench bench;
    char *elapsed;

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

    xkb_enable_quiet_logging(ctx);

    bench_start(&bench);
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        fseek(file, 0, SEEK_SET);
        table = xkb_compose_table_new_from_file(ctx, file, "",
                                                XKB_COMPOSE_FORMAT_TEXT_V1,
                                                XKB_COMPOSE_COMPILE_NO_FLAGS);
        assert(table);
        xkb_compose_table_unref(table);
    }
    bench_stop(&bench);

    fclose(file);
    free(path);

    elapsed = bench_elapsed_str(&bench);
    fprintf(stderr, "compiled %d compose tables in %ss\n",
            BENCHMARK_ITERATIONS, elapsed);
    free(elapsed);

    xkb_context_unref(ctx);
    return 0;
}
