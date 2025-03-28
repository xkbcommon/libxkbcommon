/*
 * Copyright © 2009 Dan Nicholson
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "test.h"

#define GOLDEN_TESTS_OUTPUTS "keymaps/"

static void
test_encodings(struct xkb_context *ctx)
{
    struct xkb_keymap *keymap;

    /* Accept UTF-8 encoded BOM (U+FEFF) */
    const char utf8_with_bom[] =
        "\xef\xbb\xbfxkb_keymap {"
        "  xkb_keycodes { };"
        "  xkb_types { };"
        "  xkb_compat { };"
        "  xkb_symbols { };"
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
}

static void
test_component_syntax_error(struct xkb_context *ctx)
{
    /* The following used to trigger memory leak */
    const char* const keymaps[] = {
        "xkb_keymap {"
        "  xkb_keycodes { };"
        "};"
        "};", /* Syntax error, keymap “complete” */
        "xkb_keymap {"
        "  xkb_keycodes { };"
        "  xkb_types { };"
        "  xkb_compat { };"
        "  xkb_symbols { };"
        "};"
        "};"/* Syntax error, keymap complete */,
        "xkb_keymap {"
        "  xkb_keycodes { };"
        "}" /* Syntax error, incomplete map */
        "  xkb_types { };"
        "  xkb_compat { };"
        "  xkb_symbols { };"
        "};",
        "xkb_keymap {"
        "  xkb_keycodes { };"
        ";" /* Syntax error, incomplete map */
        "  xkb_types { };"
        "  xkb_compat { };"
        "  xkb_symbols { };"
        "};",
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        const struct xkb_keymap *keymap =
            test_compile_buffer(ctx, keymaps[k], strlen(keymaps[k]));
        assert(!keymap);
    }
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

    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u %s ***\n", __func__, k, keymaps[k]);
        k++;
        keymap = test_compile_buffer(ctx, keymaps[k], strlen(keymaps[k]));
        assert(!keymap);
    }
}

/* Test some limits related to allocations */
static void
test_alloc_limits(struct xkb_context *ctx)
{
    const char * const keymaps[] = {
        /* Keycodes */
        "xkb_keymap {\n"
        /* Valid keycode value, but we should not handle it
         * with our *continuous* array! */
        "  xkb_keycodes { <> = 0xfffffffe; };\n"
        "  xkb_types { };\n"
        "  xkb_compat { };\n"
        "  xkb_symbols { key <> {[a]}; };\n"
        "};",
        /* Key types */
        "xkb_keymap {\n"
        "  xkb_keycodes { };\n"
        "  xkb_types {\n"
        "    type \"X\" { map[none] = 0xfffffffe; };\n" /* Invalid level index */
        "  };\n"
        "  xkb_compat { };\n"
        "  xkb_symbols { };\n"
        "};",
        "xkb_keymap {\n"
        "  xkb_keycodes { };\n"
        "  xkb_types {\n"
        "    type \"X\" {levelname[0xfffffffe]=\"x\";};\n" /* Invalid level index */
        "  };\n"
        "  xkb_compat { };\n"
        "  xkb_symbols { };\n"
        "};"
    };
    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        const struct xkb_keymap *keymap =
            test_compile_buffer(ctx, keymaps[k], strlen(keymaps[k]));
        assert(!keymap);
    }
}

struct keymap_test_data {
    const char * const keymap;
    const char * const expected;
};

/* Our keymap compiler is the xkbcommon buffer compiler */
static struct xkb_keymap *
compile_buffer(struct xkb_context *context, const char *buf, size_t len,
                     void *private)
{
    return test_compile_buffer(context, buf, len);
}

