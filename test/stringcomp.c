/*
 * Copyright © 2009 Dan Nicholson
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "test.h"
#include "test/utils-text.h"

#define DATA_PATH "keymaps/stringcomp.data"

static void
test_explicit_actions(struct xkb_context *ctx)
{
    struct xkb_keymap *keymap;

    char *original = test_read_file("keymaps/explicit-actions.xkb");
    assert(original);
    char *tmp = uncomment(original, strlen(original), "//");
    assert(tmp);
    char *expected = strip_lines(tmp, strlen(tmp), "//");
    free(tmp);
    assert(expected);
    char *got = NULL;

    /* Try original */
    keymap = test_compile_string(ctx, original);
    free(original);
    assert(keymap);
    got = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_USE_ORIGINAL_FORMAT);
    xkb_keymap_unref(keymap);
    assert_streq_not_null("Check output from original", expected, got);
    free(got);

    /* Try round-trip */
    keymap = test_compile_string(ctx, expected);
    assert(keymap);
    got = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_USE_ORIGINAL_FORMAT);
    xkb_keymap_unref(keymap);
    assert_streq_not_null("Check roundtrip", expected, got);
    free(got);

    free(expected);
}

static struct xkb_keymap*
compile_string(struct xkb_context *context,
               const char *buf, size_t len, void *private)
{
    return test_compile_string(context, buf);
}

int
main(int argc, char *argv[])
{
    test_init();

    bool update_output_files = false;
    if (argc > 1) {
        if (streq(argv[1], "update")) {
            /* Update files with *obtained* results */
            update_output_files = true;
        } else {
            fprintf(stderr, "ERROR: unsupported argument: \"%s\".\n", argv[1]);
            exit(EXIT_FAILURE);
        }
    }

    struct xkb_context *ctx = test_get_context(CONTEXT_NO_FLAG);
    struct xkb_keymap *keymap;
    char *dump, *dump2;

    assert(ctx);

    /* Make sure we can't (falsely claim to) compile an empty string. */
    keymap = test_compile_string(ctx, "");
    assert(!keymap);

    /* Load in a prebuilt keymap, make sure we can compile it from a string,
     * then compare it to make sure we get the same result when dumping it
     * to a string. */
    char *original = test_read_file(DATA_PATH);
    assert(original);
    assert(test_compile_output(ctx, compile_string, NULL, "Round-trip",
                               original, 0 /* unused */, DATA_PATH,
                               update_output_files));
    free(original);

    /* Make sure we can recompile our output for a normal keymap from rules. */
    keymap = test_compile_rules(ctx, NULL, NULL,
                                "ru,ca,de,us", ",multix,neo,intl", NULL);
    assert(keymap);
    dump = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_USE_ORIGINAL_FORMAT);
    assert(dump);
    xkb_keymap_unref(keymap);
    keymap = test_compile_string(ctx, dump);
    assert(keymap);
    /* Now test that the dump of the dump is equal to the dump! */
    dump2 = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_USE_ORIGINAL_FORMAT);
    assert(dump2);
    assert(streq(dump, dump2));

    /* Test response to invalid formats and flags. */
    assert(!xkb_keymap_new_from_string(ctx, dump, 0, 0));
    assert(!xkb_keymap_new_from_string(ctx, dump, -1, 0));
    assert(!xkb_keymap_new_from_string(ctx, dump, XKB_KEYMAP_FORMAT_TEXT_V1+1, 0));
    assert(!xkb_keymap_new_from_string(ctx, dump, XKB_KEYMAP_FORMAT_TEXT_V1, -1));
    assert(!xkb_keymap_new_from_string(ctx, dump, XKB_KEYMAP_FORMAT_TEXT_V1, 1414));
    assert(!xkb_keymap_get_as_string(keymap, 0));
    assert(!xkb_keymap_get_as_string(keymap, 4893));

    xkb_keymap_unref(keymap);
    free(dump);
    free(dump2);

    test_explicit_actions(ctx);

    xkb_context_unref(ctx);

    return EXIT_SUCCESS;
}
