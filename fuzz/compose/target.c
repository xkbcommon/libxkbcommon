/*
 * A target program for fuzzing the Compose text format.
 *
 * Currently, just parses an input file, and hopefully doesn't crash or hang.
 */
#include "config.h"

#include <assert.h>

#include "xkbcommon/xkbcommon.h"
#include "xkbcommon/xkbcommon-compose.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    struct xkb_context *ctx;
    struct xkb_compose_table *table;
    ctx = xkb_context_new(XKB_CONTEXT_NO_DEFAULT_INCLUDES); // | XKB_CONTEXT_NO_ENVIRONMENT_NAMES);
    assert(ctx);
    table = xkb_compose_table_new_from_buffer(ctx, (const char*) data, size, "en_US.UTF-8",
                                      XKB_COMPOSE_FORMAT_TEXT_V1,
                                      XKB_COMPOSE_COMPILE_NO_FLAGS);
    xkb_compose_table_unref(table);
    xkb_context_unref(ctx);
    return 0;
}