static void
test_integers(struct xkb_context *ctx, bool update_output_files) {
    const struct keymap_test_data keymaps[] = {
        {
                .keymap =
                    "xkb_keymap {\n"
                    "  xkb_keycodes {\n"
                    /* Out of range (expect 32 bits, got > 64 bits) */
                    "    <> = 0x10000000000000000;\n"
                    "  };\n"
                    "  xkb_types { };\n"
                    "  xkb_compat { };\n"
                    "  xkb_symbols { };\n"
                    "};",
                .expected = NULL
        },
        {
                .keymap =
                    "xkb_keymap {\n"
                    "  xkb_keycodes { <> = 1; };\n"
                    "  xkb_types { };\n"
                    "  xkb_compat {\n"
                    "  };\n"
                    "  xkb_symbols {\n"
                    "    key <> {\n"
                    /* FIXME: Unchecked overflow */
                    "      [MovePointer(x=0xffffffff + 1,\n"
                    "                   y=0x80000000 * 2)]\n"
                    "    };\n"
                    "  };\n"
                    "};",
                .expected = GOLDEN_TESTS_OUTPUTS "integers-overflow.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <> = 1;\n"
                "    indicator 32 = \"xxx\";\n"
                "  };\n"
                "  xkb_types { };\n"
                "  xkb_compat {\n"
                "    group 0xffffffff = Mod5;\n"
                "  };\n"
                "  xkb_symbols {\n"
                /* Computations with 64 bit ints that then fit into 16 bits */
                "    key <> {\n"
                "      actions[1 + -~0x100000001 / 0x100000000]=\n"
                "      [MovePointer(x=0x100000000 - 0xfffffffe,\n"
                "                   y=~-0x7fff * 0x30000 / 0x2ffff)]\n"
                "    };\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "integers.xkb"
        }
    };
    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_compile_output(ctx, compile_buffer, NULL, __func__,
                                   keymaps[k].keymap, strlen(keymaps[k].keymap),
                                   keymaps[k].expected, update_output_files));
    }
}

static void
test_keycodes(struct xkb_context *ctx, bool update_output_files) {
    const struct keymap_test_data keymaps[] = {
        /*
         * Ensure keycodes bound are correctly updated.
         *
         * Since expanding bounds is tested in the rest of the code, we test
         * only shrinking here.
         */

        /* Single keycode */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <A> = 0;\n"
                "    override <A> = 1;\n"
                "    augment  <A> = 300;\n"
                "  };\n"
                "  xkb_types { };\n"
                "  xkb_compat {\n"
                "    interpret.repeat= False;\n"
                "  };\n"
                "  xkb_symbols { };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-bounds-single-1.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <A> = 300;\n"
                "    override <A> = 1;\n"
                "    augment  <A> = 0;\n"
                "  };\n"
                "  xkb_types { };\n"
                "  xkb_compat {\n"
                "    interpret.repeat= False;\n"
                "  };\n"
                "  xkb_symbols { };\n"
                "};",
            /* NOTE: same as previous */
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-bounds-single-1.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <A> = 0;\n"
                "    override <A> = 1;\n"
                "    override <A> = 301;\n"
                "    override <A> = 300;\n"
                "  };\n"
                "  xkb_types { };\n"
                "  xkb_compat {\n"
                "    interpret.repeat= False;\n"
                "  };\n"
                "  xkb_symbols { };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-bounds-single-2.xkb"
        },

        /* Multiple keycodes to single keycode */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <A> = 300;\n"
                "    <B> = 1;\n"
                "    augment  <B> = 301;\n"
                "    override <A> = 1;\n"
                "  };\n"
                "  xkb_types { };\n"
                "  xkb_compat {\n"
                "    interpret.repeat= False;\n"
                "  };\n"
                "  xkb_symbols { };\n"
                "};",
            /* NOTE: same as previous */
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-bounds-single-1.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <A> = 0;\n"
                "    <B> = 1;\n"
                "    augment  <B> = 300;\n"
                "    override <A> = 1;\n"
                "  };\n"
                "  xkb_types { };\n"
                "  xkb_compat {\n"
                "    interpret.repeat= False;\n"
                "  };\n"
                "  xkb_symbols { };\n"
                "};",
            /* NOTE: same as previous */
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-bounds-single-1.xkb"
        },

        /* Multiple keycodes to multiple keycodes */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <A> = 1;\n"
                "    <B> = 0;\n"
                "    override <B> = 300;\n"
                "    augment  <A> = 0;\n"
                "  };\n"
                "  xkb_types { };\n"
                "  xkb_compat {\n"
                "    interpret.repeat= False;\n"
                "  };\n"
                "  xkb_symbols { };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-bounds-multiple-1.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <A> = 301;\n"
                "    <B> = 300;\n"
                "    override <A> = 1;\n"
                "    augment  <B> = 302;\n"
                "  };\n"
                "  xkb_types { };\n"
                "  xkb_compat {\n"
                "    interpret.repeat= False;\n"
                "  };\n"
                "  xkb_symbols { };\n"
                "};",
            /* NOTE: same as previous */
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-bounds-multiple-1.xkb"
        },
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_compile_output(ctx, compile_buffer, NULL, __func__,
                                   keymaps[k].keymap, strlen(keymaps[k].keymap),
                                   keymaps[k].expected, update_output_files));
    }
}

