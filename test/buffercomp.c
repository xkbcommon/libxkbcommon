/*
 * Copyright © 2009 Dan Nicholson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "test.h"

#define DATA_PATH "keymaps/stringcomp.data"

static bool
test_encodings(struct xkb_context *ctx)
{
    struct xkb_keymap *keymap;

    /* Accept UTF-8 encoded BOM (U+FEFF) */
    const char utf8_with_bom[] =
        "\xef\xbb\xbfxkb_keymap {"
        "  xkb_keycodes { include \"evdev\" };"
        "  xkb_types { include \"complete\" };"
        "  xkb_compat { include \"complete\" };"
        "  xkb_symbols { include \"pc\" };"
        "};";
    keymap = test_compile_buffer(ctx, utf8_with_bom, sizeof(utf8_with_bom));
    assert(keymap);
    xkb_keymap_unref(keymap);

    /* Reject UTF-16LE encoded string */
    const char utf16_le[] =
        "x\0k\0b\0_\0k\0e\0y\0m\0a\0p\0 \0{\0\n\0"
        " \0 \0x\0k\0b\0_\0k\0e\0y\0c\0o\0d\0e\0s\0 \0{\0 \0i\0n\0c\0l\0u\0d\0e\0 \0\"\0e\0v\0d\0e\0v\0\"\0 \0}\0;\0\n\0"
        " \0 \0x\0k\0b\0_\0t\0y\0p\0e\0s\0 \0{\0 \0i\0n\0c\0l\0u\0d\0e\0 \0\"\0c\0o\0m\0p\0l\0e\0t\0e\0\"\0 \0}\0;\0\n\0"
        " \0 \0x\0k\0b\0_\0c\0o\0m\0p\0a\0t\0 \0{\0 \0i\0n\0c\0l\0u\0d\0e\0 \0\"\0c\0o\0m\0p\0l\0e\0t\0e\0\"\0 \0}\0;\0\n\0"
        " \0 \0x\0k\0b\0_\0s\0y\0m\0b\0o\0l\0s\0 \0{\0 \0i\0n\0c\0l\0u\0d\0e\0 \0\"\0p\0c\0\"\0 \0}\0;\0\n\0"
        "}\0;\0";
    keymap = test_compile_buffer(ctx, utf16_le, sizeof(utf16_le));
    assert(!keymap);

    /* Reject UTF-16LE with BOM encoded string */
    const char utf16_le_with_bom[] =
        "\xff\xfex\0k\0b\0_\0k\0e\0y\0m\0a\0p\0 \0{\0\n\0"
        " \0 \0x\0k\0b\0_\0k\0e\0y\0c\0o\0d\0e\0s\0 \0{\0 \0i\0n\0c\0l\0u\0d\0e\0 \0\"\0e\0v\0d\0e\0v\0\"\0 \0}\0;\0\n\0"
        " \0 \0x\0k\0b\0_\0t\0y\0p\0e\0s\0 \0{\0 \0i\0n\0c\0l\0u\0d\0e\0 \0\"\0c\0o\0m\0p\0l\0e\0t\0e\0\"\0 \0}\0;\0\n\0"
        " \0 \0x\0k\0b\0_\0c\0o\0m\0p\0a\0t\0 \0{\0 \0i\0n\0c\0l\0u\0d\0e\0 \0\"\0c\0o\0m\0p\0l\0e\0t\0e\0\"\0 \0}\0;\0\n\0"
        " \0 \0x\0k\0b\0_\0s\0y\0m\0b\0o\0l\0s\0 \0{\0 \0i\0n\0c\0l\0u\0d\0e\0 \0\"\0p\0c\0\"\0 \0}\0;\0\n\0"
        "}\0;\0";
    keymap = test_compile_buffer(ctx, utf16_le_with_bom, sizeof(utf16_le_with_bom));
    assert(!keymap);

    /* Reject UTF-16BE encoded string */
    const char utf16_be[] =
        "\0x\0k\0b\0_\0k\0e\0y\0m\0a\0p\0 \0{\0\n\0"
        " \0 \0x\0k\0b\0_\0k\0e\0y\0c\0o\0d\0e\0s\0 \0{\0 \0i\0n\0c\0l\0u\0d\0e\0 \0\"\0e\0v\0d\0e\0v\0\"\0 \0}\0;\0\n\0"
        " \0 \0x\0k\0b\0_\0t\0y\0p\0e\0s\0 \0{\0 \0i\0n\0c\0l\0u\0d\0e\0 \0\"\0c\0o\0m\0p\0l\0e\0t\0e\0\"\0 \0}\0;\0\n\0"
        " \0 \0x\0k\0b\0_\0c\0o\0m\0p\0a\0t\0 \0{\0 \0i\0n\0c\0l\0u\0d\0e\0 \0\"\0c\0o\0m\0p\0l\0e\0t\0e\0\"\0 \0}\0;\0\n\0"
        " \0 \0x\0k\0b\0_\0s\0y\0m\0b\0o\0l\0s\0 \0{\0 \0i\0n\0c\0l\0u\0d\0e\0 \0\"\0p\0c\0\"\0 \0}\0;\0\n\0"
        "}\0;";
    keymap = test_compile_buffer(ctx, utf16_be, sizeof(utf16_be));
    assert(!keymap);

    return true;
}

int
main(int argc, char *argv[])
{
    struct xkb_context *ctx = test_get_context(0);
    struct xkb_keymap *keymap;
    char *original, *dump;

    assert(ctx);

    /* Load in a prebuilt keymap, make sure we can compile it from memory,
     * then compare it to make sure we get the same result when dumping it
     * to a string. */
    original = test_read_file(DATA_PATH);
    assert(original);

    /* Load a prebuild keymap, once without, once with the trailing \0 */
    for (int i = 0; i <= 1; i++) {
        keymap = test_compile_buffer(ctx, original, strlen(original) + i);
        assert(keymap);

        dump = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_USE_ORIGINAL_FORMAT);
        assert(dump);

        if (!streq(original, dump)) {
            fprintf(stderr,
                    "round-trip test failed: dumped map differs from original\n");
            fprintf(stderr, "path to original file: %s\n",
                    test_get_path(DATA_PATH));
            fprintf(stderr, "length: dumped %lu, original %lu\n",
                    (unsigned long) strlen(dump),
                    (unsigned long) strlen(original));
            fprintf(stderr, "dumped map:\n");
            fprintf(stderr, "%s\n", dump);
            fflush(stderr);
            assert(0);
        }

        free(dump);
        xkb_keymap_unref(keymap);
    }

    free(original);

    /* Make sure we can't (falsely claim to) compile an empty string. */
    keymap = test_compile_buffer(ctx, "", 0);
    assert(!keymap);

    assert(test_encodings(ctx));

    /* Make sure we can recompile our output for a normal keymap from rules. */
    keymap = test_compile_rules(ctx, NULL, NULL,
                                "ru,ca,de,us", ",multix,neo,intl", NULL);
    assert(keymap);
    dump = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_USE_ORIGINAL_FORMAT);
    assert(dump);
    xkb_keymap_unref(keymap);
    keymap = test_compile_buffer(ctx, dump, strlen(dump));
    assert(keymap);
    xkb_keymap_unref(keymap);
    free(dump);

    xkb_context_unref(ctx);

    return 0;
}
