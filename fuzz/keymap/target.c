/*
 * A target program for fuzzing the XKB keymap text format.
 *
 * Currently, just parses an input file, and hopefully doesn't crash or hang.
 */
#include "config.h"

#include <assert.h>

#include "xkbcommon/xkbcommon.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    struct xkb_context *ctx;
    struct xkb_keymap *keymap;
    ctx = xkb_context_new(XKB_CONTEXT_NO_DEFAULT_INCLUDES | XKB_CONTEXT_NO_ENVIRONMENT_NAMES);
    assert(ctx);
    keymap = xkb_keymap_new_from_buffer(ctx, data, size,
                                      XKB_KEYMAP_FORMAT_TEXT_V1,
                                      XKB_KEYMAP_COMPILE_NO_FLAGS);
    xkb_keymap_unref(keymap);
    xkb_context_unref(ctx);
    return 0;
}
    