static void
test_masks(struct xkb_context *ctx, bool update_output_files) {
    const struct keymap_test_data keymaps[] = {
        {
                .keymap =
                    "xkb_keymap {\n"
                    "  xkb_keycodes { };\n"
                    "  xkb_types { };\n"
                    "  xkb_compat {\n"
                    /* Cannot be negative */
                    "    virtual_modifiers Test1 = -1;\n"
                    "  };\n"
                    "  xkb_symbols { };\n"
                    "};",
                .expected = NULL
        },
        {
                .keymap =
                    "xkb_keymap {\n"
                    "  xkb_keycodes { };\n"
                    "  xkb_types { };\n"
                    "  xkb_compat {\n"
                    /* Out of range (expect 32bits) */
                    "    virtual_modifiers Test1 = 0x100000000;\n"
                    "  };\n"
                    "  xkb_symbols { };\n"
                    "};",
                .expected = NULL
        },
        {
                .keymap =
                    "xkb_keymap {\n"
                    "  xkb_keycodes { };\n"
                    "  xkb_types { };\n"
                    "  xkb_compat {\n"
                    /* Out of range (expect 32bits) */
                    "    virtual_modifiers Test1 = ~0x100000000;\n"
                    "  };\n"
                    "  xkb_symbols { };\n"
                    "};",
                .expected = NULL
        },
        {
                .keymap =
                    "xkb_keymap {\n"
                    "  xkb_keycodes { };\n"
                    "  xkb_types { };\n"
                    "  xkb_compat {\n"
                    /* Unsupported operator */
                    "    virtual_modifiers Test1 = !Mod1;\n"
                    "  };\n"
                    "  xkb_symbols { };\n"
                    "};",
                .expected = NULL
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { };\n"
                "  xkb_types {\n"
                /* Try range */
                "    virtual_modifiers Test01 = 0;\n"
                "    virtual_modifiers Test02 = 0xffffffff;\n"
                /* Try various operations on masks */
                "    virtual_modifiers Test11 = 0xf0 + 0x0f;\n"
                "    virtual_modifiers Test12 = 0xff - 0x0f;\n"
                "    virtual_modifiers Test13 = ~0xf;\n"
                "    virtual_modifiers Test14 = ~none;\n"
                "    virtual_modifiers Test15 = ~all;\n"
                "    virtual_modifiers Test16 = ~0xffffffff;\n"
                "    virtual_modifiers Test17 = all - ~Mod1 + Mod2;\n"
                "    type \"XXX\" {\n"
                "      modifiers = Test12;\n"
                /* Masks mappings are not resolved here, so:
                 *   map[Test12 - Mod5] <=> map[Test12] */
                "      map[Test12 - Mod5] = 2;\n"
                "    };\n"
                "  };\n"
                "  xkb_compat { };\n"
                "  xkb_symbols { };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "masks.xkb"
        }
    };
    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_compile_output(ctx, compile_buffer, NULL, __func__,
                                   keymaps[k].keymap, strlen(keymaps[k].keymap),
                                   keymaps[k].expected, update_output_files));
    }
}

