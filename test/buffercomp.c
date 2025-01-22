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

static void
test_recursive(struct xkb_context *ctx)
{
    struct xkb_keymap *keymap;

    const char* const keymaps[] = {
        /* Recursive keycodes */
        "Keycodes: recursive",
        "xkb_keymap {"
        "  xkb_keycodes { include \"evdev+recursive\" };"
        "  xkb_types { include \"complete\" };"
        "  xkb_compat { include \"complete\" };"
        "  xkb_symbols { include \"pc\" };"
        "};",
        "Keycodes: recursive(bar)",
        "xkb_keymap {"
        "  xkb_keycodes { include \"evdev+recursive(bar)\" };"
        "  xkb_types { include \"complete\" };"
        "  xkb_compat { include \"complete\" };"
        "  xkb_symbols { include \"pc\" };"
        "};",
        /* Recursive key types */
        "Key types: recursive",
        "xkb_keymap {"
        "  xkb_keycodes { include \"evdev\" };"
        "  xkb_types { include \"recursive\" };"
        "  xkb_compat { include \"complete\" };"
        "  xkb_symbols { include \"pc\" };"
        "};",
        "Key types: recursive(bar)",
        "xkb_keymap {"
        "  xkb_keycodes { include \"evdev\" };"
        "  xkb_types { include \"recursive(bar)\" };"
        "  xkb_compat { include \"complete\" };"
        "  xkb_symbols { include \"pc\" };"
        "};",
        /* Recursive compat */
        "Compat: recursive",
        "xkb_keymap {"
        "  xkb_keycodes { include \"evdev\" };"
        "  xkb_types { include \"recursive\" };"
        "  xkb_compat { include \"complete\" };"
        "  xkb_symbols { include \"pc\" };"
        "};",
        "Compat: recursive(bar)",
        "xkb_keymap {"
        "  xkb_keycodes { include \"evdev\" };"
        "  xkb_types { include \"complete\" };"
        "  xkb_compat { include \"recursive(bar)\" };"
        "  xkb_symbols { include \"pc\" };"
        "};",
        /* Recursive symbols */
        "Symbols: recursive",
        "xkb_keymap {"
        "  xkb_keycodes { include \"evdev\" };"
        "  xkb_types { include \"complete\" };"
        "  xkb_compat { include \"complete\" };"
        "  xkb_symbols { include \"recursive\" };"
        "};",
        "Symbols: recursive(bar)",
        "xkb_keymap {"
        "  xkb_keycodes { include \"evdev\" };"
        "  xkb_types { include \"complete\" };"
        "  xkb_compat { include \"complete\" };"
        "  xkb_symbols { include \"recursive(bar)\" };"
        "};"
    };

    const size_t len = ARRAY_SIZE(keymaps);

    for (size_t k = 0; k < len; k++) {
        fprintf(stderr, "*** Recursive test: %s ***\n", keymaps[k++]);
        keymap = test_compile_buffer(ctx, keymaps[k], strlen(keymaps[k]));
        assert(!keymap);
    }
}

/* Test various multi-{keysym,action} syntaxes */
static void
test_multi_keysyms_actions(struct xkb_context *ctx)
{
    struct xkb_keymap *keymap;

    /* Macros to define the tests */
#define make_keymap(keysyms, actions)               \
        "xkb_keymap {\n"                            \
        "  xkb_keycodes {\n"                        \
        "    minimum= 8;\n"                         \
        "    maximum= 10;\n"                        \
        "    <AE01> = 10;\n"                        \
        "  };\n"                                    \
        "  xkb_types { include \"basic\" };\n"      \
        "  xkb_compat { include \"basic\" };\n"     \
        "  xkb_symbols {\n"                         \
        "    key <AE01> { " keysyms actions " };\n" \
        "  };\n"                                    \
        "};"
#define make_keymap_with_keysyms(keysyms) \
        make_keymap("[" keysyms "]", "")
#define make_keymap_with_actions(actions) \
        make_keymap("", "actions[1] = [" actions "]")
#define test_keymaps                                    \
        make_keymaps_with(make_keymap_with_keysyms,     \
                          "a", "b", "c", "d"),          \
        make_keymaps_with(make_keymap_with_actions,     \
                          "SetMods(modifiers=Control)", \
                          "SetGroup(group=+1)",         \
                          "Private(data=\"foo\")",      \
                          "Private(data=\"bar\")")
#define run_test(kind, condition, msg, ...) do {                      \
    const char* const keymaps[] = { test_keymaps };                   \
    for (size_t k = 0; k < ARRAY_SIZE(keymaps); k++) {                \
        fprintf(stderr,                                               \
                "*** test_multi_keysyms_actions: " kind "#%zu ***\n", \
                k + 1);                                               \
        keymap = test_compile_buffer(ctx, keymaps[k],                 \
                                     strlen(keymaps[k]));             \
        assert_printf(condition,                                      \
                      "The following symbols " msg " parse:\n%s\n",   \
                      keymaps[k]);                                    \
        __VA_ARGS__;                                                  \
    }                                                                 \
} while (0)

    /* Test: valid keymaps */
