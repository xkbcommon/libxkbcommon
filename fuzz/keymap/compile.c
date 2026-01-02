/*
 * A target program for fuzzing the XKB keymap text format.
 *
 * Currently, just parses an input file, and hopefully doesn't crash or hang.
 */
#include "config.h"

#include <assert.h>

#include "xkbcommon/xkbcommon.h"

extern int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    struct xkb_context *ctx;
    struct xkb_keymap *keymap;

    ctx = xkb_context_new(XKB_CONTEXT_NO_DEFAULT_INCLUDES | XKB_CONTEXT_NO_ENVIRONMENT_NAMES);
    assert(ctx);

    keymap = xkb_keymap_new_from_buffer(ctx, (const char*)Data, Size,
                                          XKB_KEYMAP_FORMAT_TEXT_V1,
                                          XKB_KEYMAP_COMPILE_NO_FLAGS);
    xkb_keymap_unref(keymap);
    xkb_context_unref(ctx);

    // DoSomethingInterestingWithMyAPI(Data, Size);
    return 0;  // Values other than 0 and -1 are reserved for future use.
}