static void
test_interpret(struct xkb_context *ctx, bool update_output_files)
{

    const struct keymap_test_data keymaps[] = {
        /* Invalid: empty string */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { <> = 10; };\n"
                "  xkb_types { include \"basic\" };\n"
                "  xkb_compat {\n"
                "    interpret \"\" { repeat = false; }\n"
                "  };\n"
                "  xkb_symbols { };\n"
                "};",
            .expected = NULL
        },
        /* Invalid UTF-8 encoding */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { <> = 10; };\n"
                "  xkb_types { include \"basic\" };\n"
                "  xkb_compat {\n"
                "    interpret \"\xff\" { repeat = false; };\n"
                "  };\n"
                "  xkb_symbols { };\n"
                "};",
            .expected = NULL
        },
        /* Invalid multiple Unicode code points */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { <> = 10; };\n"
                "  xkb_types { include \"basic\" };\n"
                "  xkb_compat {\n"
                "    interpret \"ab\" { repeat = false; };\n"
                "  };\n"
                "  xkb_symbols { };\n"
                "};",
            .expected = NULL
        },
        /* Valid */
        {
            .keymap =
                u8"xkb_keymap {\n"
                u8"  xkb_keycodes { };\n"
                u8"  xkb_types { };\n"
                u8"  xkb_compat {\n"
                u8"   interpret 0x1     { repeat = false; };\n"
                u8"   interpret 0xB     { repeat = false; };\n"
                u8"   interpret Shift_L { repeat = false; };\n"
                u8"   interpret a       { repeat = false; };\n"
                u8"   interpret \"ä\"   { repeat = false; };\n"
                u8"   interpret \"✨\"  { repeat = false; };\n"
                u8"   interpret \"🎺\"  { repeat = false; };\n"
                u8"  };\n"
                u8"  xkb_symbols { };\n"
                u8"};",
            .expected = GOLDEN_TESTS_OUTPUTS "compat-interpret.xkb"
        },
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_compile_output(ctx, compile_buffer, NULL, __func__,
                                   keymaps[k].keymap, strlen(keymaps[k].keymap),
                                   keymaps[k].expected, update_output_files));
    }
}