#define make_keymaps_with(func, a, b, c, d)      \
        func(a),                                 \
        func(a", "b),                            \
        func("{ "a", "b" }"),                    \
        func("{ "a", "b", "c" }"),               \
        func(a", { "b", "c" }"),                 \
        func("{ "a", "b" }, "c),                 \
        func("{ "a", "b" }, { "c", "d" }"),      \
        func("{ "a", "b" }, "c", { "d", "a" }"), \
        func("{ "a", "b" }, { "c", "d" }, "a)
    run_test("valid", keymap != NULL, "does *not*", xkb_keymap_unref(keymap));
#undef make_keymaps_with

    /* Test: invalid keymaps */
#define make_keymaps_with(func, a, b, c, d)    \
        func("{}"),                            \
        func("{ {} }"),                        \
        func("{ "a" }"),                       \
        func("{ "a", {} }"),                   \
        func("{ {}, "b" }"),                   \
        func("{ {}, {} }"),                    \
        func("{ "a", { "b" } }"),              \
        func("{ { "a" }, "b" }"),              \
        func("{ { "a", "b" }, "c" }"),         \
        func("{ "a", { "b", "c" } }"),         \
        func("{ "a", {}, "c" }"),              \
        func("{ "a", "b", {} }"),              \
        func("{ {}, "b", "c" }"),              \
        func("{ { "a", "b" }, "c", "d" }"),    \
        func("{ "a", { "b", "c" }, "d" }"),    \
        func("{ "a", "b", { "c", "d" } }"),    \
        func("{ { "a", "b" }, { "c", "d" } }")
    run_test("invalid", keymap == NULL, "*does*");
#undef make_keymap
#undef make_keymap_with_actions
#undef make_keymap_with_keysyms
#undef make_keymaps_with
#undef test_keymaps
#undef run_test
}

static void
test_invalid_symbols_fields(struct xkb_context *ctx)
{
    /* Any of the following is invalid syntax, but also use to trigger
     * a NULL pointer deference, thus this regression test */
    const char * const keymaps[] = {
        "xkb_keymap {\n"
        "    xkb_keycodes { <> = 9; };\n"
        "    xkb_types { };\n"
        "    xkb_compat { };\n"
        "    xkb_symbols { key <> { vmods = [] }; };\n"
        "};",
        "xkb_keymap {\n"
        "    xkb_keycodes { <> = 9; };\n"
        "    xkb_types { };\n"
        "    xkb_compat { };\n"
        "    xkb_symbols { key <> { repeat = [] }; };\n"
        "};",
        "xkb_keymap {\n"
        "    xkb_keycodes { <> = 9; };\n"
        "    xkb_types { };\n"
        "    xkb_compat { };\n"
        "    xkb_symbols { key <> { type = [] }; };\n"
        "};",
        "xkb_keymap {\n"
        "    xkb_keycodes { <> = 9; };\n"
        "    xkb_types { };\n"
        "    xkb_compat { };\n"
        "    xkb_symbols { key <> { groupswrap = [] }; };\n"
        "};",
        "xkb_keymap {\n"
        "    xkb_keycodes { <> = 9; };\n"
        "    xkb_types { };\n"
        "    xkb_compat { };\n"
        "    xkb_symbols { key <> { groupsredirect = [] }; };\n"
        "};"
    };
    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        const struct xkb_keymap *keymap =
            test_compile_buffer(ctx, keymaps[k], strlen(keymaps[k]));
        assert(!keymap);
    }
}

int
main(int argc, char *argv[])
{
    test_init();

    struct xkb_keymap *keymap;
    char *original, *dump;

    struct xkb_context *ctx = test_get_context(CONTEXT_NO_FLAG);
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

    test_recursive(ctx);
    test_multi_keysyms_actions(ctx);
    test_invalid_symbols_fields(ctx);

    xkb_context_unref(ctx);

    return EXIT_SUCCESS;
}
