/*
 * Copyright © 2009 Dan Nicholson
 * SPDX-License-Identifier: MIT
 */

#include "config.h"
#include "test-config.h"

#include "test.h"
#include "xkbcommon/xkbcommon.h"

static int
test_file(struct xkb_context *ctx, const char *path_rel)
{
    struct xkb_keymap *keymap =
        test_compile_file( ctx, XKB_KEYMAP_FORMAT_TEXT_V1, path_rel);

    if (!keymap)
        return 0;

    xkb_keymap_unref(keymap);
    return 1;
}

int
main(void)
{
    test_init();

    struct xkb_context *ctx = test_get_context(0);

    assert(test_file(ctx, "keymaps/basic.xkb"));
    assert(test_file(ctx, "keymaps/comprehensive-plus-geom.xkb"));
    assert(test_file(ctx, "keymaps/no-types.xkb"));
    assert(test_file(ctx, "keymaps/quartz.xkb"));
    assert(test_file(ctx, "keymaps/no-aliases.xkb"));
    assert(test_file(ctx, "keymaps/modmap-none.xkb"));
    assert(test_file(ctx, "keymaps/invalid-escape-sequence.xkb"));

    assert(!test_file(ctx, "keymaps/divide-by-zero.xkb"));
    assert(!test_file(ctx, "keymaps/syntax-error.xkb"));
    assert(!test_file(ctx, "keymaps/syntax-error2.xkb"));
    assert(!test_file(ctx, "keymaps/empty-symbol-decl.xkb"));
    assert(!test_file(ctx, "keymaps/invalid-qualified-type-field.xkb"));
    assert(!test_file(ctx, "keymaps/invalid-qualified-symbols-field.xkb"));
    assert(!test_file(ctx, "does not exist"));

    /* Test response to invalid flags and formats. */
    fclose(stdin);
    assert(!xkb_keymap_new_from_file(ctx, NULL, XKB_KEYMAP_FORMAT_TEXT_V1, 0));
    assert(!xkb_keymap_new_from_file(ctx, stdin, 0, 0));
    assert(!xkb_keymap_new_from_file(ctx, stdin, XKB_KEYMAP_USE_ORIGINAL_FORMAT, 0));
    assert(!xkb_keymap_new_from_file(ctx, stdin, 1234, 0));
    assert(!xkb_keymap_new_from_file(ctx, stdin, XKB_KEYMAP_FORMAT_TEXT_V1, -1));
    assert(!xkb_keymap_new_from_file(ctx, stdin, XKB_KEYMAP_FORMAT_TEXT_V1, 1234));

    xkb_context_unref(ctx);

    return 0;
}