/* Test various multi-{keysym,action} syntaxes */
static void
test_multi_keysyms_actions(struct xkb_context *ctx, bool update_output_files)
{
    /* Macro to create keymap for failing tests */
#define make_keymap(xs)                         \
        "xkb_keymap {\n"                        \
        "  xkb_keycodes {\n"                    \
        "    <AE01> = 10;\n"                    \
        "  };\n"                                \
        "  xkb_types { include \"basic\" };\n"  \
        "  xkb_compat { include \"basic\" };\n" \
        "  xkb_symbols {\n"                     \
        "    key <AE01> { [" xs "] };\n"        \
        "  };\n"                                \
        "};"

#define make_keymaps_with(func, name, a, b, c, d)                           \
    /* Test: valid keymaps */                                               \
    {                                                                       \
        .keymap =                                                           \
            "xkb_keymap {\n"                                                \
            "  xkb_keycodes {\n"                                            \
            "    <01> = 1;\n"                                               \
            "    <02> = 2;\n"                                               \
            "    <03> = 3;\n"                                               \
            "    <04> = 4;\n"                                               \
            "    <05> = 5;\n"                                               \
            "    <06> = 6;\n"                                               \
            "    <07> = 7;\n"                                               \
            "    <08> = 8;\n"                                               \
            "    <09> = 9;\n"                                               \
            "    <10> = 10;\n"                                              \
            "  };\n"                                                        \
            "  xkb_types { include \"basic+extra\" };\n"                    \
            "  xkb_compat { include \"basic\" };\n"                         \
            "  xkb_symbols {\n"                                             \
            "    key <01> { [ "a "] };\n"                                   \
            "    key <02> { [ "a", "b" ] };\n"                              \
            "    key <03> { [ { "a" } ] };\n"                               \
            "    key <04> { [ { "a", "b" } ] };\n"                          \
            "    key <05> { [ { "a", "b", "c" } ] };\n"                     \
            "    key <06> { [ "a", { "b", "c" } ] };\n"                     \
            "    key <07> { [ { "a", "b" }, "c" ] };\n"                     \
            "    key <08> { [ { "a", "b" }, { "c", "d" } ] };\n"            \
            "    key <09> { [ { "a", "b" }, "c", { "d", "a" } ] };\n"       \
            "    key <10> { [ { "a", "b" }, { "c", "d" }, "a" ] };\n"       \
            "  };\n"                                                        \
            "};",                                                           \
        .expected = GOLDEN_TESTS_OUTPUTS "symbols-multi-" name              \
    },                                                                      \
    /* Test: invalid keymaps */                                             \
    { .keymap = func("{}")                            , .expected = NULL }, \
    { .keymap = func("{ {} }")                        , .expected = NULL }, \
    { .keymap = func("{ "a", {} }")                   , .expected = NULL }, \
    { .keymap = func("{ {}, "b" }")                   , .expected = NULL }, \
    { .keymap = func("{ {}, {} }")                    , .expected = NULL }, \
    { .keymap = func("{ "a", { "b" } }")              , .expected = NULL }, \
    { .keymap = func("{ { "a" }, "b" }")              , .expected = NULL }, \
    { .keymap = func("{ { "a", "b" }, "c" }")         , .expected = NULL }, \
    { .keymap = func("{ "a", { "b", "c" } }")         , .expected = NULL }, \
    { .keymap = func("{ "a", {}, "c" }")              , .expected = NULL }, \
    { .keymap = func("{ "a", "b", {} }")              , .expected = NULL }, \
    { .keymap = func("{ {}, "b", "c" }")              , .expected = NULL }, \
    { .keymap = func("{ { "a", "b" }, "c", "d" }")    , .expected = NULL }, \
    { .keymap = func("{ "a", { "b", "c" }, "d" }")    , .expected = NULL }, \
    { .keymap = func("{ "a", "b", { "c", "d" } }")    , .expected = NULL }, \
    { .keymap = func("{ { "a", "b" }, { "c", "d" } }"), .expected = NULL }

    const struct keymap_test_data keymaps[] = {
        make_keymaps_with(make_keymap, "keysyms",
                          "a", "b", "c", "d"),
        make_keymaps_with(make_keymap, "actions",
                          "SetMods(modifiers=Control)",
                          "SetGroup(group=+1)",
                          "Private(data=\"foo\")",
                          "Private(data=\"bar\")")
    };
    for (size_t k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%zu ***\n", __func__, k);
        assert(test_compile_output(ctx, compile_buffer, NULL, __func__,
                                   keymaps[k].keymap,
                                   strlen(keymaps[k].keymap),
                                   keymaps[k].expected,
                                   update_output_files));
    }
#undef make_keymaps_with
#undef make_keymap
}

/* Test keysyms as strings */
static void
test_keysyms_as_strings(struct xkb_context *ctx, bool update_output_files)
{
    const struct keymap_test_data keymaps[] = {
        /* Invalid UTF-8 encoding */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { <> = 10; };\n"
                "  xkb_types { include \"basic\" };\n"
                "  xkb_compat { };\n"
                "  xkb_symbols {\n"
                "    key <> { [\"\xff\"] };\n"
                "  };\n"
                "};",
            .expected = NULL
        },
        {
            .keymap =
                u8"xkb_keymap {\n"
                u8"  xkb_keycodes {\n"
                u8"    <10> = 10;\n"
                u8"    <11> = 11;\n"
                u8"    <12> = 12;\n"
                u8"    <20> = 20;\n"
                u8"    <21> = 21;\n"
                u8"    <22> = 22;\n"
                u8"    <23> = 23;\n"
                u8"    <24> = 24;\n"
                u8"    <25> = 25;\n"
                u8"    <30> = 30;\n"
                u8"    <31> = 31;\n"
                u8"    <32> = 32;\n"
                u8"    <33> = 33;\n"
                u8"    <34> = 34;\n"
                u8"    <35> = 35;\n"
                u8"    <40> = 40;\n"
                u8"    <41> = 41;\n"
                u8"    <42> = 42;\n"
                u8"    <50> = 50;\n"
                u8"    <51> = 51;\n"
                u8"    <52> = 52;\n"
                u8"    <60> = 60;\n"
                u8"    <61> = 61;\n"
                u8"    <62> = 62;\n"
                u8"    <63> = 63;\n"
                u8"    <64> = 64;\n"
                u8"    <AD08> = 70;\n"
                u8"    <AC05> = 71;\n"
                u8"    <AB05> = 72;\n"
                u8"    <AD01> = 73;\n"
                u8"  };\n"
                u8"  xkb_types { include \"basic\" };\n"
                u8"  xkb_compat { };\n"
                u8"  xkb_symbols {\n"
                /* Empty string */
                u8"    key <10> { [\"\", {b, \"\", c}] };\n"
                u8"    key <11> { [{a, \"\"}, {b, \"\"}] };\n"
                u8"    key <12> { [{\"\"}, {\"\", \"\"}] };\n"
                /* Single string: Plain */
                u8"    key <20> { [\"a\", \"bc\"] };\n"
                u8"    key <23> { [\"✨\", \"🎺\"] };\n" // U+2728 ✨, // U+1F3BA 🎺
                u8"    key <24> { [\"u\u0308\"] };\n" // u + U+0308 ◌̈ COMBINING DIAERESIS
                u8"    key <25> { [\"∀∂∈ℝ∧∪≡∞ ↑↗↨↻⇣ ┐┼╔╘░►☺♀ ﬁ�⑀₂ἠḂӥẄɐː⍎אԱა\"] };\n"
                /* Single string: Nested */
                u8"    key <30> { [{\"a\"      }, {\"bc\"      }] };\n"
                u8"    key <31> { [{\"a\", \"\"}, {\"bc\", \"\"}] };\n"
                u8"    key <32> { [{\"\", \"a\"}, {\"\", \"bc\"}] };\n"
                u8"    key <33> { [{\"✨\"}, {\"🎺\"}] };\n" // U+2728 ✨, // U+1F3BA 🎺
                u8"    key <34> { [{\"u\u0308\"}] };\n" // u + U+0308 ◌̈ COMBINING DIAERESIS
                u8"    key <35> { [{\"∀∂∈ℝ∧∪≡∞ ↑↗↨↻⇣ ┐┼╔╘░►☺♀ ﬁ�⑀₂ἠḂӥẄɐː⍎אԱა\"}] };\n"
                /* Multi: string, literal */
                u8"    key <40> { [{\"a\",       b}, {\"cde\",       f}] };\n"
                u8"    key <41> { [{\"a\", \"\", b}, {\"cde\", \"\", f}] };\n"
                u8"    key <42> { [{\"a\", b, \"\"}, {\"cde\", f, \"\"}] };\n"
                /* Multi: literal, string */
                u8"    key <50> { [{a,       \"b\"}, {c,       \"def\"}] };\n"
                u8"    key <51> { [{a, \"\", \"b\"}, {c, \"\", \"def\"}] };\n"
                u8"    key <52> { [{a, \"b\", \"\"}, {c, \"def\", \"\"}] };\n"
                /* Multi: string, string */
                u8"    key <60> { [{\"a\",       \"b\"}, {\"cd\",       \"ef\"}] };\n"
                u8"    key <61> { [{\"a\", \"\", \"b\"}, {\"cd\", \"\", \"ef\"}] };\n"
                u8"    key <63> { [{\"a\",       \"bcd\"}, {\"efg\",       \"h\"}] };\n"
                u8"    key <64> { [{\"a\", \"\", \"bcd\"}, {\"efg\", \"\", \"h\"}] };\n"
                /* Example from the doc */
                u8"key <AD08> { [ \"ij\" , \"Ĳ\"   ] }; // IJ Dutch digraph\n"
                u8"key <AC05> { [ \"g̃\"  , \"G̃\"   ] }; // G̃ Guarani letter\n"
                /* NOTE: We use U+200E LEFT-TO-RIGHT MARK in order to display the strings in
                 *       in the proper order. */
                u8"key <AB05> { [ \"لا\"‎  , \"لآ\"‎   ] }; // ⁧لا⁩ ⁧لآ⁩ Arabic Lam-Alef ligatures decomposed\n"
                u8"key <AD01> { [ \"c’h\", \"C’h\" ] }; // C’H Breton trigraph\n"
                u8"  };\n"
                u8"};",
            .expected = GOLDEN_TESTS_OUTPUTS "symbols-key-string-keysyms.xkb"
        },
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_compile_output(ctx, compile_buffer, NULL, __func__,
                                   keymaps[k].keymap, strlen(keymaps[k].keymap),
                                   keymaps[k].expected, update_output_files));
    }
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
        "};",
        /* Used to parse without error because the “repeats” key field is valid,
         * but we should fail in the following 2 keymaps */
        "xkb_keymap {\n"
        "    xkb_keycodes { <> = 9; };\n"
        "    xkb_types { };\n"
        "    xkb_compat { };\n"
        "    xkb_symbols { key <> { vmods=[], repeats=false }; };\n"
        "};",
        "xkb_keymap {\n"
        "    xkb_keycodes { <> = 9; };\n"
        "    xkb_types { };\n"
        "    xkb_compat { };\n"
        "    xkb_symbols { key <> { repeats=false, vmods=[] }; };\n"
        "};"
    };
    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        const struct xkb_keymap *keymap =
            test_compile_buffer(ctx, keymaps[k], strlen(keymaps[k]));
        assert(!keymap);
    }
}

static void
test_modifier_maps(struct xkb_context *ctx, bool update_output_files)
{
    /* Only accept key and keysyms in the modifier_map list */
    struct keymap_test_data keymaps[] = {
        {
            .keymap =
                u8"xkb_keymap {\n"
                u8"  xkb_keycodes {\n"
                u8"    <CAPS> = 66;\n"
                u8"    alias <LOCK> = <CAPS>;\n"
                u8"    <0> = 0;"
                u8"    <1> = 1;"
                u8"    <2> = 2;"
                u8"    <3> = 3;"
                u8"    <any>  = 10;"
                u8"    <none> = 11;"
                u8"    <a> = 61;"
                u8"    <b> = 62;"
                u8"    <c> = 63;"
                u8"    <100> = 100;"
                u8"  };\n"
                u8"  xkb_types { include \"basic\" };\n"
                u8"  xkb_compat { };\n"
                u8"  xkb_symbols {\n"
                u8"    key <CAPS> { [Caps_Lock] };\n"
                u8"    key <any>  { [any, A] };\n"
                u8"    key <none> { [none, N] };\n"
                u8"    key <0>    { [0] };\n"
                u8"    key <1>    { [1] };\n"
                u8"    key <2>    { [2] };\n"
                u8"    key <a>    { [a] };\n"
                u8"    key <b>    { [b] };\n"
                u8"    key <c>    { [\"🎺\"] };\n"
                u8"    key <3>    { [NotAKeysym, 3] };\n"
                u8"    key <100>  { [C] };\n"
                u8"    modifier_map Lock {\n"
                u8"      <100>, <LOCK>, any, none,\n"
                u8"      0, 1, 0x2, a, \"b\", \"🎺\", NotAKeysym\n"
                u8"    };\n"
                u8"  };\n"
                u8"};",
            .expected = GOLDEN_TESTS_OUTPUTS "symbols-modifier_map.xkb"
        },
        /* Invalid: empty string */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { };\n"
                "  xkb_types { };\n"
                "  xkb_compat { };\n"
                "  xkb_symbols { modifier_map Lock { \"\" }; };\n"
                "};",
            .expected = NULL
        },
        /* Invalid Unicode encoding */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { };\n"
                "  xkb_types { };\n"
                "  xkb_compat { };\n"
                "  xkb_symbols { modifier_map Lock { \"\xff\" }; };\n"
                "};",
            .expected = NULL
        },
        /* Invalid multiple Unicode code points */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { };\n"
                "  xkb_types { };\n"
                "  xkb_compat { };\n"
                "  xkb_symbols { modifier_map Lock { \"ab\" }; };\n"
                "};",
            .expected = NULL
        },
        /* Invalid type: list */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { };\n"
                "  xkb_types { };\n"
                "  xkb_compat { };\n"
                "  xkb_symbols { modifier_map Lock { [a] }; };\n"
                "};",
            .expected = NULL
        },
        /* Invalid type: list */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { };\n"
                "  xkb_types { };\n"
                "  xkb_compat { };\n"
                "  xkb_symbols { modifier_map Lock { {a, b} }; };\n"
                "};",
            .expected = NULL
        },
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_compile_output(ctx, compile_buffer, NULL, __func__,
                                   keymaps[k].keymap, strlen(keymaps[k].keymap),
                                   keymaps[k].expected, update_output_files));
    }
}

static void
test_prebuilt_keymap_roundtrip(struct xkb_context *ctx, bool update_output_files)
{
    /* Load in a prebuilt keymap, make sure we can compile it from memory,
     * then compare it to make sure we get the same result when dumping it
     * to a string. */
    const char * const path = GOLDEN_TESTS_OUTPUTS "stringcomp.data";
    char *original = test_read_file(path);
    assert(original);

    /* Load a prebuild keymap, once without, once with the trailing \0 */
    for (unsigned int i = 0; i <= 1; i++) {
        fprintf(stderr, "------\n*** %s, trailing '\\0': %d ***\n", __func__, i);

        assert(test_compile_output(ctx, compile_buffer, NULL, __func__,
                                   original, strlen(original) + i, path,
                                   update_output_files));
    }

    free(original);
}

static void
test_keymap_from_rules(struct xkb_context *ctx)
{
    /* Make sure we can recompile our output for a normal keymap from rules. */
    fprintf(stderr, "------\n*** %s ***\n", __func__);
    struct xkb_keymap *keymap = test_compile_rules(ctx, NULL, NULL,
                                                   "ru,ca,de,us",
                                                   ",multix,neo,intl", NULL);
    assert(keymap);
    char *dump = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_USE_ORIGINAL_FORMAT);
    assert(dump);
    xkb_keymap_unref(keymap);
    keymap = test_compile_buffer(ctx, dump, strlen(dump));
    assert(keymap);
    xkb_keymap_unref(keymap);
    free(dump);
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
    assert(ctx);

    /* Make sure we can't (falsely claim to) compile an empty string. */
    struct xkb_keymap *keymap = test_compile_buffer(ctx, "", 0);
    assert(!keymap);

    test_encodings(ctx);
    test_component_syntax_error(ctx);
    test_recursive(ctx);
    test_alloc_limits(ctx);
    test_integers(ctx, update_output_files);
    test_keycodes(ctx, update_output_files);
    test_masks(ctx, update_output_files);
    test_interpret(ctx, update_output_files);
    test_multi_keysyms_actions(ctx, update_output_files);
    test_keysyms_as_strings(ctx, update_output_files);
    test_invalid_symbols_fields(ctx);
    test_modifier_maps(ctx, update_output_files);
    test_prebuilt_keymap_roundtrip(ctx, update_output_files);
    test_keymap_from_rules(ctx);

    xkb_context_unref(ctx);

    return EXIT_SUCCESS;
}
